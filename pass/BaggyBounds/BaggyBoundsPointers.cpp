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

using namespace llvm;

namespace {
  struct BaggyBoundsPointers : public FunctionPass {
    static char ID;
    BaggyBoundsPointers() : FunctionPass(ID) {}
    Constant *sizeTable;
    Constant  *slowPathFunc;

    BasicBlock *instrumentMemset(BasicBlock *orig, MemSetInst *i) {
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
      Value *TableOffset = builder.CreateLShr(BaseInt, 4, "baggy.offset");
      LoadInst *SizeTablePtr = builder.CreateLoad(sizeTable, "baggy.table");
      Value *Tableaddr = builder.CreateInBoundsGEP(SizeTablePtr, TableOffset);
      LoadInst *Size = builder.CreateLoad(Tableaddr, "alloc.size");
      Value *MaskedSize = builder.CreateZExtOrBitCast(Size, Type::getInt32Ty(baggyBlock->getContext()));
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

    BasicBlock *instrumentGEP(BasicBlock *orig, GetElementPtrInst *i, PHINode *phi) {
      BasicBlock *baggyBlock = BasicBlock::Create(orig->getContext(), "baggy.check",
                                                  orig->getParent(), orig);
      IRBuilder<> builder(baggyBlock);
      Value *base, *baseint, *tableoffset, *tableaddr, *inst, *sizeint, *tmpsize, *sizeTableAddr;
      LoadInst *size, *sizeTablePtr;

      // Baggy lookup
      base = builder.CreateConstInBoundsGEP1_32(i->getOperand(0), 0, "baggy.base");
      baseint = builder.CreatePtrToInt(base, IntegerType::get(baggyBlock->getContext(), 32));
      tableoffset = builder.CreateLShr(baseint, 4, "baggy.offset");
      //sizeTableAddr = builder.CreateConstInBoundsGEP1_32(sizeTable, 0);
      sizeTablePtr = builder.CreateLoad(sizeTable, "baggy.table");
      tableaddr = builder.CreateInBoundsGEP(sizeTablePtr, tableoffset);
      size = builder.CreateLoad(tableaddr, "alloc.size");
      tmpsize = builder.CreateZExtOrBitCast(size, IntegerType::get(baggyBlock->getContext(), 32));
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

    virtual bool doInitialization(Module &M) {
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
    }

    virtual bool runOnFunction(Function &F) {
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

      for (Function::iterator bb = F.begin(), bbend = F.end(); bb != bbend; ++bb) {
        BasicBlock *block = &(*bb);
        for (BasicBlock::iterator i = block->begin(), e = block->end(); i != e; ++i) {
          if (isa<GetElementPtrInst>(*i) && !cast<GetElementPtrInst>(i)->hasAllZeroIndices()) {
            BasicBlock *after = block->splitBasicBlock(i, "baggy.split");
            BasicBlock *baggy;
            PHINode *phi;
            GetElementPtrInst *inst = cast<GetElementPtrInst>(i->clone());

            // Remove the getelementptrinst from the old block
            // i->removeFromParent();
            phi = PHINode::Create(i->getType(), 2);
            ReplaceInstWithInst(i->getParent()->getInstList(), i, phi);

            // Create the instrumentation block
            baggy = instrumentGEP(after, inst, phi);

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
            BasicBlock *after = block->splitBasicBlock(i, "baggy.split");
            BasicBlock *baggy;
            MemSetInst *inst = cast<MemSetInst>(i);

            // remove the instruction from this block
            i->removeFromParent();

            // Instrument the function
            baggy = instrumentMemset(after, inst);

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
    }
  };
}

char BaggyBoundsPointers::ID = 0;
static RegisterPass<BaggyBoundsPointers>
X("baggy-pointers",
  "Baggy Bounds Pointer Instrumentation Pass",
  true,
  false);
