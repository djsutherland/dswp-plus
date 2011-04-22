/*
 * DFAFramework.h
 *
 *  Created on: Jan 29, 2011
 *      Author: Fuyao Zhao, Mark Hahnenberg
 */

#ifndef DFAFRAMEWORK_H_
#define DFAFRAMEWORK_H_


#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/Value.h"
#include "llvm/Support/CFG.h"
#include "DFAValue.h"
#include <map>

using namespace llvm;
using namespace std;

class DFAFramework{
private:
	/*
	 * Transfer Function for Basic block
	 */
	DFAValue transfer(BasicBlock *bb, DFAValue value);

	/*
	 * do meet operator for block, according if it is forward
	 */
	DFAValue meetAll(BasicBlock *bb);

protected:
	//store the value for in and out, value* could be either instruction * or block *
	map<Value *, DFAValue> in;
	map<Value *, DFAValue> out;
public:

	//DFAFramework(bool foward, DFAValue boundary, DFAValue interior);
	~DFAFramework();

	/*
	 * get the in set
	 */
	map<Value *, DFAValue> &getIn();

	/*
	 * get the out set
	 */
	map<Value *, DFAValue> &getOut();

	/*
	 * solve the data flow problem
	 * return a map which maps every value (instruction or basic block to DFAValue)
	 */
	map<Value *, DFAValue> &solve(Function *fun);

	/*
	 * Transfer Function for Instruction level, must be overridden
	 */
	virtual DFAValue transfer(Instruction *isn, DFAValue value) = 0;

	/*
	 * Meet Operator in Data Flow Analysis, must be overridden
	 */
	virtual DFAValue meet(DFAValue a, DFAValue b) = 0;

	/*
	 * Initialize the value, including dealing with the phi node
	 */
	virtual void initialize(Function *fun) = 0;

	/*
	 * tell the framework whether the problem is a forward problem or backward problem
	 */
	virtual bool isFoward() = 0;
};

#endif /* DFAFRAMEWORK_H_ */
