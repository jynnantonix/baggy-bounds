#include "llvm/Pass.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {
  struct BaggyBoundsRewriter : public ModulePass {
    static char ID;
    BaggyBoundsRewriter() : ModulePass(ID) {}

   virtual bool runOnModule(Module &M) {
      Function *StrCpy = M.getFunction("strcpy");
      if (StrCpy != NULL) {
        Constant *BaggyStrCpy = M.getOrInsertFunction("baggy_strcpy", StrCpy->getFunctionType());
        StrCpy->replaceAllUsesWith(BaggyStrCpy);
      }

      Function *StrCat = M.getFunction("strcat");
      if (StrCat != NULL) {
        Constant *BaggyStrCat = M.getOrInsertFunction("baggy_strcat", StrCat->getFunctionType());
        StrCat->replaceAllUsesWith(BaggyStrCat);
      }

      Function *Sprintf = M.getFunction("sprintf");
      if (Sprintf != NULL) {
        Constant *BaggySprintf = M.getOrInsertFunction("baggy_sprintf", Sprintf->getFunctionType());
        Sprintf->replaceAllUsesWith(BaggySprintf);
      }
    }

    virtual void getAnalysisUsage(AnalysisUsage &Info) const {
    }
  };
}

char BaggyBoundsRewriter::ID = 0;
static RegisterPass<BaggyBoundsRewriter>
X("baggy-rewriter",
  "Rewrite Libc calls to use Baggy Bounds wrappers",
  true,
  false);
