all: DSWP.so

CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) -g -O0

DSWP.so: DSWP_0.o DSWP_1.o DSWP_2.o DSWP_3.o DSWP_4.o DSWP_5.o DSWP_DEBUG.o DSWP_PRE.o DFAFramework.o DFAValue.o LivenessAnalysis.o Utils.o
	$(CXX) -dylib -flat_namespace -shared -g -O0  $^ $(shell llvm-config --ldflags --libs support) -o $@

test-files/%.o: test-files/%.c
	clang -O0 -c -emit-llvm $< -o $@
test-files/%.o: test-files/%.cpp
	clang++ -O0 -c -emit-llvm $< -o $@
test-files/%-DSWP.o: test-files/%-m2r.o DSWP.so
	opt -load ./DSWP.so -dswp $< -o $@

tidy:
	rm -f *.o dag partition showgraph

clean: tidy
	rm -f DSWP.so
