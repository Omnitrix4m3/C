lib=allocator.so

# Set the following to '0' to disable log messages:
LOGGER ?= 1

CFLAGS += -Wall -g -pthread -fPIC -shared
LDFLAGS +=

$(lib): allocator.c allocator.h logger.h
	$(CC) $(CFLAGS) $(LDFLAGS) -DLOGGER=$(LOGGER) allocator.c -o $@

docs: Doxyfile
	doxygen

clean:
	rm -f $(lib)
	rm -rf docs


# Tests --

test: $(lib) ./tests/run_tests
	@DEBUG="$(debug)" ./tests/run_tests $(run)

testupdate: testclean test

./tests/run_tests:
	rm -rf tests
	git clone https://github.com/USF-OS/Memory-Allocator-Tests.git tests

testclean:
	rm -rf tests
