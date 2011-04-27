//1st step: create dependence graph

#include "DSWP.h"

using namespace llvm;
using namespace std;

void DSWP::addControlDependence(BasicBlock *a, BasicBlock *b) {
	//TODO: dinstingush the loop carried

	Instruction *u = &a->getInstList().back();
	Instruction *v = b->getFirstNonPHI();
	addEdge(u, v, CONTROL);
}

void DSWP::checkControlDependence(BasicBlock *a, BasicBlock *b, PostDominatorTree &pdt) {
	BasicBlock *lca = pdt.findNearestCommonDominator(a, b);

	cout << a->getNameStr() << " " << b->getNameStr() << " " << lca->getNameStr() << endl;

	if (lca == pre[a]) {	//case 1
		BasicBlock *BB = b;
		while (BB != lca) {
			addControlDependence(a, BB);
			BB = pre[BB];
		}
	} else if (lca == a) {	//case 2: capture loop dependence
		BasicBlock *BB = b;
		while (BB != pre[a]) {
			cout << "\t" << a->getNameStr() << " " << BB->getNameStr() << endl;
			addControlDependence(a, BB);
			BB = pre[BB];
		}
	} else {
		error("unknow case in checkControlDependence!");
	}
}

void DSWP::buildPDG(Loop *L) {
    //Initialize PDG
	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
			Instruction *inst = &(*ui);

			//standardlize the name for all expr
			if (util.hasNewDef(inst)) {
				inst->setName(util.genId());
				dname[inst] = inst->getNameStr();
			} else {
				dname[inst] = util.genId();
			}

			pdg[inst] = new vector<Edge>();
			rev[inst] = new vector<Edge>();
		}
	}

	//LoopInfo &li = getAnalysis<LoopInfo>();

	/*
	 * Memory dependency analysis
	 */
	MemoryDependenceAnalysis &mda = getAnalysis<MemoryDependenceAnalysis>();

	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
			Instruction *inst = &(*ii);

			//data dependence = register dependence + memory dependence

			//begin register dependence
			for (Value::use_iterator ui = ii->use_begin(); ui != ii->use_end(); ui++) {
				if (Instruction *user = dyn_cast<Instruction>(*ui)) {
					addEdge(inst, user, REG);
				}
			}
			//finish register dependence

			//begin memory dependence
			MemDepResult mdr = mda.getDependency(inst);
			//TODO not sure clobbers mean!!

			if (mdr.isDef()) {
				Instruction *dep = mdr.getInst();

				if (isa<LoadInst>(inst)) {
					if (isa<StoreInst>(dep)) {
						addEdge(dep, inst, DTRUE);	//READ AFTER WRITE
					}
				}
				if (isa<StoreInst>(inst)) {
					if (isa<LoadInst>(dep)) {
						addEdge(dep, inst, DANTI);	//WRITE AFTER READ
					}
					if (isa<StoreInst>(dep)) {
						addEdge(dep, inst, DOUT);	//WRITE AFTER WRITE
					}
				}
				//READ AFTER READ IS INSERT AFTER PDG BUILD
			}
			//end memory dependence
		}//for ii
	}//for bi

	/*
	 * begin control dependence
	 */
	PostDominatorTree &pdt = getAnalysis<PostDominatorTree>();
	//cout << pdt.getRootNode()->getBlock()->getNameStr() << endl;

	/*
	 * alien code part 1
	 */
	LoopInfo *LI = &getAnalysis<LoopInfo>();
	std::set<BranchInst*> backedgeParents;
	for (Loop::block_iterator bi = L->getBlocks().begin(); bi
			!= L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
			Instruction *inst = ii;
			if (BranchInst *brInst = dyn_cast<BranchInst>(inst)) {
				// get the loop this instruction (and therefore basic block) belongs to
				Loop *instLoop = LI->getLoopFor(BB);
				bool branchesToHeader = false;
				for (int i = brInst->getNumSuccessors() - 1; i >= 0
						&& !branchesToHeader; i--) {
					// if the branch could exit, store it
					if (LI->getLoopFor(brInst->getSuccessor(i)) != instLoop) {
						branchesToHeader = true;
					}
				}
				if (branchesToHeader) {
					backedgeParents.insert(brInst);
				}
			}
		}
	}


	//build information for predecessor of blocks in post dominator tree
	for (Function::iterator bi = func->begin(); bi != func->end(); bi++) {
		BasicBlock *BB = bi;
		DomTreeNode *dn = pdt.getNode(BB);

		for (DomTreeNode::iterator di = dn->begin(); di != dn->end(); di++) {
			BasicBlock *CB = (*di)->getBlock();
			pre[CB] = BB;
		}
	}
//
//	//add dependency within a basicblock
//	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
//		BasicBlock *BB = *bi;
//		Instruction *pre = NULL;
//		for (BasicBlock::iterator ui = BB->begin(); ui != BB->end(); ui++) {
//			Instruction *inst = &(*ui);
//			if (pre != NULL) {
//				addEdge(pre, inst, CONTROL);
//			}
//			pre = inst;
//		}
//	}

//	//the special kind of dependence need loop peeling ? I don't know whether this is needed
//	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
//		BasicBlock *BB = *bi;
//		for (succ_iterator PI = succ_begin(BB); PI != succ_end(BB); ++PI) {
//			BasicBlock *succ = *PI;
//
//			checkControlDependence(BB, succ, pdt);
//		}
//	}


	/*
	 * alien code part 2
	 */
	// add normal control dependencies
	// loop through each instruction
	for (Loop::block_iterator bbIter = L->block_begin(); bbIter
			!= L->block_end(); ++bbIter) {
		BasicBlock *bb = *bbIter;
		// check the successors of this basic block
		if (BranchInst *branchInst = dyn_cast<BranchInst>(bb->getTerminator())) {
			if (branchInst->getNumSuccessors() > 1) {
				BasicBlock * succ = branchInst->getSuccessor(0);
				// if the successor is nested shallower than the current basic block, continue
				if (LI->getLoopDepth(bb) < LI->getLoopDepth(succ)) {
					continue;
				}
				// otherwise, add all instructions to graph as control dependence
				while (succ != NULL && succ != bb && LI->getLoopDepth(succ)
						>= LI->getLoopDepth(bb)) {
					Instruction *terminator = bb->getTerminator();
					for (BasicBlock::iterator succInstIter = succ->begin(); &(*succInstIter)
							!= succ->getTerminator(); ++succInstIter) {
						addEdge(terminator, &(*succInstIter), CONTROL);
					}
					if (BranchInst *succBrInst = dyn_cast<BranchInst>(succ->getTerminator())) {
						if (succBrInst->getNumSuccessors() > 1) {
							addEdge(terminator, succ->getTerminator(),
									CONTROL);
						}
					}
					if (BranchInst *br = dyn_cast<BranchInst>(succ->getTerminator())) {
						if (br->getNumSuccessors() == 1) {
							succ = br->getSuccessor(0);
						} else {
							succ = NULL;
						}
					} else {
						succ = NULL;
					}
				}
			}
		}
	}


	/*
	 * alien code part 3
	 */
    for (std::set<BranchInst*>::iterator exitIter = backedgeParents.begin(); exitIter != backedgeParents.end(); ++exitIter) {
        BranchInst *exitBranch = *exitIter;
        if (exitBranch->isConditional()) {
            BasicBlock *header = LI->getLoopFor(exitBranch->getParent())->getHeader();
            for (BasicBlock::iterator ctrlIter = header->begin(); ctrlIter != header->end(); ++ctrlIter) {
                addEdge(exitBranch, &(*ctrlIter), CONTROL);
            }
        }
    }

	//end control dependence
}
