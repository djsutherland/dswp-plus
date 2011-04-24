#include "Utils.h"
#include <iostream>
#include <cstdio>
#include <sstream>

using namespace std;

string itoa(int i) {
    std::stringstream out;
    out << i;
    return out.str();
}

void error(const char *info) {
	printf("%s\n", info);
}

void error(string info) {
	printf("%s\n", info.c_str());
}


int Utils::id = 0;

string Utils::genId() {
	std::stringstream out;
	out << "t" << id++;
	return out.str();
}

bool Utils::hasNewDef(Instruction *inst) {
	if (isa<StoreInst>(inst))
		return false;
	if (isa<TerminatorInst>(inst))
		return false;
	return true;
}
