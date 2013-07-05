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
LINKFLAGS		+= -fprofile-arcs
endif

LIBFLAGS		= -fPIC $(FLAGS)

CFLAGS		+= -std=c11

INCLUDE		+= $(addprefix -I,$(include_))
LIBS		+= $(addprefix -l,$(libs_))
LIBDIRS		+= $(addprefix -L,$(subst :, ,$(libdirs_)))

objects		:= $(patsubst src/%.c,src/%.o,$(wildcard src/*.c))
tests		:= $(patsubst tests/%.c,tests/test.%,$(wildcard tests/*.c))

build : $(objects)
	$(CC) $(LINKFLAGS) $^ $(LIBDIRS) $(LIBS)

src/%.o : $(ROOT)/src/%.c
	cd src; \
	$(CC) $(CFLAGS) $(LIBFLAGS) $(INCLUDE) -c $<

test : build $(tests)

test-run : test
	export LD_LIBRARY_PATH=../; \
	cd tests; \
	./test.* ;

coverage : FORCE
	make test-run COVERAGE=1; \
	lcov --capture --directory . --output-file cov/$(LIBNAME).info; \
	genhtml cov/$(LIBNAME).info --output-directory cov ;

tests/test.% : $(ROOT)/tests/%.c
	cd tests; \
	$(CC) $(CFLAGS) $(FLAGS) $(INCLUDE) -o $(ROOT)/$@ $< $(LIBDIRS) -L.. -lcheck -l$(LIBNAME)

doc : FORCE
	$(DOCTOOL) $(DOCFLAGS) `find src -name *.[c]`

clean : FORCE
	rm -f lib$(LIBNAME).so; \
	rm -f `find src -name "*.o"`; \
	rm -f `find tests -name test.*`; \
	rm -f `find . -name '*.gcda'`; \
	rm -f `find . -name '*.gcno'`;

FORCE :
