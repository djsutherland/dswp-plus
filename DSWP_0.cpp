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

bool DSWP::doInitialization(Loop *, LPPassManager &LPM) {
//	cout << "can you see that?" << endl;
	return true;
}

void DSWP::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<LoopInfo>();
    AU.addRequired<DominatorTree>();
    AU.addRequired<PostDominatorTree>();
    AU.addRequired<AliasAnalysis>();
    AU.addRequired<MemoryDependenceAnalysis>();
  //  AU.addRequired<LivenessAnalysis>();
}

void DSWP::initilize(Loop *L) {

}

bool DSWP::runOnLoop(Loop *L, LPPassManager &LPM) {
	if (L->getLoopDepth() != 1)	//ONLY care about top level loops
    	return false;

	cout << "we are running on a loop" << endl;

	buildPDG(L);
	showGraph(L);
	findSCC(L);
	buildDAG(L);
	showDAG(L);
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

	cout << a->getNameStr() << " " << b->getNameStr() << " " << lca->getNameStr() << endl;

	if (lca == pre[a]) {	//case 1
		BasicBlock *BB = b;
		while (BB != lca) {
			addControlDependence(a, BB);
			BB = pre[BB];
		}
	} else if (lca == a) {	//case 2: capture loop dependence
		BasicBlock *BB = b;
		while (BB != pre[a]) {
			cout << "\t" << a->getNameStr() << " " << BB->getNameStr() << endl;
			addControlDependence(a, BB);
			BB = pre[BB];
		}
	} else {
		error("unknow case in checkControlDependence!");
	}
}

void DSWP::buildPDG(Loop *L) {
    //Initialize PDG
	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);

			//standardlize the name for all expr
			if (util.hasNewDef(inst)) {
				inst->setName(util.genId());
				dname[inst] = inst->getNameStr();
			} else {
				dname[inst] = util.genId();
			}

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

	//cout << pdt.getRootNode()->getBlock()->getNameStr() << endl;
	//for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
	Function *fun = L->getHeader()->getParent();
	for (Function::iterator bi = fun->begin(); bi != fun->end(); bi++) {
		BasicBlock *BB = bi;
		DomTreeNode *dn = pdt.getNode(BB);

		for (DomTreeNode::iterator di = dn->begin(); di != dn->end(); di++) {
			BasicBlock *CB = (*di)->getBlock();
			pre[CB] = BB;
		}
	}

	//TODO add dependency within a basicblock


	//edn control dependece

	//TODO check edge exist
	//TODO the special kind of dependence need loop peeling ? I don't know whether this is needed
	//TODO consider how to deal with phi node, now there is no phi node
	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (succ_iterator PI = succ_begin(BB); PI != succ_end(BB); ++PI) {
			BasicBlock *succ = *PI;

			checkControlDependence(BB, succ, pdt);
		}
	}
	//end control dependence
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DSWP::findSCC(Loop *L) {
	used.clear();

	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);
			if (!used[inst]) {
				dfs1(inst);
			}
		}
	}

	used.clear();
	sccNum = 0;

	for (int i = list.size() - 1; i >= 0; i --) {
		Instruction *inst = list[i];
		if (!used[inst]) {
			dfs2(inst);
			sccNum++;
		}
	}
}

void DSWP::dfs1(Instruction *I) {
	used[I] = true;
	for (vector<Edge>::iterator ei = pdg[I]->begin(); ei != pdg[I]->end(); ei++) {
		Instruction *next = ei->v;
		if (!used[next])
			dfs1(next);
	}
	list.push_back(I);
}

void DSWP::dfs2(Instruction *I) {
	used[I] = true;
	for (vector<Edge>::iterator ei = rev[I]->begin(); ei != rev[I]->end(); ei++) {
		Instruction *next = ei->v;
		if (!used[next])
			dfs1(next);
	}
	sccId[I] = sccNum;
}

void DSWP::buildDAG(Loop *L) {

	for (int i = 0; i < sccNum; i++) {
		dag[i] = new vector<int>();
	}

	map<int, map<int, bool> > added;

	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *I = &(*ui);
			for (vector<Edge>::iterator ei = pdg[I]->begin(); ei != pdg[I]->end(); ei++) {
				Instruction *next = ei->v;

				int u = sccId[I];
				int v = sccId[next];

				//it is possible the edge has already been added
				if (!added[u][v]) {
					dag[u]->push_back(v);
					added[u][v] = true;
				}
			}
		}
	}

	//store InstInSCC, vector store all the inst for a scc
	InstInSCC.clear();
	for (int i = 0; i < sccNum; i++)
		InstInSCC.push_back(vector<Instruction *>());

	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *I = &(*ui);
			InstInSCC[sccId[I]].push_back(I);
		}
	}
}


/////////////////////////////////////////////test function/////////////////////////////////////////////////////

void DSWP::showGraph(Loop *L) {
	cout << L->getHeader()->getNameStr() << endl;
	cout << L->getBlocks().size() << endl;


	std::string name = "showgraph";
	ofstream file((name.c_str()));
	raw_os_ostream ost(file);

	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);
			vector<Edge> * edges = pdg[inst];

			ost << dname[inst] << " " << *inst << ":\n";
			ost << "\t";
			for (vector<Edge>::iterator ei = edges->begin(); ei != edges->end(); ei ++) {
				ost << dname[(ei->v)] << "[" << ei->dtype << "]\t";
			}
			ost << "\n\n";
		}
	}
}

void DSWP::showDAG(Loop *L) {
	std::string name = "dag";
	ofstream file((name.c_str()));
	raw_os_ostream ost(file);

	ost << "num:" << sccNum << "\n";

	for (int i = 0; i < sccNum; i++) {

		ost << "instruction in SCC " << i << ":";

		vector<Instruction *> insts = this->InstInSCC[i];
		for (vector<Instruction *>::iterator ii = insts.begin(); ii != insts.end(); ii++) {
			Instruction *inst = *ii;
			ost << dname[inst] << " ";
		}
		ost << "\n";

		ost << "adjacent scc" << ":";

		vector<int> *edges = this->dag[i];
		for (unsigned i = 0; i < edges->size(); i++) {
			ost << edges->at(i) << " ";
		}
		ost << "\n";
	}


}


