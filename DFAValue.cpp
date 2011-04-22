/*
 * DFAValue.cpp
 *
 *  Created on: Jan 29, 2011
 *      Author: Fuyao Zhao, Mark Hahnenberg
 */

#include "DFAValue.h"

#include <iostream>
using namespace std;


DFAValue::DFAValue() {

}

DFAValue::DFAValue(int n) {
	bits = vector<bool>(n);
	clear();
}

void DFAValue::set(int i) {
	bits[i] = 1;
}

void DFAValue::clear(int i) {
	bits[i] = 0;
}

bool DFAValue::get(int i) const {
	return bits[i];
}

void DFAValue::clear() {
	for (unsigned i = 0; i < bits.size(); i++)
		clear(i);
}

void DFAValue::set() {
	for (unsigned i = 0; i < bits.size(); i++)
		set(i);
}


DFAValue DFAValue::operator | (const DFAValue &y) const {
	DFAValue res = *this;
	for (unsigned i = 0; i < y.bits.size(); i++) {
		res.bits[i] = y.bits[i] | this->bits[i];
	}
	return res;
}

DFAValue DFAValue::operator & (const DFAValue &y) const {
	DFAValue res = *this;
	for (unsigned i = 0; i < y.bits.size(); i++) {
		res.bits[i] = y.bits[i] & this->bits[i];
		//if (y.get(i) && this->get(i))
		//	res.set(i);
	}
	return res;
}

bool DFAValue::operator == (const DFAValue &y) const {
	for (unsigned i = 0; i < y.bits.size(); i++) {
		if (get(i) != y.get(i) )
			return false;
	}
	return true;
}


bool DFAValue::operator != (const DFAValue &y) const {
	return !(*this == y);
}


void DFAValue::show() {
	cout << bits.size() << ": ";
	for (unsigned i = 0; i < bits.size(); i++) {
		cout << (bits[i] ? 1 : 0);
	}
	cout << endl;
}
