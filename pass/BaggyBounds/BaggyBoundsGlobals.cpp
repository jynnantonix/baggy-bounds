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

using namespace llvm;

namespace {

	// Given the size of an allocation, return the desired aligment.
	// Finds the smallest power of 2 which is at least as large as sz
	// but uses the SLOT_SIZE if that is too low.
	unsigned int get_alignment(unsigned int sz) {
		while ((sz & (-sz)) != sz) {
			sz += sz & (-sz);
		}
		return sz < 16 ? 16 : sz;
	}

	// returns the log of a power of 2
	unsigned int get_lg(unsigned int sz) {
		unsigned int c = 0;
		while (sz) {
			c++;
			sz >>= 1;
		}
		return c;
	}

	// Returns a
	// call void baggy_save_in_table
	// instruction (with arguments)
	Instruction* get_save_in_table_instr(Module& m, Constant* location, unsigned int allocation_size) {
		Function* func = m.getFunction("baggy_save_in_table");
		if (!func) {
			// auto-generated function type FuncTy_bsit (type of baggy_save_in_table function)
			std::vector<Type*>FuncTy_4_args;
			 FuncTy_4_args.push_back(IntegerType::get(m.getContext(), 32));
			 FuncTy_4_args.push_back(IntegerType::get(m.getContext(), 32));
			 FunctionType* FuncTy_bsit = FunctionType::get(
			  /*Result=*/Type::getVoidTy(m.getContext()),
			  /*Params=*/FuncTy_4_args,
			  /*isVarArg=*/false);

			func = Function::Create(FuncTy_bsit, GlobalValue::ExternalLinkage, "baggy_save_in_table", &m);
		}

		Constant* loc_param = ConstantExpr::getCast(Instruction::PtrToInt, location, IntegerType::get(m.getContext(), 32));
		ConstantInt* sz_param = ConstantInt::get(m.getContext(), APInt(32, get_lg(allocation_size)));
		std::vector<Value*> params;
		params.push_back(loc_param);
		params.push_back(sz_param);
		Instruction* inst = CallInst::Create(func, params, "");

		return inst;
	}

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

			// Loop through global variables
            for (Module::global_iterator iter = m.global_begin(); iter != m.global_end(); ++iter) {
                if (iter->hasInitializer()) {
                	// Set the alignment on the global variable
                    unsigned int allocation_size = DL->getTypeAllocSize(iter->getType()->getElementType());
                    unsigned int real_allocation_size = get_alignment(allocation_size);
                    iter->setAlignment(real_allocation_size);

					// Add an instruction to update the size_table with the global variable's size
					builder.Insert(get_save_in_table_instr(m, iter, real_allocation_size));
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
