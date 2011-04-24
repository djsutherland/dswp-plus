//test

/*
 * DFAValue.h
 *
 *  Created on: Jan 29, 2011
 *      Author: Fuyao Zhao, Mark Hahnenberg
 */

#ifndef DFAVALUE_H_
#define DFAVALUE_H_

#include <vector>
using namespace std;

class DFAValue {
private:
	vector<bool> bits;
public:
	DFAValue();
	DFAValue(int n);	// the length of the bit vector is n;
	//DFAValue(const DFAValue &y);

	void set(int i);	// set ith bit to one
	void clear(int i);	// set ith bit to zero

	void set();			// set all bits to one
	void clear();		// set all bits to zero

	bool get(int i) const;	// get the value of ith bit


	void show();	//for debug

	DFAValue operator & (const DFAValue &y) const;
	DFAValue operator | (const DFAValue &y) const;
	DFAValue operator - (const DFAValue &y) const;	//todo if needed
	bool operator == (const DFAValue &y) const;
	bool operator != (const DFAValue &y) const;
};

#endif /* DFAVALUE_H_ */
