SRCS = $(wildcard *.c) $(wildcard backend/*.c)
OBJS = $(SRCS:.c=.o)

LLVM_CONFIG := $(shell command -v llvm-config-21 || command -v llvm-config)
CFLAGS = $(CFLAGS_BASE) -I. -Ibackend -fPIC $(shell $(LLVM_CONFIG) --cflags)

LDFLAGS = $(shell $(LLVM_CONFIG) --libs core bitwriter)

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
