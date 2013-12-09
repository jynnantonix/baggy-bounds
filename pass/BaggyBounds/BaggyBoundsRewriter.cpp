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
  struct BaggyBoundsRewriter : public FunctionPass {
    static char ID;
    BaggyBoundsRewriter() : FunctionPass(ID) {}
    StringMap<Constant *> BaggyLibc;
    Function *StrCpy;
    Function *StrCat;

    Constant *createLibcStrFunction(Module &M, StringRef FuncName) {
      std::vector<Type *> FuncTy_2_args;
      FuncTy_2_args.push_back(Type::getInt8PtrTy(getGlobalContext()));
      FuncTy_2_args.push_back(Type::getInt8PtrTy(getGlobalContext()));

      FunctionType *FuncTy_str = FunctionType::get(Type::getInt8PtrTy(getGlobalContext()),
                                                    FuncTy_2_args, false);

      return M.getOrInsertFunction(FuncName, FuncTy_str);
    }

    virtual bool doInitialization(Module &M) {
      Constant *&BaggyStrcat = BaggyLibc["baggy_strcat"];
      if (BaggyStrcat == NULL) {
        BaggyStrcat = createLibcStrFunction(M, "baggy_strcat");
      }

      Constant *&BaggyStrcpy = BaggyLibc["baggy_strcpy"];
      if (BaggyStrcpy == NULL) {
        BaggyStrcpy = createLibcStrFunction(M, "baggy_strcpy");
      }

      StrCpy = M.getFunction("strcpy");
      StrCat = M.getFunction("strcat");
    }

    virtual bool runOnFunction(Function &F) {
      bool ret = false;
      if (StrCpy == NULL && StrCat == NULL) {
        return false;
      }

      for (Function::iterator bb = F.begin(), bbend = F.end(); bb != bbend; ++bb) {
        BasicBlock *block = &(*bb);
        for (BasicBlock::iterator i = block->begin(), e = block->end(); i != e; ++i) {
          if (!isa<CallInst>(*i)) {
            continue;
          }
          CallInst *Call = &cast<CallInst>(*i);
          if (Call->getCalledFunction() == StrCpy) {
            Call->setCalledFunction(BaggyLibc["baggy_strcpy"]);
            ret = true;
          } else if (Call->getCalledFunction() == StrCat) {
            Call->setCalledFunction(BaggyLibc["baggy_strcat"]);
            ret = true;
          }
        }
      }

      return ret;
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
