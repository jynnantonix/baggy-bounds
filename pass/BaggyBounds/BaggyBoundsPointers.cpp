#include "llvm/Pass.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
  struct BaggyBoundsPointers : public BasicBlockPass {
    static char ID;
    BaggyBoundsPointers() : BasicBlockPass(ID) {}

    virtual bool runOnBasicBlock(BasicBlock &BB) {
      for (BasicBlock::iterator i = BB.begin(), e = BB.end(); i != e; ++i) {
        if (isa<GetElementPtrInst>(*i)) {
          GetElementPtrInst *inst = &cast<GetElementPtrInst>(*i);
          IRBuilder<> builder(inst);
          Value *tablestart = ConstantInt::get(IntegerType::get(BB.getContext(), 32), 0x78000000);
          Value *base, *baseptr, *tableoffset, *tableaddr, *hack;
          LoadInst *size;

          base = builder.CreateConstInBoundsGEP1_32(inst->getPointerOperand(), 0);
          baseptr = builder.CreatePtrToInt(base, IntegerType::get(BB.getContext(), 32));
          tableoffset = builder.CreateAShr(baseptr, 4);
          hack = builder.CreateIntToPtr(tablestart, Type::getInt8PtrTy(BB.getContext()));
          tableaddr = builder.CreateInBoundsGEP(hack, tableoffset);
          size = builder.CreateLoad(tableaddr, "alloc_size");
       }
      }

      return false;
    }
  };
}

char BaggyBoundsPointers::ID = 0;
static RegisterPass<BaggyBoundsPointers>
X("baggy-pointers",
  "Baggy Bounds Pointer Instrumentation Pass",
  true,
  false);
