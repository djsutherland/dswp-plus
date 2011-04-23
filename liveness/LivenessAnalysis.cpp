#include "LivenessAnalysis.h"

char LivenessAnalysis::ID = 0;
RegisterPass<LivenessAnalysis> X("liveness-dswp", "15745: liveness Analysis");
