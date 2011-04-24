// 5th step - inserting thread synchronization

#include "DSWP.h"

using namespace llvm;
using namespace std;

void DSWP::insertSynchronization(Loop *L) {
	this->insertSynDependecy(L);
	// main function is allFunc[0]
	Function *main = this->allFunc[0];
	// insert initialization code (e.g. send the function
	// pointers to the waiting threads)

	for (unsigned int i = 0; i < allEdges.size(); i++) {
		Edge e = allEdges[i];
		int t1 = assigned[sccId[e.u]];
		int t2 = assigned[sccId[e.v]];
		if (t1 == t2)	//they are in same thread, therefore no need to syn
			continue;

		if (isa<TerminatorInst>(e.v)) {	//so e.v is a  branch that could be copy into many thread
			vector<Value*> termList = termMap[e.v];
			for (vector<Value*>::iterator vi = termList.begin(); vi != termList.end(); vi++) {
				Value *vv = *vi;
				//insert produce after e.u
				//insert consume before vv
			}
		} else {
			//insert produce after e.u
			//insert consume before e.v
		}
	}

//	// remaining functions are auxiliary
//	for (unsigned i = 1; i < this->allFunc.size(); i++) {
//		Function *curr = this->allFunc[i];
//		// check each instruction and insert flows
//	}
}

void DSWP::insertSynDependecy(Loop *L) {
	MemoryDependenceAnalysis &mda = getAnalysis<MemoryDependenceAnalysis>();

	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
			Instruction *inst = &(*ii);

			MemDepResult mdr = mda.getDependency(inst);

			if (mdr.isDef()) {
				Instruction *dep = mdr.getInst();
				if (isa<LoadInst> (inst)) {
					if (isa<LoadInst> (dep)) {
						addEdge(dep, inst, DSYN); //READ AFTER READ
					}
				}
			}
		}
	}
}
