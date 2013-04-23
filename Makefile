CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) -g -O0
.PHONY: all runtime/libruntime.a gdb/% valgrind/% time/% \
		tidy clean clean-examples

all: DSWP.so

DSWP.so: DSWP_0.o DSWP_1.o DSWP_2.o DSWP_3.o DSWP_4.o DSWP_5.o DSWP_DEBUG.o \
	     DSWP_PRE.o DFAFramework.o DFAValue.o LivenessAnalysis.o Utils.o \
	     raw_os_ostream.o
	$(CXX) -dylib -flat_namespace -shared -g -O0  $^ -o $@
# We're including raw_os_ostream.o because we can't just link in libLLVMSupport:
# http://lists.cs.uiuc.edu/pipermail/llvmdev/2010-June/032508.html

runtime/libruntime.a:
	$(MAKE) -C runtime libruntime.a

Example/%.bc: Example/%.c
	clang -O0 -c -emit-llvm $< -o $@
Example/%.bc: Example/%.cpp
	clang++ -O0 -c -emit-llvm $< -o $@
Example/%.bc.ll: Example/%.bc
	llvm-dis $< -o $@
Example/%-m2r.bc: Example/%.bc
	opt -mem2reg $< -o $@
Example/%-DSWP.bc: Example/%.bc DSWP.so
	opt -load ./DSWP.so -dswp $< -o $@

Example/%-DSWP.out: Example/%-DSWP.bc runtime/libruntime.a
	clang -pthread $< runtime/libruntime.a -o $@
Example/%.out: Example/%.bc
	clang -O0 $< -o $@

time/%: Example/%.out
	time $<

gdb/%: Example/%.bc DSWP.so
	gdb --args opt -load ./DSWP.so -dswp $< -o /dev/null
valgrind/%: Example/%.bc DSWP.so
	valgrind $(VALGRIND_ARGS) opt -load ./DSWP.so -dswp $< -o /dev/null

tidy:
	rm -f *.o dag partition showgraph
	$(MAKE) -C runtime tidy

clean-examples:
	rm -f Example/*.bc Example/*.bc.ll Example/*.out

clean: tidy clean-examples
	rm -f DSWP.so
	$(MAKE) -C runtime clean
