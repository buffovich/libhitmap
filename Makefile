ROOT	:= $(abspath .)

libdirs_	= /usr/lib
libs_		=
include_ 	= /usr/include $(ROOT)/src

LIBNAME			= hitmap
CC				?= gcc
LINKFLAGS		= -Xlinker --no-as-needed -Xlinker -Bdynamic -shared -Xlinker --export-dynamic -o lib$(LIBNAME).so
FLAGS			= -Wall -Wextra

ifdef DEBUG
DEBUG_FORMAT	?= gdb
FLAGS			+= -O0 -g -g$(DEBUG_FORMAT)$(DEBUG)
else
OPTIMIZE		?= 3
FLAGS			+= -O$(OPTIMIZE)
LINKFLAGS		+= -s
endif

ifdef COVERAGE
FLAGS			+= -fprofile-arcs -ftest-coverage
LINKFLAGS		+= -fprofile-arcs -ftest-coverage
endif

LIBFLAGS		= -fPIC $(FLAGS)

CFLAGS		+= -std=c11

INCLUDE		+= $(addprefix -I,$(include_))
LIBS		+= $(addprefix -l,$(libs_))
LIBDIRS		+= $(addprefix -L,$(subst :, ,$(libdirs_)))

dynamic_objects		:= $(patsubst src/%.c,src/dynamic_%.o,$(wildcard src/*.c))
static_objects	:= $(patsubst src/%.c,src/static_%.o,$(wildcard src/*.c))
tests		:= $(patsubst tests/%.c,tests/test.%,$(wildcard tests/*.c))
benchmarks	:= $(patsubst benchmarks/%.c,benchmarks/bench.%,$(wildcard benchmarks/*.c))

build : build_dynamic build_static

build_dynamic : $(dynamic_objects)
	$(CC) $(LINKFLAGS) $^ $(LIBDIRS) $(LIBS);

build_static : $(static_objects)
	ar -r lib$(LIBNAME).a $^

test : build_dynamic $(tests)
	export LD_LIBRARY_PATH=../; \
	cd tests; \
	./test.* ;

memcheck : build_dynamic $(tests)
	export LD_LIBRARY_PATH=..; \
	cd tests; \
	for F in test.*; do \
		valgrind --tool=memcheck --trace-children=yes --log-file=./$$F.memcheck --leak-check=full ./$$F; \
	done;

coverage : FORCE
	make test COVERAGE=1; \
	lcov --capture --directory . --output-file cov/$(LIBNAME).info; \
	genhtml cov/$(LIBNAME).info --output-directory cov ;

benchmark : build_static $(benchmarks)
	export LD_LIBRARY_PATH=../; \
	cd benchmarks; \
	for F in bench.*; do \
		valgrind --tool=callgrind --branch-sim=yes --cache-sim=yes --simulate-hwpref=yes --cacheuse=yes --callgrind-out-file=./$$F.callgrind ./$$F; \
	done;

src/dynamic_%.o : $(ROOT)/src/%.c
	cd src; \
	$(CC) $(CFLAGS) $(LIBFLAGS) $(INCLUDE) -o $(ROOT)/$@ -c $<;

src/static_%.o : $(ROOT)/src/%.c
	cd src; \
	$(CC) $(CFLAGS) $(FLAGS) $(INCLUDE) -o $(ROOT)/$@ -c $<;

tests/test.% : $(ROOT)/tests/%.c
	cd tests; \
	$(CC) $(CFLAGS) $(FLAGS) $(INCLUDE) -o $(ROOT)/$@ $< $(LIBDIRS) -L.. -lcheck -l$(LIBNAME);

benchmarks/bench.% : $(ROOT)/benchmarks/%.c
	cd benchmarks; \
	$(CC) -static $(CFLAGS) $(FLAGS) $(INCLUDE) -o $(ROOT)/$@ $< $(LIBDIRS) -L.. -l$(LIBNAME);

doc : FORCE
	doxygen

clean : FORCE
	rm -f lib$(LIBNAME).so; \
	rm -f lib$(LIBNAME).a; \
	rm -f `find src -name "*.o"`; \
	rm -f `find tests -name test.*`; \
	rm -f `find benchmarks -name bench.*`; \
	rm -f `find . -name '*.gcda'`; \
	rm -f `find . -name '*.gcno'`; \
	rm -rf ./cov/* ; \
	rm -rf ./doc/html ; \
	rm -rf ./tests/*.memcheck; \
	rm -rf ./benchmarks/*.callgrind.*

FORCE :
