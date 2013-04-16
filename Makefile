all: DSWP.so

CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) -g -O0

DSWP.so: DSWP_0.o DSWP_1.o DSWP_2.o DSWP_3.o DSWP_4.o DSWP_5.o LivenessAnalysis.o Utils.o
	$(CXX) -dylib -flat_namespace -shared -g -O0 $^ -o $@

test-files/%.o: test-files/%.c
	clang -O0 -c -emit-llvm $< -o $@
test-files/%.o: test-files/%.cpp
	clang++ -O0 -c -emit-llvm $< -o $@
test-files/%-DSWP.o: test-files/%-m2r.o DSWP.so
	opt -load ./DSWP.so -dswp $< -o $@
