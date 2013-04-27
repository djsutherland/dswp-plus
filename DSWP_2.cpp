//2nd step: strong connected graph

#include "DSWP.h"

using namespace llvm;
using namespace std;

raw_os_ostream outstream(cout);

void DSWP::findSCC(Loop *L) {
	cout<<">>Starting SCC computation"<<endl;
	used.clear();

	for (Loop::block_iterator bi = L->getBlocks().begin();
		 bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);
			if (!used[inst])
				dfs_forward(inst);
		}
	}

	used.clear();
	dag_added.clear();
	sccNum = 0;
	//store InstInSCC, vector store all the inst for a scc
	InstInSCC.clear();

	for (int i = list.size() - 1; i >= 0; i--) {
		Instruction *inst = list[i];
		if (!used[inst]) {
			InstInSCC.push_back(vector<Instruction *>()); //Allocate instr. list
			dag[sccNum] = new vector<int>(); //Allocate DAG adj. list for ith SCC
			dfs_reverse(inst);
			sccNum++;
		}
	}
}

void DSWP::dfs_forward(Instruction *I) {
	cout<<">>Calling dfs_forward on [[";
	I->print(outstream);
	cout<<"]]"<<endl;

	used[I] = true;
	for (vector<Edge>::iterator ei = pdg[I]->begin(); ei != pdg[I]->end();
								ei++) {
		Instruction *next = ei->v;
		if (!used[next])
			dfs_forward(next);
	}
	list.push_back(I);

	cout<<">>End dfs_forward on [[";
	I->print(outstream);
	cout<<"]]"<<endl;
}

void DSWP::dfs_reverse(Instruction *I) {
	cout<<">>Calling dfs_reverse on [[";
	I->print(outstream);
	cout<<"]]"<<endl;

	used[I] = true;
	for (vector<Edge>::iterator ei = rev[I]->begin(); ei != rev[I]->end(); ei++) {
		Instruction *next = ei->v;
		if (!used[next])
			dfs_reverse(next);
		else { //Represents edge between two different SCCs
			int u = sccId[next];
			std::pair<int, int> sccedge = std::make_pair(u, sccNum);
			if (!dag_added[sccedge]) { //No edge between two SCCs yet
				//Add the edge
				dag[u]->push_back(sccNum);
				dag_added[sccedge] = true;
			}
		}
	}
	sccId[I] = sccNum;
	InstInSCC[sccNum].push_back(I);

	cout<<">>End dfs_reverse on [[";
	I->print(outstream);
	cout<<"]]"<<endl;
}
