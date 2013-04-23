CFLAGS += -pthread -I.
LDFLAGS += -pthread

.PHONY: all tidy clean

all: libruntime.a

libruntime.a: queue.o simple_sync.o runtime_debug.o
	ar rcs $@ $^
tests/test: tests/test.o queue.o
tests/sync_test: tests/sync_test.o queue.o simple_sync.o

tidy:
	rm -f *.o tests/*.o

clean: tidy
	rm -f libruntime.a tests/test tests/sync_test
