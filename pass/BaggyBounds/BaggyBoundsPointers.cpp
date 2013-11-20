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
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
  struct BaggyBoundsPointers : public FunctionPass {
    static char ID;
    BaggyBoundsPointers() : FunctionPass(ID) {}
    GlobalVariable *sizeTable;
    Function *slowPath;

    BasicBlock *createBaggyBlock(BasicBlock *orig, GetElementPtrInst *i) {
      BasicBlock *baggyBlock = BasicBlock::Create(orig->getContext(), "baggy.check",
                                                  orig->getParent(), orig);
      IRBuilder<> builder(baggyBlock);
      Value *tablestart = ConstantInt::get(IntegerType::get(baggyBlock->getContext(), 32), 0x78000000);
      Value *base, *baseint, *tableoffset, *tableaddr, *hack, *inst;
      LoadInst *size;

      // Baggy lookup
      base = builder.CreateConstInBoundsGEP1_32(i->getOperand(0), 0);
      baseint = builder.CreatePtrToInt(base, IntegerType::get(baggyBlock->getContext(), 32));
      tableoffset = builder.CreateAShr(baseint, 4);
      hack = builder.CreateIntToPtr(tablestart, Type::getInt8PtrTy(baggyBlock->getContext()));
      tableaddr = builder.CreateInBoundsGEP(sizeTable, tableoffset);
      size = builder.CreateLoad(tableaddr, "alloc.size");

      // insert arithmetic
      baggyBlock->getInstList().push_back(i);

      // baggy check
      Value *combine, *instint, *result, *sizeint, *br;

      instint = builder.CreatePtrToInt(i, IntegerType::get(baggyBlock->getContext(), 32));
      combine = builder.CreateXor(baseint, instint);
      sizeint = builder.CreateZExtOrBitCast(size, IntegerType::get(baggyBlock->getContext(), 32));
      result = builder.CreateAShr(combine, sizeint);
      br = builder.CreateBr(orig);

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

            // Remove the getelementptrinst from the old block
            i->removeFromParent();

            // Create the instrumentation block
            baggy = createBaggyBlock(after, &cast<GetElementPtrInst>(*i));

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
