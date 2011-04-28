#include "DSWP.h"

using namespace llvm;
using namespace std;

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

void DSWP::showLiveInfo(Loop *L) {
	cout << "live variable information" << endl;
	for (int i = 0; i < livein.size(); i++) {
		Value * val = livein[i];
		cout << val->getNameStr() << "\t";
		//val->getType()->dump();
		//cout << endl;
	}
	cout << endl;
}

