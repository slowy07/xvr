SRCS = $(wildcard *.c) $(wildcard backend/*.c)
OBJS = $(SRCS:.c=.o)

CFLAGS = $(CFLAGS_BASE) -I. -Ibackend -fPIC $(shell llvm-config --cflags)

LDFLAGS = $(shell llvm-config --libs core bitwriter)

OUT = ../$(XVR_OUTDIR)

all: $(OUT)/libxvr.a $(OUT)/libxvr.so

$(OUT)/libxvr.a: $(OBJS)
	$(AR) crs $@ $^
	$(RANLIB) $@

$(OUT)/libxvr.so: $(OBJS)
	$(CC) -shared -fPIC -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(OUT)/libxvr.a $(OUT)/libxvr.so

.PHONY: all clean
