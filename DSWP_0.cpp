//include the public part of DSWP
#include "DSWP.h"

using namespace llvm;
using namespace std;

Edge::Edge(Instruction *u, Instruction *v, DType dtype) {
	this->u = u;
	this->v = v;
	this->dtype = dtype;
}

char DSWP::ID = 0;
RegisterPass<DSWP> X("dswp", "15745 Decoupled Software Pipeline");

DSWP::DSWP() : LoopPass (ID){
}

void DSWP::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<LoopInfo>();
    AU.addRequired<DominatorTree>();
    AU.addRequired<PostDominatorTree>();
    AU.addRequired<AliasAnalysis>();
    AU.addRequired<MemoryDependenceAnalysis>();
    AU.addRequired<LivenessAnalysis>();
}

bool DSWP::runOnLoop(Loop *L, LPPassManager &LPM) {
    if (L->getLoopDepth() != 1)	//ONLY care about top level loops
    	return false;
	buildPDG(L);
	findSCC(L);
	buildDAG(L);
	threadPartition(L);
	return false;
}


void DSWP::addEdge(Instruction *u, Instruction *v, DType dtype) {
	pdg[u]->push_back(Edge(u, v, dtype));
	rev[v]->push_back(Edge(v, u, dtype));
}

bool DSWP::checkEdge(Instruction *u, Instruction *v) {

	for (vector<Edge>::iterator it = pdg[u]->begin(); it != pdg[v]->end(); it++) {
		if (it->v == v) {
			//TODO report something
			return true;
		}
	}
	return false;
}

