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

  // remaining functions are auxiliary
  for (unsigned i = 1; i < this->allFunc.size(); i++) {
    Function *curr = this->allFunc[i];
    // check each instruction and insert flows
    
  }
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
