// 5th step - inserting thread synchronization

#include "DSWP.h"

using namespace llvm;
using namespace std;

void DSWP::insertSynchronization(Loop *L) {
	//this->insertSynDependecy(L);
	cout << "inserting sychronization" << endl;
	int channel = 0;

	for (unsigned int i = 0; i < allEdges.size(); i++) {
		Edge e = allEdges[i];
//		int t1 = assigned[sccId[e.u]];
//		int t2 = assigned[sccId[e.v]];
//		if (t1 == t2)	//they are in same thread, therefore no need to syn
//			continue;


		for (int utr = 0; utr < MAX_THREAD; utr++) {
			for (int vtr = 0; vtr < MAX_THREAD; vtr++) {
				if (utr == vtr)
					continue;

				if (getInstAssigned(e.u) != utr)
					continue;

				if (instMap[utr][e.u] == NULL || instMap[vtr][e.v] == NULL)
					continue;

				Instruction * nu = dyn_cast<Instruction>(instMap[utr][e.u]);
				Instruction * nv = dyn_cast<Instruction>(instMap[vtr][e.v]);

				if (nu == NULL || nv == NULL)
					continue;

				insertProduce(nu, nv, e.dtype, channel, utr, vtr);
				insertConsume(nu, nv, e.dtype, channel, utr, vtr);
				channel++;
			}
		}


//		if (isa<TerminatorInst>(e.v)) {	//so e.v is a  branch that could be copy into many thread
//			//vector<Value*> termList = termMap[e.v];
//			for (unsigned j = 0; j < MAX_THREAD; j++) {
//				Value *vv = instMap[j][e.v];
//				if (vv == NULL)
//					continue;
//				//insertProduce(dyn_cast<Instruction>(oldToNew[e.u]), dyn_cast<Instruction>(oldToNew[e.v]), e.dtype, channel);
//				//insertConsume(dyn_cast<Instruction>(oldToNew[e.u]), dyn_cast<Instruction>(vv), e.dtype, channel);
//				channel++;
//			}
//		} else {
//
//			//should get thread num first!!
//			int uthr = this->getInstAssigned(e.u);
//			int vthr = this->getInstAssigned(e.v);
////			e.u->dump();
////			e.v->dump();
////			Value *tu = oldToNew[e.u];
////			tu->dump();
////			Instruction *nu = dyn_cast<Instruction>(tu);
//			//Instruction *nv = dyn_cast<Instruction>(oldToNew[e.u]);
//
//			insertProduce(dyn_cast<Instruction>(instMap[uthr][e.u]), dyn_cast<Instruction>(instMap[vthr][e.v]), e.dtype, channel, uthr, vthr);
//			insertConsume(dyn_cast<Instruction>(instMap[uthr][e.u]), dyn_cast<Instruction>(instMap[vthr][e.v]), e.dtype, channel, uthr, vthr);
//			channel++;
//		}
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

void DSWP::insertProduce(Instruction * u, Instruction *v, DType dtype, int channel, int uthread, int vthread) {
	Function *fun = module->getFunction("sync_produce");
	vector<Value*> args;
	Instruction *insPos = u->getNextNode();

	if (insPos == NULL) {
		error("here cannot be null");
	}

	//if (isa<BranchInst>(u)) {
	//	error("I don't know how do deal with it");
	//	return;
	//}

	if (dtype == REG) {	//register dep
		CastInst *cast;

		if (u->getType()->isIntegerTy()) {
			cast = new SExtInst(u, eleType, u->getNameStr() + "_64");
		}
		else if (u->getType()->isFloatingPointTy()) {
			if (u->getType()->isFloatTy()) {
				error("float sucks");
			}
			cast = new BitCastInst(u, eleType, u->getNameStr() + "_64");
		}
		else if (u->getType()->isPointerTy()){
			cast = new PtrToIntInst(u, eleType, u->getNameStr() + "_64");	//cast value
		} else {
			error("what's the hell type");
		}

		cast->insertBefore(insPos);
		args.push_back(cast);													//push the value
	}
	else if (dtype == DTRUE) { //true dep
		error("check mem dep!!");

		StoreInst *store = dyn_cast<StoreInst>(u);
		if (store == NULL) {
			error("not true dependency!");
		}
		BitCastInst *cast = new BitCastInst(store->getOperand(0), Type::getInt8PtrTy(*context), u->getNameStr() + "_ptr");
		cast->insertBefore(insPos);
		args.push_back(cast);													//push the value											//insert call
	} else {	//others
		args.push_back(Constant::getNullValue( Type::getInt64Ty(*context) ) );			//just a dummy value
	}

	args.push_back(ConstantInt::get(Type::getInt32Ty(*context), channel));
	CallInst *call = CallInst::Create(fun, args.begin(), args.end());		//call it
//	call->setName("p" + channel);
	call->insertBefore(insPos);												//insert call
}

void DSWP::insertConsume(Instruction * u, Instruction *v, DType dtype, int channel, int uthread, int vthread) {
//	int uthread = this->getNewInstAssigned(u);
//	int vthread = this->getNewInstAssigned(v);

	Value *oldu = newToOld[u];
	Function *fun = module->getFunction("sync_consume");
	vector<Value*> args;

	args.push_back(ConstantInt::get(Type::getInt32Ty(*context), channel));
	CallInst *call = CallInst::Create(fun, args.begin(), args.end(), "c" + itoa(channel));
	call->insertBefore(v);

	if (dtype == REG) {
		CastInst *cast;

		if (u->getType()->isIntegerTy()) {
			cast = new TruncInst(call, u->getType(), call->getNameStr() + "_val");
		}
		else if (u->getType()->isFloatingPointTy()) {
			if (u->getType()->isFloatTy())
				error("cannot deal with double");
			cast = new BitCastInst(call, u->getType(), call->getNameStr() + "_val");	//cast value
		}
		else if (u->getType()->isPointerTy()){
			cast = new IntToPtrInst(call, u->getType(), call->getNameStr() + "_val");	//cast value
		} else {
			error("what's the hell type");
		}

		cast->insertBefore(v);
//		for (unsigned i = 0; i < v->getNumOperands(); i++) {
//			Value *op = v->getOperand(i);
//			int opthread = this->getNewInstAssigned(op);
//			if (op == u) {
//				v->setOperand(i, cast);
//			}
//		}

		//cout << "replace reg" << endl;

		//TODO perhaps also need to replace other elements after this User
		for (Instruction::use_iterator ui = oldu->use_begin(); ui != oldu->use_end(); ui++) {	// this is diff from next block for DTRUE
			Instruction *user = dyn_cast<Instruction>(*ui);

			if (user == NULL) {
				error("how could it be NULL");
			}

		//	int userthread = this->getNewInstAssigned(user);
			if (user->getParent()->getParent() != v->getParent()->getParent()) {
				continue;
			}

			//user->dump();
			for (unsigned i = 0; i < user->getNumOperands(); i++) {
				Value * op = user->getOperand(i);
				if (op == oldu) {
					user->setOperand(i, cast);
				}
			}
			//cast->dump();
			//user->dump();
			//cout <<"replace operand" << endl;
		}

	} else if (dtype == DTRUE) {	//READ after WRITE
		error("check mem dep!!");

		if (!isa<LoadInst>(v)) {
			error("not true dependency");
		}
		BitCastInst *cast = new BitCastInst(call, v->getType(), call->getNameStr() + "_ptr");
		cast->insertBefore(v);

		//replace the v with 'cast' in v's thread: (other thread with be dealed using dependence)
		for (Instruction::use_iterator ui = v->use_begin(); ui != v->use_end(); ui++) {
			Instruction *user = dyn_cast<Instruction>(*ui);

			if (user == NULL) {
				error("how could it be NULL");
			}

		//	int userthread = this->getNewInstAssigned(user);
			if (user->getParent()->getParent() != v->getParent()->getParent()) {
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


