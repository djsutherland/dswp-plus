//2nd step: strong connected graph

#include "DSWP.h"

using namespace llvm;
using namespace std;

void DSWP::findSCC(Loop *L) {
	used.clear();

	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
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
			InstInSCC.push_back(vector<Instruction *>()); //Allocate instruction list
			dag[i] = new vector<int>(); //Allocate DAG adjecency list for ith SCC
			dfs_reverse(inst);
			sccNum++;
		}
	}
}

void DSWP::dfs_forward(Instruction *I) {
	used[I] = true;
	for (vector<Edge>::iterator ei = pdg[I]->begin(); ei != pdg[I]->end(); ei++) {
		Instruction *next = ei->v;
		if (!used[next])
			dfs1(next);
	}
	list.push_back(I);
}

void DSWP::dfs_reverse(Instruction *I) {
	used[I] = true;
	for (vector<Edge>::iterator ei = rev[I]->begin(); ei != rev[I]->end(); ei++) {
		Instruction *next = ei->v;
		if (!used[next])
			dfs_reverse(next);
		else { //Represents edge between two different SCCs
			int u = sccId[next];
			std::pair<int> sccedge = std::make_pair(u, sccNum);
			if (!dag_added[sccedge]) { //No edge between two SCCs yet
				//Add the edge
				dag[u]->push_back(sccNum);
				dag_added[sccedge] = true;
			}
		}
	}
	sccId[I] = sccNum;
	InstInSCC[sccNum].push_back(I);
}
