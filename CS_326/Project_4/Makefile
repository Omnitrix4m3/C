bin=www

# Set the following to '0' to disable log messages:
LOGGER ?= 1

# Compiler/linker flags
CFLAGS += -Wall -g -pthread -fPIC
LDFLAGS +=

src=www.c
obj=$(src:.c=.o)

all: $(bin) libwww.so

$(bin): $(obj)
	$(CC) $(CFLAGS) $(LDFLAGS) $(obj) -o $@

libwww.so: $(obj)
	$(CC) $(CFLAGS) $(LDFLAGS) $(obj) -shared -o $@

docs: Doxyfile
	doxygen

clean:
	rm -f $(bin) libwww.so
	rm -rf docs


# Tests --

test: $(lib) ./tests/run_tests
	@DEBUG="$(debug)" ./tests/run_tests $(run)

testupdate: testclean test

./tests/run_tests:
	rm -rf tests
	git clone https://github.com/USF-OS/Web-Server-Tests.git tests

testclean:
	rm -rf tests
