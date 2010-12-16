SUFFIXES += .d

SRCS = $(shell find . -type f -name \*.c | grep -v '.preprocessed.c$$')
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
PP_OBJS = $(SRCS:.c=.preprocessed.c)
EXE = crystalhd_wrapper.so

CFLAGS = -O2 -pipe -D_GNU_SOURCE -fPIC -pthread -shared -g
ASTYLE_FLAGS = -y --indent-classes --indent-switches --indent-cases --indent-blocks --indent-brackets --indent-namespaces --indent-labels --indent-preprocessor --break-blocks --break-elseifs --pad=paren --pad=paren-out --indent=tab=8 --brackets=linux -n

MAKEDEPEND = gcc -MM $(CFLAGS)

.PHONY: all clean check_functions preprocess
.IGNORE: $(PP_OBJS) preprocess

all: $(EXE)

-include $(DEPS)

$(EXE): $(OBJS)
	gcc $(CFLAGS) $(OBJS) -o $(@)

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(PP_OBJS)
	rm -f $(EXE)

%.d: %.c
	$(MAKEDEPEND) $< > $@

%.o: %.c %.d

check_functions:
	( \
		grep Dts $(SRCS) | sed "s/^[\t]Dts/Dts/" ; \
		cpp /usr/include/libcrystalhd/libcrystalhd_if.h | grep "^Dts.*($$" \
	) | sort

%.preprocessed.c: %.c %.d
	echo '#include "crystalhd_common.h"' > $(@)
	cpp $(<) | tail -n +`echo \`cpp $(<) | nl -b a | grep $(<) | tail -n 1 | cut -f 1 -d '#'\`` | grep -v "^#" | grep -v "^$$" | grep -v "^[[:space:]]*;$$" >> $(@)
	astyle $(ASTYLE_FLAGS) $(@)

$(PP_OBJS:.c=.o): $(PP_OBJS) $(PP_OBJS:.preprocessed.c=.d)

$(EXE:.so=.preprocessed.so): $(PP_OBJS:.c=.o)
	gcc $(CFLAGS) $(PP_OBJS:.c=.o) -o $(@)

preprocess: $(PP_OBJS)
