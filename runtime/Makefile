simple_sync: queue.o simple_sync.o
	@

tests: queue.o test.o simple_sync.o sync_test.o
	llvm-gcc -pthread -I . -emit-llvm queue.o tests/test.o -o tests/test
	llvm-gcc -pthread -I . -emit-llvm queue.o simple_sync.o tests/sync_test.o -o tests/sync_test

sync_test.o:
	llvm-gcc -pthread -c -I . -emit-llvm tests/sync_test.c -o tests/sync_test.o

simple_sync.o:
	llvm-gcc -pthread -c -I . -emit-llvm  simple_sync.c -o simple_sync.o

queue.o:
	llvm-gcc -pthread -c -I . -emit-llvm queue.c -o queue.o

test.o:
	llvm-gcc -pthread -c -I . -emit-llvm tests/test.c -o tests/test.o

clean:
	rm *.o
	rm tests/*.o
	rm tests/test
