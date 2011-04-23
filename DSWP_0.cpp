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
  //  AU.addRequired<LivenessAnalysis>();
}

bool DSWP::runOnLoop(Loop *L, LPPassManager &LPM) {
	if (L->getLoopDepth() != 1)	//ONLY care about top level loops
    	return false;

	cout << "we are running on a loop" << endl;

	buildPDG(L);
	showGraph(L);
	//findSCC(L);
	//buildDAG(L);
	//threadPartition(L);
	//loopSplit(L);
	return true;
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

//////////////////////////////////////////////////////////////////////////////////////////


void DSWP::addControlDependence(BasicBlock *a, BasicBlock *b) {
	//TODO: dinstingush the loop carried

	Instruction *u = &a->getInstList().back();
	Instruction *v = b->getFirstNonPHI();
	addEdge(u, v, CONTROL);
}

void DSWP::checkControlDependence(BasicBlock *a, BasicBlock *b, PostDominatorTree &pdt) {
	BasicBlock *lca = pdt.findNearestCommonDominator(a, b);

	if (lca == pre[a]) {	//case 1
		BasicBlock *BB = b;
		while (BB != lca) {
			addControlDependence(a, BB);
			BB = pre[BB];
		}
	}

	if (lca == a) {	//case 2
		BasicBlock *BB = b;
		while (BB != pre[a]) {
			addControlDependence(a, BB);
			BB = pre[BB];
		}
	}
}

void DSWP::buildPDG(Loop *L) {
    //Initialize PDG
	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);
		//	if (!inst->hasName()) {			//insert names for each expr to ensure
		//		inst->setName(util.genId());
		//	}

			pdg[inst] = new vector<Edge>();
			rev[inst] = new vector<Edge>();
		}
	}

	//LoopInfo &li = getAnalysis<LoopInfo>();
	PostDominatorTree &pdt = getAnalysis<PostDominatorTree>();
	MemoryDependenceAnalysis &mda = getAnalysis<MemoryDependenceAnalysis>();

	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
			Instruction *inst = &(*ii);

			//data dependence = register dependence + memory dependence

			//begin register dependence
			for (Value::use_iterator ui = ii->use_begin(); ui != ii->use_end(); ui++) {
				if (Instruction *user = dyn_cast<Instruction>(*ui)) {
					addEdge(inst, user, REG);
				}
			}
			//finish register dependence

			//begin memory dependence
			MemDepResult mdr = mda.getDependency(inst);
			//TODO not sure clobbers mean!!

			if (mdr.isDef()) {
				Instruction *dep = mdr.getInst();

				if (isa<LoadInst>(inst)) {
					if (isa<StoreInst>(dep)) {
						addEdge(dep, inst, DTRUE);	//READ AFTER WRITE
					}
				}
				if (isa<StoreInst>(inst)) {
					if (isa<LoadInst>(dep)) {
						addEdge(dep, inst, DANTI);	//WRITE AFTER READ
					}
					if (isa<StoreInst>(dep)) {
						addEdge(dep, inst, DOUT);	//WRITE AFTER WRITE
					}
				}
			}
			//end memory dependence
		}//for ii
	}//for bi


	//begin control dependence
	//initialize pre
	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		DomTreeNode *dn = pdt.getNode(BB);

		for (DomTreeNode::iterator di = dn->begin(); di != dn->end(); di++) {
			BasicBlock *CB = (*di)->getBlock();	//TODO NOT SURE HERE IS RIGTH
			pre[CB] = BB;
		}
	}

	//TODO check edge exist
	//TODO the special kind of dependence need loop peeling ? I don't know whether this is needed
	//TODO consider how to deal with phi node
	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (succ_iterator PI = succ_begin(BB); PI != succ_end(BB); ++PI) {
			BasicBlock *succ = *PI;
			checkControlDependence(BB, succ, pdt);
		}
	}
	//end control dependence
}

/////////////////////////////////////////////test function/////////////////////////////////////////////////////

void DSWP::showGraph(Loop *L) {
	std::string name = "showgraph";
	ofstream file((name.c_str()));
	raw_os_ostream ost(file);

	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);
			vector<Edge> * edges = pdg[inst];

			ost << *inst << ":\n";
			ost << "\t";
			for (vector<Edge>::iterator ei = edges->begin(); ei != edges->end(); ei ++) {
				ost << *(ei->v) << "[" << ei->dtype << "]\t";
			}
			ost << "\n\n";
		}
	}
}

bool DSWP::doInitialization(Loop *, LPPassManager &LPM) {
//	cout << "can you see that?" << endl;
	return true;
}
