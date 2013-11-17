#ifndef UTIL_H
#define UTIL_H

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"

unsigned int get_alignment(unsigned int sz);
unsigned int get_lg(unsigned int sz);
llvm::Instruction* get_save_in_table_instr(llvm::Module& m,
		llvm::Value* location, unsigned int allocation_size);

#endif
