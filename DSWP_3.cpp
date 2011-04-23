//3rd step: thread partition

#include "DSWP.h"

using namespace llvm;
using namespace std;


void DSWP::threadPartition(Loop *L) {
	//1. get total latency and latency for

	assigned.clear();
	sccLatency.clear();

	int totalLatency = 0;

	for (int i = 0; i < sccNum; i++)
		sccLatency.push_back(0);
	for (Loop::block_iterator bi = L->getBlocks().begin(); bi != L->getBlocks().end(); bi++) {
		BasicBlock *BB = *bi;
		for (BasicBlock::iterator ii = BB->begin(); ii != BB->end(); ii++) {
			Instruction *inst = &(*ii);
			int latency = getLatency(inst);
			sccLatency[sccId[inst]] += latency;
			totalLatency += latency;
		}
	}

	int averLatency = totalLatency / MAX_THREAD;

	cout << "latency info:" << totalLatency << " " << averLatency << endl;

	for (int i = 0; i < sccNum; i++)
		assigned.push_back(-2);
	//-2: not assigned, and not in queue
	//-1: not assigned, but in queue
	//0 ~ MAX_THREAD, already been assigned, not in queue

	int estLatency[MAX_THREAD];

	//TODO NOT SURE HERE IS RIGHT! header vs. preheader
	int start = sccId[L->getHeader()->getFirstNonPHI()];

	priority_queue<QNode> Q;
	Q.push(QNode(start, sccLatency[start]));

	for (int i = 0; i < MAX_THREAD; i++) {
		while (!Q.empty()) {
			QNode top = Q.top(); Q.pop();
			assigned[top.u] = i;
			estLatency[i] += top.latency;
			//update the list
			for (unsigned j = 0; j < dag[top.u]->size(); j++) {
				int v = dag[top.u]->at(j);
				if (assigned[v] > -2)
					continue;
				assigned[v] = -1;
				Q.push(QNode(v, sccLatency[v]));
			}
			//check load balance
			if (estLatency[i] >= averLatency) {
				//cout << estLatency[i] << endl;
				break;
			}
		}
	}
	//following is impossible since every time I let it bigger that aver
	if (!Q.empty()) {
		printf("err");
	}

	for (int i = 0; i < MAX_THREAD; i++)
		part[i].clear();

	for (int i = 0; i < sccNum; i++) {
	//	cout << i << "  " << assigned[i] << endl;
		part[assigned[i]].push_back(i);
	}
}

int DSWP::getLatency(Instruction *I) {
	int opcode = I->getOpcode();

	//copy from dswp583 google project site TODO this table is obviously not right!
	int cost;
    // These are instruction latencies for an AMD K10
    // (Well, most of them are, some I just made up)
    switch (opcode) {
        // Terminator instructions
        case 1: cost=2; break;          // Return
        case 2: cost=3; break;          // Branch
        case 3: cost=3; break;          // Switch

                // Standard binary operators
        case 8: cost=1; break;          // Add
        case 9: cost=4; break;          // FAdd
        case 10: cost=1; break;         // Sub
        case 11: cost=4; break;         // FSub
        case 12: cost=3; break;         // Mul
        case 13: cost=4; break;         // FMul
        case 14: cost=17; break;        // UDiv
        case 15: cost=17; break;        // SDiv
        case 16: cost=24; break;        // FDiv
        case 17: cost=17; break;        // URem
        case 18: cost=17; break;        // SRem
        case 19: cost=24; break;        // FRem

                 // logical operators (integer operands)
        case 20: cost=7; break;         // Shift left
        case 21: cost=7; break;         // Shift right
        case 22: cost=7; break;         // Shifr right (arithmetic)
        case 23: cost=1; break;         // And
        case 24: cost=1; break;         // Or
        case 25: cost=1; break;         // Xor

                 // Memory ops
                case 26: cost=2; break;         // Alloca
        case 27: cost=2; break;         // Load
        case 28: cost=2; break;         // Store
        case 29: cost=1; break;         // GetElementPtr

                 // Cast operators
        case 30: cost=1; break;         // Truncate integers
        case 31: cost=1; break;         // Zero extend integers
        case 32: cost=1; break;         // Sign extend integers
        case 33: cost=4; break;         // FPtoUI
        case 34: cost=4; break;         // FPtoSI
        case 35: cost=4; break;         // UItoFP
        case 36: cost=4; break;         // SItoFP
        case 37: cost=4; break;         // FPTrunc
        case 38: cost=4; break;         // FPExt
        case 39: cost=2; break;         // PtrToInt
        case 40: cost=2; break;         // IntToPtr
        case 41: cost=1; break;         // Type cast

                 // Other
        case 42: cost=1; break;         // Integer compare
        case 43: cost=1; break;         // Float compare
        case 44: cost=1; break;         // PHI node
        case 45: cost=50; break;        // Call function (this one is really variable)

        default: cost=1;
    }

    return (cost);
}
