/*
 * DFAFramework.cpp
 *
 *  Created on: Jan 29, 2011
 *      Author: Fuyao Zhao, Mark Hahnenberg
 */

#include "DFAFramework.h"
using namespace llvm;

DFAFramework::~DFAFramework() {
}

DFAValue DFAFramework::transfer(BasicBlock *bb, DFAValue value) {
	DFAValue tmp = value;
	if (isFoward()) {
		for (BasicBlock::InstListType::iterator it = bb->getInstList().begin(); it
				!= bb->getInstList().end(); it++) {
			in[it] = tmp;
			tmp = transfer(it, tmp);
			out[it] = tmp;
		}
	} else {
		for (BasicBlock::InstListType::reverse_iterator it =
				bb->getInstList().rbegin(); it != bb->getInstList().rend(); it++) {
			Instruction *p = &*it;	 //a bit tricky here to do &(*it), * is a overridden operator for reverse_iterator
			out[p] = tmp;
			tmp = transfer(p, tmp);
			in[p] = tmp;
		}

	}
	return tmp;
}

DFAValue DFAFramework::meetAll(BasicBlock *BB) {
	DFAValue val;
	if (isFoward()) {	//if the problem is a forward problem
		val = in[BB];
		for (pred_iterator PI = pred_begin(BB); PI != pred_end(BB); ++PI) {
			BasicBlock *pred = *PI;
			val = meet(val, out[pred]);
		}
	} else {			//if the problem is backward problem
		val = out[BB];
		for (succ_iterator PI = succ_begin(BB); PI != succ_end(BB); ++PI) {
			BasicBlock *succ = *PI;
			val = meet(val, in[succ]);
		}
	}
	return val;
}

map<Value *, DFAValue> & DFAFramework::solve(Function *fun) {
	initialize(fun);

	if (isFoward()) {
		while (1) {
			bool change = false;
			for (Function::iterator B = fun->getBasicBlockList().begin(); B
					!= fun->getBasicBlockList().end(); B++) { // use naive order now, todo use better order
				in[B] = meetAll(B);
				DFAValue res = transfer(B, in[B]);
				if (res != out[B]) 	//check if anything change
					change = true;
				out[B] = res;
			}
			if (!change)
				break;
		}
		return out;
	} else {
		while (1) {
			bool change = false;
			for (Function::iterator B = fun->getBasicBlockList().begin(); B
					!= fun->getBasicBlockList().end(); B++) { // use naive order now, todo use better order
				out[B] = meetAll(B);
				DFAValue res = transfer(B, out[B]);
				if (res != in[B])	//check if anything change
					change = true;
				in[B] = res;
			}
			if (!change)
				break;
		}
		return in;
	}
}


map<Value *, DFAValue> & DFAFramework::getIn() {
	return in;
}

map<Value *, DFAValue> & DFAFramework::getOut() {
	return out;
}
