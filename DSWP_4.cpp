//4th step: code splitting

#include "DSWP.h"

using namespace llvm;
using namespace std;

void DSWP::preLoopSplit(Loop *L) {
	allFunc.clear();

	/*
	 * Insert a new block
	 */
	replaceBlock = BasicBlock::Create(*context, "loop-replace", func);
	BranchInst *brInst = BranchInst::Create(exit, replaceBlock);
	replaceBlock->moveBefore(exit);

	if (L->contains(exit)) {
		error("don't know why");
	}

	//BranchInst *brInst = BranchInst::Create(exit);
	//newBlock->getInstList().push_back(brInst);
	//every block to header (except the ones in the loop), will now redirect to here
	for (pred_iterator PI = pred_begin(header); PI != pred_end(header); ++PI) {
		BasicBlock *pred = *PI;
		if (L->contains(pred)) {
			continue;
		}
		TerminatorInst *termInst = pred->getTerminator();

		for (unsigned i = 0; i < termInst->getNumOperands(); i++) {
			BasicBlock *bb = dyn_cast<BasicBlock> (termInst->getOperand(i));
			if (bb == header) {
				termInst->setOperand(i, replaceBlock);
			}
		}
	}
	//TODO DOUBLE CHECK IF THIS WOULD CHANGE THE POST DOMINATOR TREE STRUCTURE!!!!!!!!!!!!!!!!!!!!!!!!!!!

	for (Loop::block_iterator li = L->block_begin(); li != L->block_end(); li++) {
		BasicBlock *BB = *li;
		if (BB == replaceBlock) {
			error("the blokc should not appear here!");
		}
	}

	/*
	 * prepare the function Type
	 */
	vector<const Type*> funArgTy;
	PointerType* argPointTy = PointerType::get(Type::getInt8Ty(*context), 0);
	funArgTy.push_back(argPointTy);
	FunctionType *fType = FunctionType::get(Type::getInt8PtrTy(*context),
			funArgTy, false);
	//add functions
	for (int i = 0; i < MAX_THREAD; i++) {
		//create functions to  each thread
		Constant * c = module->getOrInsertFunction(itoa(loopCounter)
				+ "_subloop_" + itoa(i), fType); //they both have same function type
		if (c == NULL) {
			error("no function!");
		}
		Function *func = cast<Function> (c);
		func->setCallingConv(CallingConv::C);
		allFunc.push_back(func);
	}

	/*
	 * prepare the actual parameters type
	 */
	ArrayType *arrayType = ArrayType::get(Type::getInt8PtrTy(*context),
			livein.size());
	AllocaInst *trueArg = new AllocaInst(arrayType, ""); //true argment for actual (the split one) function call
	trueArg->insertBefore(brInst);
	//trueArg->setAlignment(8);
	//trueArg->dump();

	for (unsigned i = 0; i < livein.size(); i++) {
		Value *val = livein[i];
		BitCastInst * castVal = new BitCastInst(val, Type::getInt8PtrTy(
				*context), val->getName() + "_ptr");
		castVal->insertBefore(brInst);

		//get the element ptr
		ConstantInt* idx = ConstantInt::get(Type::getInt64Ty(*context),
				(uint64_t) i);
		//GetElementPtrInst* ele_addr = GetElementPtrInst::Create(trueArg, idx,"");		//use this cannot get the right type!
		vector<Value *> arg;
		arg.push_back(idx);
		arg.push_back(idx);
		GetElementPtrInst* ele_addr = GetElementPtrInst::Create(trueArg,
				arg.begin(), arg.end(), "");
		ele_addr->insertBefore(brInst);

		//ele_addr->getType()->dump();

		StoreInst * storeVal = new StoreInst(castVal, ele_addr);
		storeVal->insertBefore(brInst);
	}

	/*
	 * call functions
	 */
	Function *delegate = module->getFunction("sync_delegate");
	for (int i = 0; i < MAX_THREAD; i++) {
		Function *func = allFunc[i];
		vector<Value*> args;
		args.push_back(ConstantInt::get(Type::getInt32Ty(*context),
				(uint64_t) i)); //tid
		args.push_back(func); //the function pointer

		PointerType * finalType = PointerType::get(
				Type::getInt8PtrTy(*context), 0);
		BitCastInst * finalArg = new BitCastInst(trueArg, finalType);
		finalArg->insertBefore(brInst);

		args.push_back(finalArg); //true arg that will be call by func
		CallInst * callfunc = CallInst::Create(delegate, args.begin(),
				args.end());
		callfunc->insertBefore(brInst);
	}

	/*
	 * join them back
	 */
	Function *join = module->getFunction("sync_join");
	CallInst *callJoin = CallInst::Create(join);
	callJoin->insertBefore(brInst);

	//replaceBlock->dump();	//check if the new block is correct

	//	//read back from memory
	//
	//	for (unsigned i = 0; i < livein.size(); i++) {
	//		Value *val = livein[i];
	//
	//		ConstantInt* idx = ConstantInt::get(Type::getInt64Ty(*context),
	//				(uint64_t) i);
	//		GetElementPtrInst* ele_addr = GetElementPtrInst::Create(trueArg, idx,
	//				""); //get the element ptr
	//
	//		Load * storeVal = new StoreInst(castVal, ele_addr);
	//		storeVal->insertBefore(brInst);
	//	}
}

void DSWP::loopSplit(Loop *L) {
	//	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
	//		BasicBlock *BB = *bi;
	//		for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
	//			Instruction *inst = &(*ii);
	//		}
	//	}


	//check for each partition, find relevant blocks, set could auto deduplicate

	for (int i = 0; i < MAX_THREAD; i++) {
		cout << "create function for thread " + itoa(i) << endl;

		//create function body to each thread
		Function *curFunc = allFunc[i];

		//each partition contain several scc
		map<Instruction *, bool> relinst;
		set<BasicBlock *> relbb;

		cout << part[i].size() << endl;

		/*
		 * analysis the dependent blocks
		 */
		for (vector<int>::iterator ii = part[i].begin(); ii != part[i].end(); ii++) {
			int scc = *ii;
			for (vector<Instruction *>::iterator iii = InstInSCC[scc].begin(); iii
					!= InstInSCC[scc].end(); iii++) {
				Instruction *inst = *iii;
				relinst[inst] = true;
				relbb.insert(inst->getParent());

				//add blocks which the instruction dependent on
				for (vector<Edge>::iterator ei = rev[inst]->begin(); ei
						!= rev[inst]->end(); ei++) {
					Instruction *dep = ei->v;
					relinst[dep] = true;
					relbb.insert(dep->getParent());
				}
			}
		}

		//check consistence of the blocks
		for (set<BasicBlock *>::iterator bi = relbb.begin(); bi != relbb.end(); bi++) {
			BasicBlock *BB = *bi;
			cout << BB->getNameStr() << "\t";
		}
		cout << endl;

		map<BasicBlock *, BasicBlock *> BBMap; //map the old block to new block

		if (relbb.size() == 0) {
			error("has size 0");
		}

		/*
		 * Create the new blocks to the new function, including an entry and exit
		 */
		BasicBlock * newEntry = BasicBlock::Create(*context, "new-entry",
				curFunc);
		BasicBlock * newExit =
				BasicBlock::Create(*context, "new-exit", curFunc);

		//copy the basicblock and modify the control flow
		for (set<BasicBlock *>::iterator bi = relbb.begin(); bi != relbb.end(); bi++) {
			BasicBlock *BB = *bi;
			BasicBlock *NBB = BasicBlock::Create(*context, BB->getNameStr()
					+ "_" + itoa(i), curFunc);
			BBMap[BB] = NBB;
		}

		/*
		 * insert the control flow and normal instructions
		 */
		BranchInst * newToHeader = BranchInst::Create(BBMap[header], newEntry); //pointer to the header so loop can be executed
		ReturnInst * newRet = ReturnInst::Create(*context,
				Constant::getNullValue(Type::getInt8PtrTy(*context)), newExit); //return null

		for (set<BasicBlock *>::iterator bi = relbb.begin(); bi != relbb.end(); bi++) {
			BasicBlock *BB = *bi;
			BasicBlock *NBB = BBMap[BB];
			if (!NBB->empty())
				error("insane error! DSWP4");

			//insert normal instruction
			for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
				Instruction * inst = ii;

				if (!relinst[inst] && !isa<TerminatorInst> (inst)) //TODO I NEED TO CHECK THIS BACK
					continue;

				Instruction *newInst = inst->clone();
				NBB->getInstList().push_back(newInst);

				if (isa<TerminatorInst> (newInst)) {
					for (unsigned j = 0; j < newInst->getNumOperands(); j++) {
						Value *op = newInst->getOperand(j);

						if (BasicBlock * oldBB = dyn_cast<BasicBlock>(op)) {
							BasicBlock * newBB = BBMap[oldBB];
							while (newBB == NULL) {
								//find the nearest post-dominator (TODO not sure it is correct)
								oldBB = pre[oldBB];
								newBB = BBMap[oldBB];
							}
							//replace the target block
							newInst->setOperand(j, newBB);

							//TODO check if there are two branch, one branch is not in the partition, then what the branch
						}
					}
					oldToNew[inst] = newInst;
					newToOld[newInst] = inst;
				}
				instMap[i][inst] = newInst;
			}
		}

		/*
		 * Insert load instruction to load argument, (replace the live in variables)
		 */
		Function::ArgumentListType &arglist = curFunc->getArgumentList();
		if (arglist.size() != 1) {
			error("argument size error!");
		}
		Argument *args = arglist.begin(); //the function only have one argmument

		BitCastInst *castArgs = new BitCastInst(args, PointerType::get(
				Type::getInt8Ty(*context), 0));
		castArgs->insertBefore(newToHeader);

		for (unsigned j = 0; j < livein.size(); j++) {
			ConstantInt* idx = ConstantInt::get(Type::getInt64Ty(*context),
					(uint64_t) j);
			GetElementPtrInst* ele_addr = GetElementPtrInst::Create(castArgs,
					idx, ""); //get the element ptr
			ele_addr->insertBefore(newToHeader);
			LoadInst * ele_val = new LoadInst(ele_addr);
			ele_val->insertBefore(newToHeader);
			Value *val = livein[j];
			instMap[i][val] = ele_val;
		}

		/*
		 * Replace the use of intruction def in the function (reg dep should be finshied in insert syn
		 */
		for (inst_iterator ii = inst_begin(curFunc); ii != inst_end(curFunc); ii++) {
			Instruction *inst = &(*ii);
			for (unsigned j = 0; j < inst->getNumOperands(); j++) {
				Value *op = inst->getOperand(j);
				if (Value * newArg = instMap[i][op]) {
					inst->setOperand(j, newArg);
				} else {
					inst->setOperand(j, oldToNew[op]);	//set a new variable but is in other thread, however, now we don't have ref to original loop
				}
			}
		}
	}

	/*
	 * Insert Syn to gurantee
	 */

	//cout << "test_now" << endl;
	return;

	//insert store instruction (store register value to memory), now I insert them into the beginning of the function
	//	Instruction *allocPos = func->getEntryBlock().getTerminator();
	//
	//	vector<StoreInst *> stores;
	//	for (set<Value *>::iterator vi = defin.begin(); vi != defin.end(); vi++) {	//TODO: is defin enough
	//		Value *val = *vi;
	//		AllocaInst * arg = new AllocaInst(val->getType(), 0, val->getNameStr() + "_ptr", allocPos);
	//		varToMem[val] = arg;
	//	}
}

void DSWP::deleteLoop(Loop *L) {
	for (Loop::block_iterator bi = L->block_begin(), bi2; bi != L->block_end(); bi = bi2) {
		bi2 = bi;
		bi2++;
		BasicBlock * BB = *bi;
		for (BasicBlock::iterator ii = BB->begin(), ii2; ii != BB->end(); ii
				= ii2) {
			ii2 = ii;
			ii2++;
			Instruction *inst = ii;
			inst->removeFromParent();
		}
		BB->removeFromParent();
	}
}

void DSWP::getLiveinfo(Loop * L) {
	defin.clear();
	livein.clear();
	liveout.clear();

	//currently I don't want to use standard liveness analysis
	for (Loop::block_iterator bi = L->getBlocks().begin(); bi
			!= L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);
			if (util.hasNewDef(inst)) {
				defin.push_back(inst);
			}
			for (Instruction::use_iterator oi = inst->use_begin(); oi
					!= inst->use_end(); oi++) {
				User *use = *oi;
				if (Instruction *ins = dyn_cast<Instruction>(use)) {
					if (!L->contains(ins)) {
						error("loop defin exist outside the loop");
					}
				} else {
					error("tell me how could not");
				}
			}
		}
	}

	//make all the use in the loop, but not defin in it, as live in variable
	for (Loop::block_iterator bi = L->getBlocks().begin(); bi
			!= L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);

			for (Instruction::op_iterator oi = inst->op_begin(); oi
					!= inst->op_end(); oi++) {
				Value *op = *oi;
				if (isa<Instruction> (op) || isa<Argument> (op)) {
					if (find(defin.begin(), defin.end(), op) == defin.end()) { //
						livein.push_back(op);
					}
				}
			}
		}
	}

	//NOW I DIDN'T SEE WHY WE NEED LIVE OUT
	//	//so basically I add variables used in loop but not been declared in loop as live variable
	//
	//	//I think we could assume liveout = livein + defin at first, especially I havn't understand the use of liveout
	//	liveout = livein;
	//	liveout.insert(defin.begin(), defin.end());
	//
	//	//now we can delete those in liveout but is not really live outside the loop
	//	LivenessAnalysis *live = &getAnalysis<LivenessAnalysis>();
	//	BasicBlock *exit = L->getExitBlock();
	//
	//	for (set<Value *>::iterator it = liveout.begin(), it2; it != liveout.end(); it = it2) {
	//		it2 = it; it2 ++;
	//		if (!live->isVaribleLiveIn(*it, exit)) {	//livein in the exit is the liveout of the loop
	//			liveout.erase(it);
	//		}
	//	}
}
