all: DSWP.so

CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) -g -O0

DSWP.so: DSWP_0.o DSWP_1.o DSWP_2.o DSWP_3.o DSWP_4.o DSWP_5.o DSWP_DEBUG.o DSWP_PRE.o DFAFramework.o DFAValue.o LivenessAnalysis.o Utils.o
	$(CXX) -dylib -flat_namespace -shared -g -O0  $^ $(shell llvm-config --ldflags --libs support) -o $@

Example/%.o: Example/%.c
	clang -O0 -c -emit-llvm $< -o $@
Example/%.o: Example/%.cpp
	clang++ -O0 -c -emit-llvm $< -o $@
Example/%-DSWP.o: Example/%.o DSWP.so
	opt -load ./DSWP.so -dswp $< -o $@
gdb/%: Example/%.o
	gdb --args opt -load ./DSWP.so -dswp $< -o /dev/null

tidy:
	rm -f *.o dag partition showgraph

clean: tidy
	rm -f DSWP.so
