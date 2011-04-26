// 5th step - inserting thread synchronization

#include "DSWP.h"

using namespace llvm;
using namespace std;

void DSWP::insertSynchronization(Loop *L) {
	//this->insertSynDependecy(L);

	int channel = 0;

	for (unsigned int i = 0; i < allEdges.size(); i++) {
		Edge e = allEdges[i];
		int t1 = assigned[sccId[e.u]];
		int t2 = assigned[sccId[e.v]];
		if (t1 == t2)	//they are in same thread, therefore no need to syn
			continue;

		if (isa<TerminatorInst>(e.v)) {	//so e.v is a  branch that could be copy into many thread
			//vector<Value*> termList = termMap[e.v];
			for (unsigned j = 0; j < MAX_THREAD; j++) {
				Value *vv = instMap[j][e.v];
				if (vv == NULL)
					continue;
				insertProduce(dyn_cast<Instruction>(oldToNew[e.u]), dyn_cast<Instruction>(oldToNew[e.v]), e.dtype, channel);
				insertConsume(dyn_cast<Instruction>(oldToNew[e.u]), dyn_cast<Instruction>(vv), e.dtype, channel);
				channel++;
			}
		} else {
			insertProduce(dyn_cast<Instruction>(oldToNew[e.u]), dyn_cast<Instruction>(oldToNew[e.v]), e.dtype, channel);
			insertConsume(dyn_cast<Instruction>(oldToNew[e.u]), dyn_cast<Instruction>(oldToNew[e.v]), e.dtype, channel);
			channel++;
		}
	}

	//cout << channel << endl;
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
	Instruction *insPos = v->getNextNode();
	if (insPos == NULL) {
		error("here cannot be null");
	}

	if (dtype == REG) {	//register dep
		BitCastInst *cast = new BitCastInst(u, Type::getInt8PtrTy(*context), u->getNameStr() + "_ptr");	//cast value
		cast->insertBefore(insPos);
		args.push_back(cast);													//push the value
	}
	else if (dtype == DTRUE) { //true dep
		StoreInst *store = dyn_cast<StoreInst>(u);
		if (store == NULL) {
			error("not true dependency!");
		}
		BitCastInst *cast = new BitCastInst(store->getOperand(0), Type::getInt8PtrTy(*context), u->getNameStr() + "_ptr");
		cast->insertBefore(insPos);
		args.push_back(cast);													//push the value											//insert call
	} else {	//others
		args.push_back(ConstantInt::get(Type::getInt8Ty(*context), 0));			//just a dummy value
	}

	args.push_back(ConstantInt::get(Type::getInt8Ty(*context), channel));
	CallInst *call = CallInst::Create(fun, args.begin(), args.end());		//call it
	call->insertBefore(insPos);												//insert call
}

void DSWP::insertConsume(Instruction * u, Instruction *v, DType dtype, int channel) {
	int uthread = this->getNewInstAssigned(u);
	int vthread = this->getNewInstAssigned(v);

	Function *fun = module->getFunction("sync_consume");
	vector<Value*> args;

	args.push_back(ConstantInt::get(Type::getInt8Ty(*context), channel));
	CallInst *call = CallInst::Create(fun, args.begin(), args.end());
	call->insertBefore(v);

	if (dtype == REG) {
		BitCastInst *cast = new BitCastInst(call, v->getType(), call->getNameStr() + "_ptr");
		cast->insertBefore(v);

		for (unsigned i = 0; i < v->getNumOperands(); i++) {
			Value *op = v->getOperand(i);
			int opthread = this->getNewInstAssigned(op);
			if (op == u) {
				v->setOperand(i, cast);
			}
		}
		//TODO perhaps also need to replace other elements after this User
		for (Instruction::use_iterator ui = u->use_begin(); ui != u->use_end(); ui++) {	// this is diff from next block for DTRUE
			User *user = *ui;

			int userthread = this->getNewInstAssigned(user);
			if (userthread != vthread) {
				continue;
			}

			cout << "see" << endl;

			for (unsigned i = 0; i < user->getNumOperands(); i++) {
				Value * op = user->getOperand(i);
				if (op == u) {
					user->setOperand(i, cast);
				}
			}
		}
	} else if (dtype == DTRUE) {	//READ after WRITE
		if (!isa<LoadInst>(v)) {
			error("not true dependency");
		}
		BitCastInst *cast = new BitCastInst(call, v->getType(), call->getNameStr() + "_ptr");
		cast->insertBefore(v);

		//replace the v with 'cast' in v's thread: (other thread with be dealed using dependence)
		for (Instruction::use_iterator ui = v->use_begin(); ui != v->use_end(); ui++) {
			User *user = *ui;

			int userthread = this->getNewInstAssigned(user);
			if (userthread != vthread) {
				cout << "prove what I think is true";
				continue;
			}

			for (unsigned i = 0; i < user->getNumOperands(); i++) {
				Value * op = user->getOperand(i);
				if (op == v) {
					user->setOperand(i, cast);
				}
			}
		}
	} else {

	}
}


