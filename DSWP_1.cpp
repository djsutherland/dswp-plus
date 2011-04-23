//1st step: create dependence graph

#include "DSWP.h"

using namespace llvm;
using namespace std;

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
	Function *fun = L->getHeader()->getParent();
	for (Function::iterator bi = fun->begin(); bi != fun->end(); bi++) {
		BasicBlock *BB = bi;
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
