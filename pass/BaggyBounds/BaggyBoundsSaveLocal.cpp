#include <algorithm>

#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/IR/IRBuilder.h"

#include "Util.h"

using namespace llvm;
using std::max;

namespace {
    struct BaggyBoundsSaveLocal : public ModulePass {
        static char ID;
        BaggyBoundsSaveLocal() : ModulePass(ID) {}

        DataLayout* DL;

        void handleInstruction(Module& M,
                                BasicBlock::InstListType& iList,
                                BasicBlock::InstListType::iterator iiter) {
            AllocaInst* inst = cast<AllocaInst>(iiter);
            unsigned int allocation_size = DL->getTypeAllocSize(inst->getType()->getElementType());
            unsigned int real_allocation_size = get_alignment(allocation_size);
            inst->setAlignment(max(inst->getAlignment(), real_allocation_size));
            // Check if the pointer can "escape" the function.
            // Second and third arguments are if returning counts as escaping
            // and if storing counts as escaping, respectively. We say yes
            // to both. Of course, it is undefined behaviour to return a pointer
            // to this stack-allocated object, so that argument should be irrelevant.
            //if (PointerMayBeCaptured(inst, true, true)) {
            	// Cast the pointer to an int and then add a save instruction.
				CastInst* intPtr = new PtrToIntInst(inst, IntegerType::get(M.getContext(), 32), "");
                Instruction* saveInst = get_save_in_table_instr(M, intPtr, real_allocation_size);
				iList.insertAfter(iiter, saveInst);
				iList.insertAfter(iiter, intPtr);
            //}
        }

        virtual bool runOnModule(Module& M) {
            DL = &getAnalysis<DataLayout>();

			// Iterate through all functions, basic blocks, and instructions, looking for
			// alloca instructions. Call handleInstruction on all alloca instructions.
            for (Module::iterator miter = M.begin(); miter != M.end(); ++miter) {
                Function& F = *miter;
                for (Function::iterator bbiter = F.begin(); bbiter != F.end(); ++bbiter) {
                    BasicBlock& bb = *bbiter;
                    BasicBlock::InstListType& iList = bb.getInstList();
                    for (BasicBlock::InstListType::iterator iiter = iList.begin();
                            iiter != iList.end(); ++iiter) {
                        Value const* inst = iiter;
                        if (isa<AllocaInst>(inst)) {
                            handleInstruction(M, iList, iiter);
                        }
                    }
                }
            }
        }

		virtual void getAnalysisUsage(AnalysisUsage &Info) const {
			Info.addRequired<DataLayout>();
		}
    };
}

char BaggyBoundsSaveLocal::ID = 0;
static RegisterPass<BaggyBoundsSaveLocal>
    X("baggy-save-local", "Baggy Bounds Locals Initialization Pass",
        true,
        false);
