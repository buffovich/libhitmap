ROOT	:= $(abspath .)

libdirs_	= /usr/lib
libs_		=
include_ 	= /usr/include $(ROOT)/src

LIBNAME			= libhitmap
CC				?= gcc
LINKFLAGS		= -Xlinker --no-as-needed -Xlinker -Bdynamic -shared -Xlinker --export-dynamic -o $(LIBNAME).so
FLAGS			= -fPIC -Wall -Wextra

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

build : $(objects)
	$(CC) $(LINKFLAGS) $(LIBDIRS) $(LIBS) $^

src/%.o : $(ROOT)/src/%.c
	cd src; $(CC) $(CFLAGS) $(FLAGS) $(INCLUDE) -c $<

doc : FORCE
	$(DOCTOOL) $(DOCFLAGS) `find src -name *.[c]`

clean : FORCE
	rm -f $(LIBNAME).so; rm -f `find src -name "*.o"`; rm src/$(CONFIG_H)

FORCE :
