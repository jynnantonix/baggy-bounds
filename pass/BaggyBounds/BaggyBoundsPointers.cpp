#include "llvm/Pass.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {
  struct BaggyBoundsPointers : public FunctionPass {
    static char ID;
    BaggyBoundsPointers() : FunctionPass(ID) {}
    GlobalVariable *sizeTable;
    Function *slowPath;

    BasicBlock *createBaggyBlock(BasicBlock *orig, GetElementPtrInst *i, PHINode *phi) {
      BasicBlock *baggyBlock = BasicBlock::Create(orig->getContext(), "baggy.check",
                                                  orig->getParent(), orig);
      IRBuilder<> builder(baggyBlock);
      Value *tablestart = ConstantInt::get(IntegerType::get(baggyBlock->getContext(), 32), 0x78000000);
      Value *base, *baseint, *tableoffset, *tableaddr, *hack, *inst, *sizeint, *tmpsize;
      LoadInst *size;

      // Baggy lookup
      base = builder.CreateConstInBoundsGEP1_32(i->getOperand(0), 0);
      baseint = builder.CreatePtrToInt(base, IntegerType::get(baggyBlock->getContext(), 32));
      tableoffset = builder.CreateAShr(baseint, 4);
      hack = builder.CreateIntToPtr(tablestart, Type::getInt8PtrTy(baggyBlock->getContext()));
      tableaddr = builder.CreateInBoundsGEP(hack, tableoffset);
      size = builder.CreateLoad(tableaddr, "alloc.size");
      tmpsize = builder.CreateZExtOrBitCast(size, IntegerType::get(baggyBlock->getContext(), 32));
      sizeint = builder.CreateAnd(tmpsize, 0x1F);

      // insert arithmetic
      baggyBlock->getInstList().push_back(i);

      // baggy check
      Value *combine, *instint, *result;

      instint = builder.CreatePtrToInt(i, IntegerType::get(baggyBlock->getContext(), 32));
      combine = builder.CreateXor(baseint, instint);
      result = builder.CreateAShr(combine, sizeint);

      // Create the slowpath block
      BasicBlock *slowPathBlock = BasicBlock::Create(baggyBlock->getContext(), "baggy.slowPath",
                                                     baggyBlock->getParent(), orig);
      IRBuilder<> slowPathBuilder(slowPathBlock);
      Value *slowPathBr, *slowPathPtr;

      slowPathPtr = slowPathBuilder.Insert(Constant::getNullValue(i->getType()));
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

    virtual bool doInitialization(Module &M) {
      sizeTable = M.getGlobalVariable("baggy_size_table");
    }

    virtual bool runOnFunction(Function &F) {
      for (Function::iterator bb = F.begin(), bbend = F.end(); bb != bbend; ++bb) {
        BasicBlock *block = &(*bb);
        for (BasicBlock::iterator i = block->begin(), e = block->end(); i != e; ++i) {
          if (isa<GetElementPtrInst>(*i)) {
            BasicBlock *after = block->splitBasicBlock(i, "baggy.split");
            BasicBlock *baggy;
            PHINode *phi;
            GetElementPtrInst *inst = cast<GetElementPtrInst>(i->clone());

            // Remove the getelementptrinst from the old block
            // i->removeFromParent();
            phi = PHINode::Create(i->getType(), 2);
            ReplaceInstWithInst(i->getParent()->getInstList(), i, phi);

            // Create the instrumentation block
            baggy = createBaggyBlock(after, inst, phi);

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
