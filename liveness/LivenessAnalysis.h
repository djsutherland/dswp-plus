#ifndef LIVE_H_
#define LIVE_H_

#include "DFAFramework.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_os_ostream.h"
#include "Utils.h"
#include <vector>
#include <iostream>
#include <ostream>
#include <fstream>
#include <map>
#include <list>

using namespace std;
using namespace llvm;

class LivenessAnalysis: public FunctionPass, public DFAFramework {
private:
	vector<Value *> allDef;
	map<Value*, int> defIndex;
public:
	static char ID;

	LivenessAnalysis();

	~LivenessAnalysis();

	void db();

	void setDefinitionVector(vector<Value *> allDef);

	virtual bool runOnFunction(Function &F);

	virtual DFAValue transfer(Instruction *isn, DFAValue value);

	virtual void initialize(Function *fun);

	virtual DFAValue meet(DFAValue a, DFAValue b);

	virtual bool isFoward();

	bool isVaribleLiveIn(Value * var, Value* position);

	bool isVaribleLiveOut(Value * var, Value* position);
};


#endif
