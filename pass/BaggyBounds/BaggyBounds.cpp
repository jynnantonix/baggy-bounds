#include "llvm/Pass.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/InstIterator.h"

using namespace llvm;

// Given the size of an allocation, return the desired aligment.
// Finds the smallest power of 2 which is at least as large as sz
// but uses the SLOT_SIZE if that is too low.
unsigned int get_alignment(unsigned int sz) {
    while ((sz & (-sz)) != sz) {
        sz += sz & (-sz);
    }
    return sz < 16 ? 16 : sz;
}

namespace {
    struct BaggyBoundsGlobals : public ModulePass {
        static char ID;
        BaggyBoundsGlobals() : ModulePass(ID) {}

        virtual bool runOnModule(Module &m) {
            DataLayout* DL = &getAnalysis<DataLayout>();

            // Add some symbols that we need
            Value* baggy_init = cast<Value>(m.getOrInsertFunction("baggy_init",
                    Type::getVoidTy(getGlobalContext()),
                    NULL));

            // Construct the function to add to global_ctors
            Constant* c = m.getOrInsertFunction("baggy.module_init",
                Type::getVoidTy(getGlobalContext()), // return type
                NULL); // signals end of argument type list
            Function* ctor_func = cast<Function>(c);
            BasicBlock* entry = BasicBlock::Create(getGlobalContext(), "entry", ctor_func);
            IRBuilder<> builder(entry);
            builder.CreateCall(baggy_init);

            for (Module::global_iterator iter = m.global_begin(); iter != m.global_end(); ++iter) {
                if (iter->hasInitializer()) {
                    unsigned int allocation_size = DL->getTypeAllocSize(iter->getType()->getElementType());
                    unsigned int alignment = get_alignment(allocation_size);
                    iter->setAlignment(alignment);
                }
            }

			// end the basis block with a ret statement
            builder.CreateRetVoid();

            // add the function to global_ctors
            appendToGlobalCtors(m, ctor_func, 0);

            return true;
        }

		virtual void getAnalysisUsage(AnalysisUsage &Info) const {
			Info.addRequired<DataLayout>();
		}
    };
}

char BaggyBoundsGlobals::ID = 0;
static RegisterPass<BaggyBoundsGlobals>
    X("baggy", "Baggy Bounds Pass",
        false /* Only looks at CFG */,
        false /* Analysis Pass */);
