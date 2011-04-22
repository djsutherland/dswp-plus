#ifndef DSWP_H_
#define DSWP_H_

#include "llvm/Pass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/Value.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/LLVMContext.h"
#include "LivenessAnalysis.h"

#include "Utils.h"

#include <ostream>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <queue>

using namespace llvm;
using namespace std;

namespace {
	static const int MAX_THREAD = 4;

	enum DType {REG, DTRUE, DANTI, DOUT, CONTROL, CONTROL_LC};

	struct Edge {
		Instruction *u, *v;
		DType dtype;
		Edge(Instruction *u, Instruction *v, DType dtype);
	};

	struct QNode {
		int u;
		int latency;
		QNode(int u, int latency) {
			this->u = u;
			this->latency = latency;
		}
		bool operator < (const QNode &y) const {
			return latency < y.latency;
		}
	};

	class DSWP: public LoopPass {

	private:
		//part 0
		void addEdge(Instruction *u, Instruction *v, DType dtype);

		bool checkEdge(Instruction *u, Instruction *v);

		//part 1: program dependence graph
		void buildPDG(Loop *L);

		void checkControlDependence(BasicBlock *a, BasicBlock *b, PostDominatorTree &pdt);

		void addControlDependence(BasicBlock *a, BasicBlock *b);

		//part 2: scc and dag
		void findSCC(Loop *L);

		void buildDAG(Loop *L);

		void dfs1(Instruction *inst);

		void dfs2(Instruction *inst);

		//program dependence graph
		map<Instruction *, vector<Edge> * > pdg;
		//reverse graph for scc
		map<Instruction *, vector<Edge> * > rev;
		//dag
		map<int, vector<int> * > dag;
		//
		vector< vector<Instruction *> > InstInSCC;

		//the father node for each block in post dominator tree
		map<BasicBlock *, BasicBlock * > pre;

		//total number of scc
		int sccNum;

		//map instruction to sccId
		map<Instruction *, int> sccId;

		//tmp flag
		map<Instruction *, bool> used;

		//store the dfs sequence
		vector<Instruction *> list;

		//part 3: thread partition
		void threadPartition(Loop *L);

		int getLatency(Instruction *I);

		vector<int> assigned;


		vector<int> part[MAX_THREAD];
		set<BasicBlock *> BBS[MAX_THREAD];

		//total lantency within a scc
		vector<int> sccLatency;

		//part 4: code splitting
		void loopSplit(Loop *L);

		//the new functions (has already been inserted, waiting for syn)
		vector<Function *> allFunc;

	public:
		static char ID;
		DSWP();
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual bool runOnLoop(Loop *L, LPPassManager &LPM);
		//virtual bool doFinalization();
	};
}


#endif
