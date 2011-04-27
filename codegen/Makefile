simple_sync: queue.o simple_sync.o
	@

tests: queue.o test.o
	gcc -pthread -I . queue.o tests/test.o -o tests/test

simple_sync.o:
	gcc -pthread -c -I . simple_sync.c -o simple_sync.o

queue.o:
	gcc -pthread -c -I . queue.c -o queue.o

test.o:
	gcc -pthread -c -I . tests/test.c -o tests/test.o

clean:
	rm *.o
	rm tests/*.o
	rm tests/test
