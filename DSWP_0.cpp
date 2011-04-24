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
    AU.addRequired<PostDominatorTree>();
    AU.addRequired<AliasAnalysis>();
    AU.addRequired<MemoryDependenceAnalysis>();
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
	threadPartition(L);
	showPartition(L);
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

	//add dependency within a basicblock
	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		Instruction *pre = NULL;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);
			if (pre != NULL) {
				addEdge(pre, inst, CONTROL);
			}
			pre = inst;
		}
	}


	//end control dependece

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
			dfs2(next);
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
				if (!added[u][v] && u != v) {
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DSWP::threadPartition(Loop *L) {
	//1. get total latency and latency for

	assigned.clear();
	sccLatency.clear();

	int totalLatency = 0;

	for (int i = 0; i < sccNum; i++)
		sccLatency.push_back(0);
	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
			Instruction *inst = &(*ii);
			int latency = getLatency(inst);
			sccLatency[sccId[inst]] += latency;
			totalLatency += latency;
		}
	}

	int averLatency = totalLatency / MAX_THREAD;

	cout << "latency info:" << totalLatency << " " << averLatency << endl;

	for (int i = 0; i < sccNum; i++)
		assigned.push_back(-2);
	//-2: not assigned, and not in queue
	//-1: not assigned, but in queue
	//0 ~ MAX_THREAD, already been assigned, not in queue

	int estLatency[MAX_THREAD];

	//TODO NOT SURE HERE IS RIGHT! header vs. preheader
	int start = sccId[L->getHeader()->getFirstNonPHI()];

	priority_queue<QNode> Q;
	Q.push(QNode(start, sccLatency[start]));

	for (int i = 0; i < MAX_THREAD; i++) {
		while (!Q.empty()) {
			QNode top = Q.top(); Q.pop();
			assigned[top.u] = i;
			estLatency[i] += top.latency;
			//update the list
			for (unsigned j = 0; j < dag[top.u]->size(); j++) {
				int v = dag[top.u]->at(j);
				if (assigned[v] > -2)
					continue;
				assigned[v] = -1;
				Q.push(QNode(v, sccLatency[v]));
			}
			//check load balance
			if (estLatency[i] >= averLatency) {
				//cout << estLatency[i] << endl;
				break;
			}
		}
	}
	//following is impossible since every time I let it bigger that aver
	if (!Q.empty()) {
		printf("err");
	}

	for (int i = 0; i < MAX_THREAD; i++)
		part[i].clear();

	for (int i = 0; i < sccNum; i++) {
	//	cout << i << "  " << assigned[i] << endl;
		part[assigned[i]].push_back(i);
	}
}

int DSWP::getLatency(Instruction *I) {
	int opcode = I->getOpcode();

	//copy from dswp583 google project site TODO this table is obviously not right!
	int cost;
    // These are instruction latencies for an AMD K10
    // (Well, most of them are, some I just made up)
    switch (opcode) {
        // Terminator instructions
        case 1: cost=2; break;          // Return
        case 2: cost=3; break;          // Branch
        case 3: cost=3; break;          // Switch

                // Standard binary operators
        case 8: cost=1; break;          // Add
        case 9: cost=4; break;          // FAdd
        case 10: cost=1; break;         // Sub
        case 11: cost=4; break;         // FSub
        case 12: cost=3; break;         // Mul
        case 13: cost=4; break;         // FMul
        case 14: cost=17; break;        // UDiv
        case 15: cost=17; break;        // SDiv
        case 16: cost=24; break;        // FDiv
        case 17: cost=17; break;        // URem
        case 18: cost=17; break;        // SRem
        case 19: cost=24; break;        // FRem

                 // logical operators (integer operands)
        case 20: cost=7; break;         // Shift left
        case 21: cost=7; break;         // Shift right
        case 22: cost=7; break;         // Shifr right (arithmetic)
        case 23: cost=1; break;         // And
        case 24: cost=1; break;         // Or
        case 25: cost=1; break;         // Xor

                 // Memory ops
                case 26: cost=2; break;         // Alloca
        case 27: cost=2; break;         // Load
        case 28: cost=2; break;         // Store
        case 29: cost=1; break;         // GetElementPtr

                 // Cast operators
        case 30: cost=1; break;         // Truncate integers
        case 31: cost=1; break;         // Zero extend integers
        case 32: cost=1; break;         // Sign extend integers
        case 33: cost=4; break;         // FPtoUI
        case 34: cost=4; break;         // FPtoSI
        case 35: cost=4; break;         // UItoFP
        case 36: cost=4; break;         // SItoFP
        case 37: cost=4; break;         // FPTrunc
        case 38: cost=4; break;         // FPExt
        case 39: cost=2; break;         // PtrToInt
        case 40: cost=2; break;         // IntToPtr
        case 41: cost=1; break;         // Type cast

                 // Other
        case 42: cost=1; break;         // Integer compare
        case 43: cost=1; break;         // Float compare
        case 44: cost=1; break;         // PHI node
        case 45: cost=50; break;        // Call function (this one is really variable)

        default: cost=1;
    }

    return (cost);
}

/////////////////////////////////////////////code spliting/////////////////////////////////////////////////////
void DSWP::loopSplit(Loop *L) {
//	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
//		BasicBlock *BB = *bi;
//		for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
//			Instruction *inst = &(*ii);
//		}
//	}
	BasicBlock *header = L->getHeader();
	BasicBlock *exit = L->getExitBlock();

	if (exit == NULL) {
		error("exit not unique! have to think about this");
	}

	allFunc.clear();

	//check for each partition, find relevant blocks, set could auto deduplicate
	for (int i = 0; i < MAX_THREAD; i++) {
		//create function to each each thread TODO: decide the argument of the function
		BasicBlock *header = L->getHeader();
		Constant * c = header->getParent()->getParent()->getOrInsertFunction("subloop_" + itoa(i), FunctionType::getVoidTy(header->getContext()), NULL);
		Function *func = cast<Function>(c);
		func->setCallingConv(CallingConv::C);
		allFunc.push_back(func);

		//each partition contain several scc
		map<Instruction *, bool> relinst;
		set<BasicBlock *> relbb;
		for (vector<int>::iterator ii = part[i].begin(); ii != part[i].end(); ii++) {
			int scc = *ii;
			for (vector<Instruction *>::iterator iii = InstInSCC[scc].begin(); iii != InstInSCC[scc].end(); iii++) {
				Instruction *inst = *iii;
				relinst[inst] = true;
				relbb.insert(inst->getParent());

				//add blocks which the instruction dependent on
				for (vector<Edge>::iterator ei = rev[inst]->begin(); ei != rev[inst]->end(); ei++) {
					Instruction *dep = ei->v;
					relinst[dep] = true;
					relbb.insert(dep->getParent());
				}
			}
		}

		//copy blocks
		map<BasicBlock *, BasicBlock *> BBMap;	//map the old block to new block
		for (set<BasicBlock *>::iterator bi = relbb.begin();  bi != relbb.end(); bi++) {
			BasicBlock *BB = *bi;
			BBMap[BB] = BasicBlock::Create(BB->getContext(), BB->getNameStr() + "_" + itoa(i), func);
		}

		IRBuilder<> Builder(getGlobalContext());

		//add instruction
		for (set<BasicBlock *>::iterator bi = relbb.begin();  bi != relbb.end(); bi++) {
			BasicBlock *BB = *bi;
			BasicBlock *NBB =  BBMap[BB];
			//BB->splitBasicBlock()

			for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
				Instruction * inst = ii;
				Instruction *newInst;
				if (isa<TerminatorInst>(inst)) {//copy the teminatorInst since they could be used mutiple times
					newInst = inst->clone();
				} else {
					newInst = inst;
					inst->removeFromParent();
				}

				//TODO check if there are two branch, one branch is not in the partition, then what the branch

				//instruction will be
				for (unsigned j = 0; j < newInst->getNumOperands(); j++) {
					Value *op = newInst->getOperand(j);
					if (BasicBlock *oldBB = dyn_cast<BasicBlock>(op)) {
						BasicBlock * newBB = BBMap[oldBB];
						while (newBB == NULL) {
							//TODO find the nearest post-dominator (not sure it is correct)
							oldBB = pre[oldBB];
							newBB = BBMap[oldBB];
						}

						newInst->setOperand(j, newBB);
					}
				}
				//TODO not sure this is right way to do this
				NBB->getInstList().insertAfter(NBB->getInstList().end(), newInst);
			}
		}// for add instruction

		//TODO insert function call here
	}
}

void DSWP::getLiveinfo(Loop * L) {
	//LivenessAnalysis *live = &getAnalysis<LivenessAnalysis>();

	//currently I don't want to use standard liveness analysis

	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);
			if (util.hasNewDef(inst)) {
				defin.insert(inst);
			}
		}
	}

	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);

			for (Instruction::op_iterator oi = inst->op_begin(); oi != inst->op_end(); oi++) {
				Value *op = *oi;
				if (defin.find(op) == defin.end()) {
					livein.insert(op);
				}
			}
		}
	}
	//so basically I add variables used in loop but not been declared in loop as live variable

	//I think we could assume liveout = livein + defin at first, especially I havn't understand the use of liveout
	liveout = livein;
	liveout.insert(defin.begin(), defin.end());
}

/////////////////////////////////////////////test function/////////////////////////////////////////////////////

void DSWP::showGraph(Loop *L) {
	cout << "header:" << L->getHeader()->getNameStr() << endl;
	cout << "exit:" << L->getExitBlock()->getNameStr() << endl;	//TODO check different functions related to exit
	cout << "num of blocks:" << L->getBlocks().size() << endl;


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

		ost << "SCC " << i << ":";

		vector<Instruction *> insts = this->InstInSCC[i];
		for (vector<Instruction *>::iterator ii = insts.begin(); ii != insts.end(); ii++) {
			Instruction *inst = *ii;
			ost << dname[inst] << " ";
		}
		ost << "\n";

		ost << "\tadjacent scc" << ":";

		vector<int> *edges = this->dag[i];
		for (unsigned i = 0; i < edges->size(); i++) {
			ost << edges->at(i) << " ";
		}
		ost << "\n";
	}
}

void DSWP::showPartition(Loop *L) {
	std::string name = "partition";
	ofstream file((name.c_str()));
	raw_os_ostream ost(file);

	ost << "latency: \n";
	for (int i = 0; i < sccNum; i++) {
		ost << i << " " << sccLatency[i] << "\n";
	}

	for (int i = 0; i < MAX_THREAD; i++) {
		ost << "partition " << i << ":" << "\n";
		vector<int> &nodes = part[i];

		ost << "\t";
		for(int j = 0; j < nodes.size(); j++) {
			ost << nodes[j] << " ";
		}
		ost << "\n";
	}
}


