all: DSWP.so

CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) -g -O0

DSWP.o: DSWP_0.cpp DSWP_1.cpp DSWP_2.cpp DSWP_3.cpp DSWP_4.cpp DSWP_5.cpp LivenessAnalysis.cpp DSWP.h LivenessAnalysis.h Utils.cpp Utils.h

%.so: %.o
	$(CXX) -dylib -flat_namespace -shared -g -O0 $^ -o $@

test-files/%.o: test-files/%.c
	clang -O0 -c -emit-llvm $< -o $@
test-files/%.o: test-files/%.cpp
	clang++ -O0 -c -emit-llvm $< -o $@
test-files/%-DSWP.o: test-files/%-m2r.o DSWP.so
	opt -load ./DSWP.so -cd-dce $< -o $@
