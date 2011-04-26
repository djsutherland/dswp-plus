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

static const int MAX_THREAD = 2;

//REG: register dependency
//DTRUE: data dependency - read after write
//DANTI: data dependency - write after read
//DOUT: data dependency - write after write
//DSYN: data dependency - read after read
enum DType {
	REG, DTRUE, DANTI, DOUT, DSYN, CONTROL, CONTROL_LC
};

struct Edge {
	Instruction *u, *v;
	DType dtype;
	Edge(Instruction *u, Instruction *v, DType dtype);
};

struct QNode {
	int u;
	int latency;
	QNode(int u, int latency);
	bool operator <(const QNode &y) const;
};

class DSWP: public LoopPass {

private:
	//neccesary information
	Module * module;
	Function *func;
	BasicBlock * header;
	BasicBlock * exit;
	LLVMContext *context;
	int loopCounter;
	set<Function *> generated;	//all generated function that should not be run in the pass
	//

	//part 0
	void addEdge(Instruction *u, Instruction *v, DType dtype);

	bool checkEdge(Instruction *u, Instruction *v);

	//part 1: program dependence graph
	void buildPDG(Loop *L);

	void checkControlDependence(BasicBlock *a, BasicBlock *b,
			PostDominatorTree &pdt);

	void addControlDependence(BasicBlock *a, BasicBlock *b);

	//part 2: scc and dag
	void findSCC(Loop *L);

	void buildDAG(Loop *L);

	void dfs1(Instruction *inst);

	void dfs2(Instruction *inst);

	//program dependence graph
	map<Instruction *, vector<Edge> *> pdg;
	//reverse graph for scc
	map<Instruction *, vector<Edge> *> rev;
	//dag
	map<int, vector<int> *> dag;
	//edge table, all the dependency relationship
	vector<Edge> allEdges;

	vector<vector<Instruction *> > InstInSCC;

	//the father node for each block in post dominator tree
	map<BasicBlock *, BasicBlock *> pre;

	//total number of scc
	int sccNum;

	//map instruction to sccId
	map<Instruction *, int> sccId;

	//tmp flag
	map<Instruction *, bool> used;

	//store the dfs sequence
	vector<Instruction *> list;

	BasicBlock * replaceBlock;

	//part 3: thread partition
	void threadPartition(Loop *L);

	//get the latency estimate of a instruction
	int getLatency(Instruction *I);

	//assigned[i] = node i in dag has been assgined to assigned[i] thread
	vector<int> assigned;

	int getInstAssigned(Value *inst);

	//part[i] = i thread contains part[i] nodes of the dag
	vector<int> part[MAX_THREAD];
	set<BasicBlock *> BBS[MAX_THREAD];

	//total lantency within a scc
	vector<int> sccLatency;

	//part 4: code splitting
	void preLoopSplit(Loop *L);

	void loopSplit(Loop *L);

	void deleteLoop(Loop *L);

	//map<Value*, vector<Value*> > termMap; //map the new instruction to the old instu (terminators)
	map<Value*, Value*> instMap[MAX_THREAD]; //map the new instruction to the old instuction
	map<Value *, Value *> oldToNew;				//direct map without take thread number as arugment
	map<Value *, Value *> newToOld;

	int getNewInstAssigned(Value *inst);

	//the new functions (has already been inserted, waiting for syn)
	vector<Function *> allFunc;

	//get live variable infomration
	void getLiveinfo(Loop * L);
	vector<Value *> livein; //live in variable
	vector<Value *> defin; //Variable generate in the loop
	vector<Value *> liveout;

	// part 5: synchronization insertion
	void insertSynDependecy(Loop *L);

	void insertSynchronization(Loop *L);

	void insertProduce(Instruction * u, Instruction *v, DType dtype,
			int channel);

	void insertConsume(Instruction * u, Instruction *v, DType dtype,
			int channel);

	//test function
	void showGraph(Loop *L);

	//show DAC information
	void showDAG(Loop *L);

	//show partition
	void showPartition(Loop *L);

	//show live in and live out set
	void showLiveInfo(Loop *L);

	Utils util;

	//give each instruction a name, including terminator instructions (which can not be setName)
	map<Instruction *, string> dname;

	void initilize(Loop *L);

public:
	static char ID;
	DSWP();
	virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	virtual bool runOnLoop(Loop *L, LPPassManager &LPM);
	virtual bool doInitialization(Loop *, LPPassManager &LPM);
	//virtual bool doFinalization();
};

#endif
