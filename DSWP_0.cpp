//include the public part of DSWP
#include "DSWP.h"

using namespace llvm;
using namespace std;

QNode::QNode(int u, int latency) {
	this->u = u;
	this->latency = latency;
}

bool QNode::operator < (const QNode &y) const {
	return latency < y.latency;
}

Edge::Edge(Instruction *u, Instruction *v, DType dtype) {
	this->u = u;
	this->v = v;
	this->dtype = dtype;
}

char DSWP::ID = 0;
RegisterPass<DSWP> X("dswp", "15745 Decoupled Software Pipeline");

DSWP::DSWP() : LoopPass (ID){
}

bool DSWP::doInitialization(Loop *L, LPPassManager &LPM) {
	header = L->getHeader();
	exit = L->getExitBlock();
	func = header->getParent();
	module = func->getParent();
	context = &module->getContext();

	if (exit == NULL) {
		error("exit not unique!");
	}

	//TODO insert external function declare

	Function * produce = module->getFunction("sync_produce");
	if (produce == NULL) {	//the first time, we need to link them
		//add sync_produce function
		vector<const Type *> produce_arg;
		produce_arg.push_back(Type::getInt8PtrTy(*context));
		produce_arg.push_back(Type::getInt32Ty(*context));
		FunctionType *produce_ft = FunctionType::get(Type::getVoidTy(*context), produce_arg, false);
		produce = Function::Create(produce_ft, Function::ExternalLinkage, "sync_produce", module);

		//add syn_consume function
		vector<const Type *> consume_arg;
		consume_arg.push_back(Type::getInt32Ty(*context));
		FunctionType *consume_ft = FunctionType::get(Type::getInt8PtrTy(*context), consume_arg, false);
		Function *consume = Function::Create(consume_ft, Function::ExternalLinkage, "sync_consume", module);

		//add sync_init
		FunctionType *init_ft = FunctionType::get(Type::getVoidTy(*context), false);
		Function *init = Function::Create(init_ft, Function::ExternalLinkage, "sync_init", module);

		//add jump
		vector<const Type *> jump_arg;
		jump_arg.push_back(Type::getInt8PtrTy(*context));
		FunctionType *jump_ft = FunctionType::get(Type::getInt8PtrTy(*context), jump_arg, false);
		Function *jump = Function::Create(jump_ft, Function::ExternalLinkage, "jump_to_func", module);
	}
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
	loopSplit(L);
	//insertSynchronization(L);
	return true;
}


void DSWP::addEdge(Instruction *u, Instruction *v, DType dtype) {
	pdg[u]->push_back(Edge(u, v, dtype));
	allEdges.push_back(Edge(u, v, dtype));
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

////////////////////////////////////////////////////////////////////////////////////////////////////////

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
		for(unsigned j = 0; j < nodes.size(); j++) {
			ost << nodes[j] << " ";
		}
		ost << "\n";
	}
}


