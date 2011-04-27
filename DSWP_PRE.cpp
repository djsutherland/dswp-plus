/*
 * DSWP_PRE.cpp
 *
 *  Created on: Apr 27, 2011
 *      Author: fuyaoz
 */

#include "DSWP_PRE.h"

DSWP_PRE::DSWP_PRE() : ModulePass(ID) {

}

DSWP_PRE::~DSWP_PRE() {
}

bool DSWP_PRE::runOnModule(Module &M) {

	//for (Function::iterator)

	for (Module::global_iterator gi = M.global_begin(); gi != M.global_end(); gi++) {
		Value *inst = gi;
		if (inst->getType()->isFloatTy()) {

		}
	}

	return true;
}


char DSWP_PRE::ID = 0;
RegisterPass<DSWP_PRE> Z("dswp-pre", "preprocess of dswp");
