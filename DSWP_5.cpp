// 5th step - inserting thread synchronization

#include "DSWP.h"

using namespace llvm;
using namespace std;

void DSWP::insertSynchronization(Loop *L) {
	this->insertSynDependecy(L);
	// main function is allFunc[0]
	Function *main = this->allFunc[0];
	//TODO insert initialization code (e.g. send the function
	// pointers to the waiting threads)
	int channel = 0;

	for (unsigned int i = 0; i < allEdges.size(); i++) {
		Edge e = allEdges[i];
		int t1 = assigned[sccId[e.u]];
		int t2 = assigned[sccId[e.v]];
		if (t1 == t2)	//they are in same thread, therefore no need to syn
			continue;

		if (isa<TerminatorInst>(e.v)) {	//so e.v is a  branch that could be copy into many thread
			vector<Value*> termList = termMap[e.v];
			for (vector<Value*>::iterator vi = termList.begin(); vi != termList.end(); vi++) {
				Value *vv = *vi;
				insertProduce(e.u, e.v, e.dtype, channel);
				insertConsume(e.u, dyn_cast<Instruction>(vv), e.dtype, channel);
				channel++;
			}
		} else {
			insertProduce(e.u, e.v, e.dtype, channel);
			insertConsume(e.u, e.v, e.dtype, channel);
			channel++;
		}
	}

//	// remaining functions are auxiliary
//	for (unsigned i = 1; i < this->allFunc.size(); i++) {
//		Function *curr = this->allFunc[i];
//		// check each instruction and insert flows
//	}
}

void DSWP::insertSynDependecy(Loop *L) {
	MemoryDependenceAnalysis &mda = getAnalysis<MemoryDependenceAnalysis>();

	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
			Instruction *inst = &(*ii);

			MemDepResult mdr = mda.getDependency(inst);

			if (mdr.isDef()) {
				Instruction *dep = mdr.getInst();
				if (isa<LoadInst> (inst)) {
					if (isa<LoadInst> (dep)) {
						addEdge(dep, inst, DSYN); //READ AFTER READ
					}
				}
			}
		}
	}
}

void DSWP::insertProduce(Instruction * u, Instruction *v, DType dtype, int channel) {
	Function *fun = module->getFunction("sync_produce");
	vector<Value*> args;

	Instruction *pos = u;

	if (dtype == DTRUE) {
		BitCastInst *cast = new BitCastInst(u, Type::getInt8PtrTy(*context), u->getNameStr() + "_ptr");
		cast->insertAfter(u);
		pos = cast;
		args.push_back(cast);
	} else {
		args.push_back(ConstantInt::get(Type::getInt8Ty(*context), 0));	//just a dummy value
	}

	args.push_back(ConstantInt::get(Type::getInt8Ty(*context), channel));

	CallInst *call = CallInst::Create(fun, args.begin(), args.end());

	call->insertAfter(pos);
}

void DSWP::insertConsume(Instruction * u, Instruction *v, DType dtype, int channel) {
	Function *fun = module->getFunction("sync_consume");
	vector<Value*> args;
	args.push_back(ConstantInt::get(Type::getInt8Ty(*context), channel));
	CallInst *call = CallInst::Create(fun, args.begin(), args.end());
	if (dtype == DTRUE) {	//READ after WRITE
		if (!isa<LoadInst>(v)) {
			error("not true dependency");
		}

		BitCastInst *cast = new BitCastInst(call, 00000, call->getNameStr() + "_ptr");
		cast->insertBefore(v);
	}
	call->insertBefore(v);
}
