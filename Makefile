override CFLAGS += -Wall -Wextra -O2 -g -march=haswell -std=gnu11 -Wno-unused-parameter

# uncomment to use the fast gold linker
# LDFLAGS = -use-ld=gold

SRCS    := $(wildcard *.c)
OBJECTS := $(patsubst %.c,%.o,$(SRCS))
HEADERS := $(wildcard *.h *.hpp)
LDFLAGS := -lm

JE_LIB := jevents/libjevents.a
JE_SRC := $(wildcard jevents/*.c jevents/*.h)

bench: $(OBJECTS) $(JE_LIB)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ $(LDFLAGS) -o $@ $(JE_LIB)

%.o: %.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $< 

$(JE_LIB): $(JE_SRC)
	cd jevents && $(MAKE) MAKEFLAGS=

clean:
	rm -f weirdo-main
	rm -f *.o
