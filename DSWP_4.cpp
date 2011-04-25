//4th step: code splitting

#include "DSWP.h"

using namespace llvm;
using namespace std;
//TODO add parameters for functions !!!

void DSWP::loopSplit(Loop *L) {
//	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
//		BasicBlock *BB = *bi;
//		for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
//			Instruction *inst = &(*ii);
//		}
//	}
	getLiveinfo(L);
	return;


	allFunc.clear();

	//prepare the arguments and function Type
	vector<const Type*> argTypes;
	argTypes.push_back(Type::)
	for (set<Value *>::iterator it = liveout.begin(), it2; it != liveout.end(); it++) {
		Value *val = * it;
		const Type *vType = val->getType();			//the type of the val
		argTypes.push_back(Type::getInt8PtrTy(*context));
	}
	FunctionType  *fType = FunctionType::get(Type::getVoidTy(*context), argTypes, false);

	//check for each partition, find relevant blocks, set could auto deduplicate
	for (int i = 0; i < MAX_THREAD; i++) {
		//create function to each each thread
		Constant * c = header->getParent()->getParent()->getOrInsertFunction("subloop_" + itoa(i), fType);	//they both have same function type
		Function *func = cast<Function>(c);
		func->setCallingConv(CallingConv::C);

		allFunc.push_back(func);

		//each partition contain several scc
		map<Instruction *, bool> relinst;
		set<BasicBlock *> relbb;
		for (vector<int>::iterator ii = part[i].begin(); ii != part[i].end(); ii++) {
			int scc = *ii;
			for (vector<Instruction *>::iterator iii = InstInSCC[scc].begin(); iii != InstInSCC[scc].end(); iii++) {
				Instruction *inst = *iii;
				relinst[inst] = true;
				relbb.insert(inst->getParent());

				//add blocks which the instruction dependent on
				for (vector<Edge>::iterator ei = rev[inst]->begin(); ei != rev[inst]->end(); ei++) {
					Instruction *dep = ei->v;
					relinst[dep] = true;
					relbb.insert(dep->getParent());
				}
			}
		}

		//copy blocks
		map<BasicBlock *, BasicBlock *> BBMap;	//map the old block to new block
		for (set<BasicBlock *>::iterator bi = relbb.begin();  bi != relbb.end(); bi++) {
			BasicBlock *BB = *bi;
			BasicBlock *NBB = BasicBlock::Create(BB->getContext(), BB->getNameStr() + "_" + itoa(i), func);
			BBMap[BB] = NBB;
			if (BB == header) {
				//TODO ensure it is in the first of the block
				//NBB->mo
			}
		}

		//IRBuilder<> Builder(getGlobalContext());

		//add instruction
		for (set<BasicBlock *>::iterator bi = relbb.begin();  bi != relbb.end(); bi++) {
			BasicBlock *BB = *bi;
			BasicBlock *NBB =  BBMap[BB];
			//BB->splitBasicBlock()

			for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
				Instruction * inst = ii;
				Instruction *newInst;
				if (isa<TerminatorInst>(inst)) {//copy the teminatorInst since they could be used mutiple times
					newInst = inst->clone();
					termMap[inst].push_back(newInst);
				} else {
					newInst = inst;
					inst->removeFromParent();
				}

				//TODO check if there are two branch, one branch is not in the partition, then what the branch

				//instruction will be
				for (unsigned j = 0; j < newInst->getNumOperands(); j++) {
					Value *op = newInst->getOperand(j);
					if (BasicBlock *oldBB = dyn_cast<BasicBlock>(op)) {
						BasicBlock * newBB = BBMap[oldBB];
						while (newBB == NULL) {
							//find the nearest post-dominator (TODO not sure it is correct)
							oldBB = pre[oldBB];
							newBB = BBMap[oldBB];
						}

						newInst->setOperand(j, newBB);
					}
				}
				//TODO not sure this is right way to do this
				NBB->getInstList().insertAfter(NBB->getInstList().end(), newInst);
			}
		}// for add instruction
	}

	//remove the old instruction and blocks in loop, basically only header should remain
	for (Loop::block_iterator bi = L->block_begin(); bi != L->block_end(); bi++) {
		BasicBlock * BB = *bi;
		BB->getTerminator()->removeFromParent();

		if (BB->getInstList().size() > 0) {
			error("check remove process of loop split");
		}

		if (BB == header) {
			BranchInst::Create(exit, header);
		} else {
			BB->removeFromParent();
		}
	}

	//insert store instruction (store register value to memory), now I insert them into the beginning of the function
//	Instruction *allocPos = func->getEntryBlock().getTerminator();
//
//	vector<StoreInst *> stores;
//	for (set<Value *>::iterator vi = defin.begin(); vi != defin.end(); vi++) {	//TODO: is defin enough
//		Value *val = *vi;
//		AllocaInst * arg = new AllocaInst(val->getType(), 0, val->getNameStr() + "_ptr", allocPos);
//		varToMem[val] = arg;
//	}

	//TODO insert function call here

//		//load back the live out
//
//		for (set<Value *>::iterator vi = liveout.begin(); vi != liveout.end(); vi++) {
//			Value *val = *vi;
//			Value *ptr = args[val];
//			LoadInst * newVal = new LoadInst(ptr, val->getNameStr() + "_new", insPos);
//
//
//			for (use_iterator ui = val->use_begin(); ui != val->use_end(); ui++) {
//
//		}
}

void DSWP::getLiveinfo(Loop * L) {

	return;
	//currently I don't want to use standard liveness analysis
	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);
			if (util.hasNewDef(inst)) {
				defin.insert(inst);
			}
		}
	}

	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);

			for (Instruction::op_iterator oi = inst->op_begin(); oi != inst->op_end(); oi++) {
				Value *op = *oi;
				if (defin.find(op) == defin.end()) {
					livein.insert(op);
				}
			}
		}
	}
	//so basically I add variables used in loop but not been declared in loop as live variable

	//I think we could assume liveout = livein + defin at first, especially I havn't understand the use of liveout
	liveout = livein;
	liveout.insert(defin.begin(), defin.end());

	//now we can delete those in liveout but is not really live outside the loop
	LivenessAnalysis *live = &getAnalysis<LivenessAnalysis>();
	BasicBlock *exit = L->getExitBlock();

	for (set<Value *>::iterator it = liveout.begin(), it2; it != liveout.end(); it = it2) {
		it2 = it; it2 ++;
		if (!live->isVaribleLiveIn(*it, exit)) {	//livein in the exit is the liveout of the loop
			liveout.erase(it);
		}
	}
}
