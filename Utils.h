#ifndef UTIL_H_
#define UTIL_H_


#include "llvm/Instructions.h"
#include <iostream>
#include <string>
#include <sstream>
#include <string>

#include "Utils.h"

using namespace llvm;
using namespace std;

string itoa(int i);
void error(const char * info);
void error(string info);

class Utils {
private:
	static int id;
public:
	static string genId();	//return a new identifier
};

#endif /* UTIL_H_ */
