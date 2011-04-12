#include "Util.h"

#include <sstream>

using namespace std;

string itoa(int i) {
    std::stringstream out;
    out << i;
    return out.str();
}
