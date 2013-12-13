#include <map>
#include <set>
#include <vector>
#include <cstdio>

#include "llvm/Pass.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/ADT/StringMap.h"

#include "Util.h"

using namespace llvm;
using std::map;
using std::set;
using std::vector;

namespace {
  struct BaggyBoundsPointersOpt : public ModulePass {
    static char ID;
    BaggyBoundsPointersOpt() : ModulePass(ID) {}
    Constant *sizeTable;
    Constant *slowPathFunc;
    DominatorTree* DT;
    DataLayout* DL;

    // map from global variables to their sizes
    map<Value*, int> globalVarSizes;

    BasicBlock *instrumentMemset(BasicBlock *orig, MemSetInst *i, Value* sizeValue) {
     BasicBlock *baggyBlock = BasicBlock::Create(orig->getContext(), "baggy.check",
                                                  orig->getParent(), orig);
      IRBuilder<> builder(baggyBlock);

      Value *Base = i->getDest();
      Value *Length = i->getLength();
      Value *LengthSized;
      if (Length->getType()->getPrimitiveSizeInBits() > 32)  {
        LengthSized = builder.CreateTrunc(Length, Type::getInt32Ty(baggyBlock->getContext()));
      } else {
        LengthSized = builder.CreateZExtOrBitCast(Length, Type::getInt32Ty(baggyBlock->getContext()));
      }
      Value *BaseInt = builder.CreatePtrToInt(Base, Type::getInt32Ty(baggyBlock->getContext()));
      Value *EndInt = builder.CreateAdd(BaseInt, LengthSized);
      /*
      Value *TableOffset = builder.CreateLShr(BaseInt, 4, "baggy.offset");
      LoadInst *SizeTablePtr = builder.CreateLoad(sizeTable, "baggy.table");
      Value *Tableaddr = builder.CreateInBoundsGEP(SizeTablePtr, TableOffset);
      LoadInst *Size = builder.CreateLoad(Tableaddr, "alloc.size");
      */
      Value *MaskedSize = builder.CreateZExtOrBitCast(sizeValue, Type::getInt32Ty(baggyBlock->getContext()));
      Value *SizeInt = builder.CreateAnd(MaskedSize, 0x1F);
      Value *Xor = builder.CreateXor(BaseInt, EndInt);
      Value *Result = builder.CreateAShr(Xor, SizeInt);

      // Add the memset instruction to this block
      baggyBlock->getInstList().push_back(i);

      // Create the slowpath block
      BasicBlock *slowPathBlock = BasicBlock::Create(baggyBlock->getContext(), "baggy.slowPath",
                                                     baggyBlock->getParent(), orig);
      IRBuilder<> slowPathBuilder(slowPathBlock);
      Value *slowPathBr, *bufcast, *pcast, *retptr;

      bufcast = slowPathBuilder.CreatePointerCast(Base, Type::getInt8PtrTy(baggyBlock->getContext()));
      pcast = slowPathBuilder.CreateIntToPtr(EndInt, Type::getInt8PtrTy(baggyBlock->getContext()));
      retptr = slowPathBuilder.CreateCall2(slowPathFunc, bufcast, pcast);
      slowPathBr = slowPathBuilder.CreateBr(orig);

      // Branch to slowpath if necessary
      MDBuilder weightBuilder(baggyBlock->getContext());
      MDNode *branchWeights;
      Value *baggyCheck, *baggyBr;
      baggyCheck = builder.CreateICmpEQ(Result,ConstantInt::get(IntegerType::get(baggyBlock->getContext(), 32), 0));
      branchWeights = weightBuilder.createBranchWeights(99, 1);
      baggyBr = builder.CreateCondBr(baggyCheck, orig, slowPathBlock, branchWeights);

      return baggyBlock;
    }

    BasicBlock *instrumentGEP(BasicBlock *orig, GetElementPtrInst *i, PHINode *phi,
                                                              Value* sizeValue) {
      BasicBlock *baggyBlock = BasicBlock::Create(orig->getContext(), "baggy.check",
                                                  orig->getParent(), orig);
      IRBuilder<> builder(baggyBlock);
      Value *tmpsize, *sizeint, *baseint, *base;

      // Baggy lookup
      /*
      Value *base, *baseint, *tableoffset, *tableaddr, *inst, *sizeint, *tmpsize, *sizeTableAddr;
      Value *size, *sizeTablePtr;
      base = builder.CreateConstInBoundsGEP1_32(i->getOperand(0), 0, "baggy.base");
      baseint = builder.CreatePtrToInt(base, IntegerType::get(baggyBlock->getContext(), 32));
      tableoffset = builder.CreateLShr(baseint, 4, "baggy.offset");
      //sizeTableAddr = builder.CreateConstInBoundsGEP1_32(sizeTable, 0);
      sizeTablePtr = builder.CreateLoad(sizeTable, "baggy.table");
      tableaddr = builder.CreateInBoundsGEP(sizeTablePtr, tableoffset);
      size = builder.CreateLoad(tableaddr, "alloc.size");
      */
      base = builder.CreateConstInBoundsGEP1_32(i->getOperand(0), 0, "baggy.base");
      baseint = builder.CreatePtrToInt(base, IntegerType::get(baggyBlock->getContext(), 32));
      tmpsize = builder.CreateZExtOrBitCast(sizeValue, IntegerType::get(baggyBlock->getContext(), 32));
      sizeint = builder.CreateAnd(tmpsize, 0x1F);

      // insert arithmetic
      baggyBlock->getInstList().push_back(i);

      // baggy check
      Value *combine, *instint, *result;

      instint = builder.CreatePtrToInt(i, IntegerType::get(baggyBlock->getContext(), 32));
      combine = builder.CreateXor(baseint, instint);
      result = builder.CreateAShr(combine, sizeint, "baggy.result");

      // Create the slowpath block
      BasicBlock *slowPathBlock = BasicBlock::Create(baggyBlock->getContext(), "baggy.slowPath",
                                                     baggyBlock->getParent(), orig);
      IRBuilder<> slowPathBuilder(slowPathBlock);
      Value *slowPathBr, *slowPathPtr, *bufcast, *pcast, *retptr;

      bufcast = slowPathBuilder.CreatePointerCast(base, Type::getInt8PtrTy(baggyBlock->getContext()));
      pcast = slowPathBuilder.CreatePointerCast(i, Type::getInt8PtrTy(baggyBlock->getContext()));
      retptr = slowPathBuilder.CreateCall2(slowPathFunc, bufcast, pcast);
      slowPathPtr = slowPathBuilder.CreatePointerCast(retptr, i->getType());
      slowPathBr = slowPathBuilder.CreateBr(orig);

      // Branch to slowpath if necessary
      MDBuilder weightBuilder(baggyBlock->getContext());
      MDNode *branchWeights;
      Value *baggyCheck, *baggyBr;
      baggyCheck = builder.CreateICmpEQ(result,ConstantInt::get(IntegerType::get(baggyBlock->getContext(), 32), 0));
      branchWeights = weightBuilder.createBranchWeights(99, 1);
      baggyBr = builder.CreateCondBr(baggyCheck, orig, slowPathBlock, branchWeights);

      // Add both branches to phi node
      phi->addIncoming(i, baggyBlock);
      phi->addIncoming(slowPathPtr, slowPathBlock);

      return baggyBlock;
    }

    Value* castToIntAndClearTopBit(LLVMContext& ctxt,
                                    BasicBlock::InstListType& iList,
                                    BasicBlock::InstListType::iterator& i,
                                    Value* val) {
      Instruction* toIntInst = new PtrToIntInst(val, IntegerType::get(ctxt, 32));
      Instruction* zeroBitInst = BinaryOperator::Create(Instruction::And, toIntInst,
                ConstantInt::get(IntegerType::get(ctxt, 32), 0x7fffffff));
      iList.insert(i, toIntInst);
      iList.insert(i, zeroBitInst);
      return zeroBitInst;
    }

    virtual bool runOnModule(Module &M) {
      sizeTable = M.getOrInsertGlobal("baggy_size_table", Type::getInt8PtrTy(getGlobalContext()));
      slowPathFunc = M.getFunction("baggy_slowpath");
      if (!slowPathFunc) {
        std::vector<Type *> FuncTy_2_args;
        FuncTy_2_args.push_back(Type::getInt8PtrTy(getGlobalContext()));
        FuncTy_2_args.push_back(Type::getInt8PtrTy(getGlobalContext()));

        FunctionType *FuncTy_bsp = FunctionType::get(Type::getInt8PtrTy(getGlobalContext()),
                                                     FuncTy_2_args, false);

        slowPathFunc = M.getOrInsertFunction("baggy_slowpath", FuncTy_bsp);
      }

      DL = &getAnalysis<DataLayout>();

      for (Module::global_iterator globalVar = M.global_begin();
            globalVar != M.global_end(); ++globalVar) {
        globalVarSizes[globalVar] = DL->getTypeAllocSize(globalVar->getType()->getElementType());
      }

      for (Module::FunctionListType::iterator F = M.begin(); F != M.end(); ++F) {
        if (F->begin() != F->end()) {
          // check that it isn't just a declaration
          runOnFunction(*F);
        }
      }
    }

    void getPointerSizesBasicBlock(DomTreeNode *N, Value* SizeTablePtr, map<Value*,Value*>& ptrToSize, set<Value*>& ignorePtrs, BasicBlock::InstListType::iterator startIter) {
      BasicBlock* BB = N->getBlock();

      BasicBlock::InstListType& instList = BB->getInstList();

      for (BasicBlock::InstListType::iterator inst = startIter;
              inst != instList.end(); ++inst) {
        if (inst->getType()->isPointerTy()) {
          switch (inst->getOpcode()) {
          case Instruction::GetElementPtr:
            ptrToSize[inst] = ptrToSize[inst->getOperand(0)];
            break;
          case Instruction::PHI:
          {
            PHINode* phi = cast<PHINode>(inst);
            PHINode* new_phi = PHINode::Create(IntegerType::get(BB->getContext(), 8),
                                                phi->getNumIncomingValues());
            instList.insert(inst, new_phi);
            ptrToSize[phi] = new_phi;
            // Due to cyclicity (i.e., a phi node is not dominated by the definitions of
            // all of its arguments) we need to create everything ptrToSize before we
            // can insert values into the newly created phi node new_phi. We will do this
            // at the end.

            break;
          }
          case Instruction::Alloca:
          {
            unsigned int allocation_size = DL->getTypeAllocSize(cast<AllocaInst>(inst)->getType()->getElementType());
            unsigned int real_allocation_size = get_alignment(allocation_size);
            ptrToSize[inst] = ConstantInt::get(IntegerType::get(BB->getContext(), 8), real_allocation_size);
            break;
          }
          case Instruction::BitCast:
          {
            Value* operand = inst->getOperand(0);
            if (operand->getType()->isPointerTy()) {
              ptrToSize[inst] = ptrToSize[operand];
              break;
            }
            // else spill over into default
          }

          // TODO any more to add? maybe a malloc or realloc call?
          default:
            Instruction* base = GetElementPtrInst::CreateInBounds(inst, ConstantInt::get(IntegerType::get(BB->getContext(), 32), 0), "baggy.base");
            Instruction* baseint = new PtrToIntInst(base, IntegerType::get(BB->getContext(), 32));
            Instruction* tableoffset = BinaryOperator::Create(Instruction::LShr, baseint, ConstantInt::get(IntegerType::get(BB->getContext(), 32), 4), "baggy.offset");
            Instruction* tableaddr = GetElementPtrInst::CreateInBounds(SizeTablePtr, tableoffset);
            Instruction* size = new LoadInst(tableaddr); //, Twine("alloc.size.", inst->getValueName()->getKey())); 
            ptrToSize[inst] = size;
            ++inst;
            instList.insert(inst, base);
            instList.insert(inst, baseint);
            instList.insert(inst, tableoffset);
            instList.insert(inst, tableaddr);
            instList.insert(inst, size);
            --inst;
            ignorePtrs.insert(tableaddr);
            ignorePtrs.insert(base);
            break;
          }
        }
      }

      // recurse on children
      const std::vector<DomTreeNode*> &Children = N->getChildren();
      for (unsigned i = 0, e = Children.size(); i != e; ++i) {
        getPointerSizesBasicBlock(Children[i], SizeTablePtr, ptrToSize, ignorePtrs, Children[i]->getBlock()->getInstList().begin());
      }

    }

    void getPointerSizes(Function& F, map<Value*, Value*>& ptrToSize, set<Value*>& ignorePtrs) {
      // Globals
      for (map<Value*, int>::iterator globalVarP = globalVarSizes.begin();
                globalVarP != globalVarSizes.end(); ++globalVarP) {
        Value* globalVar = globalVarP->first;
        unsigned int real_allocation_size = get_alignment(globalVarP->second);
        ptrToSize[globalVar] = ConstantInt::get(IntegerType::get(F.getContext(), 32), real_allocation_size);
      }

      BasicBlock& entryBlock = F.getEntryBlock();
      BasicBlock::InstListType& instList = entryBlock.getInstList();
      BasicBlock::InstListType::iterator iter = instList.begin();

      // Load the value of baggy_size_table first
      LoadInst *SizeTablePtr = new LoadInst(sizeTable, "baggy.table");
      instList.insert(iter, SizeTablePtr);

      // Arguments, place at top of entry block (entry block has no phi nodes)
      Function::ArgumentListType& argumentList = F.getArgumentList();

      for (Function::ArgumentListType::iterator argument = argumentList.begin();
                argument != argumentList.end(); ++argument) {
        if (argument->getType()->isPointerTy()) {
          Instruction* base = GetElementPtrInst::CreateInBounds(argument, ConstantInt::get(IntegerType::get(F.getContext(), 32), 0), "baggy.base");
          Instruction* baseint = new PtrToIntInst(base, IntegerType::get(F.getContext(), 32));
          Instruction* tableoffset = BinaryOperator::Create(Instruction::LShr, baseint, ConstantInt::get(IntegerType::get(F.getContext(), 32), 4), "baggy.offset");
          Instruction* tableaddr = GetElementPtrInst::CreateInBounds(SizeTablePtr, tableoffset);
          Instruction* size = new LoadInst(tableaddr); //, Twine("alloc.size.", argument->getValueName()->getKey()));
          instList.insert(iter, base);
          instList.insert(iter, baseint);
          instList.insert(iter, tableoffset);
          instList.insert(iter, tableaddr);
          instList.insert(iter, size);
          ptrToSize[argument] = size;
          ignorePtrs.insert(base);
          ignorePtrs.insert(tableaddr);
        }
      }

      // Traverse dominator tree in pre-order so that we see definitions before uses.
      getPointerSizesBasicBlock(DT->getNode(&entryBlock), SizeTablePtr, ptrToSize, ignorePtrs, iter);

      // Now finish off the phi nodes
      for (map<Value*, Value*>::iterator phi_pair = ptrToSize.begin(); phi_pair != ptrToSize.end(); ++phi_pair) {
        if (isa<PHINode>(phi_pair->first)) {
          PHINode* phi = cast<PHINode>(phi_pair->first);
          PHINode* new_phi = cast<PHINode>(phi_pair->second);
          for (PHINode::block_iterator pred_bb = phi->block_begin();
                  pred_bb != phi->block_end(); ++pred_bb) {
            new_phi->addIncoming(ptrToSize[phi->getIncomingValueForBlock(*pred_bb)], *pred_bb);
          }
        }
      }

    }

    bool runOnFunction(Function &F) {
      DT = &getAnalysis<DominatorTree>(F);

      for (Function::iterator bb = F.begin(), bbend = F.end(); bb != bbend; ++bb) {
        BasicBlock *block = &(*bb);
        BasicBlock::InstListType& iList = block->getInstList();
        for (BasicBlock::InstListType::iterator i = iList.begin(); i != iList.end(); ++i) {
          if (isa<PtrToIntInst>(*i)) {
            PtrToIntInst* ptii = cast<PtrToIntInst>(i);
            if (ptii->getDestTy()->getPrimitiveSizeInBits() >= 32) {
              // AND out the most significant bit of newly created int
              PtrToIntInst* inst1 = cast<PtrToIntInst>(ptii->clone());
              BinaryOperator* inst2 = BinaryOperator::Create(Instruction::And, inst1, ConstantInt::get(IntegerType::get(block->getContext(),32), 0x7fffffff));
              iList.insert(i, inst1);
              ReplaceInstWithInst(i->getParent()->getInstList(), i, inst2);
            }
          }
          else if (isa<ICmpInst>(*i)) {
            ICmpInst* ici = cast<ICmpInst>(i);
            if (!ici->isEquality()) {
              Value* operand2 = ici->getOperand(1);
              ICmpInst* new_ici = cast<ICmpInst>(ici->clone());
              for (int op_num = 0; op_num < 2; op_num++) {
                Value* operand = ici->getOperand(op_num);
                if (operand->getType()->isPointerTy()) {
                  new_ici->setOperand(op_num, castToIntAndClearTopBit(block->getContext(), iList, i, operand));
                }
              }
              ReplaceInstWithInst(i->getParent()->getInstList(), i, new_ici);
            }
          }
        }
      }

      map<Value*, Value*> ptrToSize;
      set<Value*> ignorePtrs; // any new getelementptr instructions you make should be ignored
      getPointerSizes(F, ptrToSize, ignorePtrs); // fills up ptrToSize map

      for (Function::iterator bb = F.begin(), bbend = F.end(); bb != bbend; ++bb) {
        BasicBlock *block = &(*bb);
        for (BasicBlock::iterator i = block->begin(), e = block->end(); i != e; ++i) {
          if (isa<GetElementPtrInst>(*i) && !cast<GetElementPtrInst>(i)->hasAllZeroIndices()
                         && ignorePtrs.find(&*i) == ignorePtrs.end()) {
            // get the value now before replacing i
            Value* sizeValue = ptrToSize[&*i];

            BasicBlock *after = block->splitBasicBlock(i, "baggy.split");
            BasicBlock *baggy;
            PHINode *phi;
            GetElementPtrInst *inst = cast<GetElementPtrInst>(i->clone());

            // Remove the getelementptrinst from the old block
            // i->removeFromParent();
            phi = PHINode::Create(i->getType(), 2);
            ReplaceInstWithInst(i->getParent()->getInstList(), i, phi);

            // Create the instrumentation block
            baggy = instrumentGEP(after, inst, phi, sizeValue);

            // Have control flow through the instrumentation block
            TerminatorInst *term = block->getTerminator();
            if (term == NULL) {
              block->getInstList().push_back(BranchInst::Create(baggy));
            } else {
              term->setSuccessor(0, baggy);
            }

            // Skip the newly created instrumentation basicblock
            ++bb;

            // We're done with this block
            break;
          } else if (isa<MemSetInst>(*i)) {
		    Value* sizeValue = ptrToSize[&*(i->getOperand(0))];

            BasicBlock *after = block->splitBasicBlock(i, "baggy.split");
            BasicBlock *baggy;
            MemSetInst *inst = cast<MemSetInst>(i);

            // remove the instruction from this block
            i->removeFromParent();

            // Instrument the function
            baggy = instrumentMemset(after, inst, sizeValue);

            // Have control flow through the instrumentation block
            TerminatorInst *term = block->getTerminator();
            if (term == NULL) {
              block->getInstList().push_back(BranchInst::Create(baggy));
            } else {
              term->setSuccessor(0, baggy);
            }

            // Skip the newly created instrumentation basicblock
            ++bb;

            // We're done with this block
            break;
          }
        }
      }

      return true;
    }

    virtual void getAnalysisUsage(AnalysisUsage &Info) const {
	    Info.addRequired<DataLayout>();
	    Info.addRequired<DominatorTree>();
    }
  };
}

char BaggyBoundsPointersOpt::ID = 0;
static RegisterPass<BaggyBoundsPointersOpt>
X("baggy-pointers-opt",
  "Baggy Bounds Pointer Instrumentation Pass",
  false,
  false);
