//4th step: code splitting

#include "DSWP.h"

using namespace llvm;
using namespace std;

void DSWP::loopSplit(Loop *L) {
//	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
//		BasicBlock *BB = *bi;
//		for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
//			Instruction *inst = &(*ii);
//		}
//	}

	allFunc.clear();

	//check for each partition, find relevant blocks, set could auto deduplicate
	for (int i = 0; i < MAX_THREAD; i++) {
		//create function to each each thread TODO: decide the argument of the function
		BasicBlock *header = L->getHeader();
		Constant * c = header->getParent()->getParent()->getOrInsertFunction("subloop_" + itoa(i), FunctionType::getVoidTy(header->getContext()), NULL);
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
			BBMap[BB] = BasicBlock::Create(BB->getContext(), BB->getNameStr() + "_" + itoa(i), func);
		}

		IRBuilder<> Builder(getGlobalContext());

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
							//TODO find the nearest post-dominator (not sure it is correct)
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

		//TODO insert function call here
	}
}
