// 5th step - inserting thread synchronization

#include "DSWP.h"

using namespace llvm;
using namespace std;

void DSWP::insertSynchronization() {
  // main function is allFunc[0]
  Function *main = this->allFunc[0];
  // insert initialization code (e.g. send the function 
  // pointers to the waiting threads)

  // remaining functions are auxiliary
  for (unsigned i = 1; i < this->allFunc.size(); i++) {
    Function *curr = this->allFunc[i];
    // check each instruction and insert flows
    
  }
}
