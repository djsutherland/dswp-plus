CFLAGS += -pthread -I.
LDFLAGS += -pthread

.PHONY: all tidy clean

all: queue.o simple_sync.o
	@

tests/test: tests/test.o queue.o

tests/sync_test: tests/sync_test.o queue.o simple_sync.o

tidy:
	rm -f *.o tests/*.o

clean: tidy
	rm -f tests/test tests/sync_test
