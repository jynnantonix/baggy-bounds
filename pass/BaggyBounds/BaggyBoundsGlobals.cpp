#include <vector>

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

#include "Util.h"

using namespace llvm;
using std::max;

namespace {
    struct BaggyBoundsGlobals : public ModulePass {
        static char ID;
        BaggyBoundsGlobals() : ModulePass(ID) {}

        virtual bool runOnModule(Module &m) {
            DataLayout* DL = &getAnalysis<DataLayout>();

            // Construct the function to add to global_ctors
            Constant* c = m.getOrInsertFunction("baggy.module_init",
                Type::getVoidTy(getGlobalContext()), // return type
                NULL); // signals end of argument type list
            Function* ctor_func = cast<Function>(c);
            BasicBlock* entry = BasicBlock::Create(getGlobalContext(), "entry", ctor_func);
            IRBuilder<> builder(entry);

			// Loop through global variables
            for (Module::global_iterator iter = m.global_begin(); iter != m.global_end(); ++iter) {
                if (iter->hasInitializer()) {
                	// Set the alignment on the global variable
                    unsigned int allocation_size = DL->getTypeAllocSize(iter->getType()->getElementType());
                    unsigned int real_allocation_size = get_alignment(allocation_size);
                    iter->setAlignment(max(iter->getAlignment(), real_allocation_size));

					// Add an instruction to update the size_table with the global variable's size
					Constant* location = ConstantExpr::getCast(Instruction::PtrToInt, iter, IntegerType::get(m.getContext(), 32));
					builder.Insert(get_save_in_table_instr(m, location, real_allocation_size));
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
    X("baggy-globals", "Baggy Bounds Globals Initializaiton Pass",
        false /* Only looks at CFG */,
        false /* Analysis Pass */);
