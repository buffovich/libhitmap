ROOT	:= $(abspath .)

libdirs_	= /usr/lib
libs_		=
include_ 	= /usr/include $(ROOT)/src

LIBNAME			= libhitmap
CC				?= gcc
LINKFLAGS		= -Xlinker --no-as-needed -Xlinker -Bdynamic -shared -Xlinker --export-dynamic -o $(LIBNAME).so
FLAGS			= -Wall -Wextra
LIBFLAGS		= -fPIC $(FLAGS)

ifdef DEBUG
DEBUG_FORMAT	?= gdb
FLAGS			+= -O0 -g -g$(DEBUG_FORMAT)$(DEBUG)
else
OPTIMIZE		?= 3
FLAGS			+= -O$(OPTIMIZE)
LINKFLAGS		+= -s
endif

CFLAGS		+= -std=c11

INCLUDE		+= $(addprefix -I,$(include_))
LIBS		+= $(addprefix -l,$(libs_))
LIBDIRS		+= $(addprefix -L,$(subst :, ,$(libdirs_)))

objects		:= $(patsubst src/%.c,src/%.o,$(wildcard src/*.c))
tests		:= $(patsubst tests/%.c,tests/test.%,$(wildcard tests/*.c))

build : $(objects)
	$(CC) $(LINKFLAGS) $^ $(LIBDIRS) $(LIBS)

src/%.o : $(ROOT)/src/%.c
	cd src; $(CC) $(CFLAGS) $(LIBFLAGS) $(INCLUDE) -c $<

test : build $(tests)
	export LD_LIBRARY_PATH=.; \
	./tests/test.* ;

tests/test.% : $(ROOT)/tests/%.c
	$(CC) $(CFLAGS) $(FLAGS) $(INCLUDE) -o $@ $< $(LIBDIRS) -L. -lcheck -lhitmap

doc : FORCE
	$(DOCTOOL) $(DOCFLAGS) `find src -name *.[c]`

clean : FORCE
	rm -f $(LIBNAME).so; rm -f `find src -name "*.o"`; \
	rm -f `find tests -name test.*`

FORCE :
