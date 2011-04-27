/*
 * DSWP_PRE.h
 *
 *  Created on: Apr 27, 2011
 *      Author: fuyaoz
 */

#ifndef DSWP_PRE_H_
#define DSWP_PRE_H_

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/Value.h"
#include "llvm/Support/CFG.h"
#include <vector>
#include <iostream>
#include <ostream>
#include <fstream>
#include <map>
#include <list>

using namespace std;
using namespace llvm;

class DSWP_PRE : public ModulePass {
public:
	static char ID;
	DSWP_PRE();
	virtual ~DSWP_PRE();
	virtual bool runOnModule(Module &M);
};

#endif /* DSWP_PRE_H_ */
