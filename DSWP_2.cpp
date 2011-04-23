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
