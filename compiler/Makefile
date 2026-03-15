SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

LLVM_CONFIG := $(shell command -v llvm-config-21 || command -v llvm-config)
CFLAGS = $(CFLAGS_BASE) -DXVR_IMPORT -DXVR_EXPORT_LLVM -I. -I../src -I../src/backend $(shell $(LLVM_CONFIG) --cflags)

LDFLAGS = $(shell $(LLVM_CONFIG) --libs core bitwriter)

OUT = ../$(XVR_OUTDIR)

all: $(OUT)/xvr

$(OUT)/xvr: $(OBJS)
	$(CC) -o $@ $^ $(OUT)/libxvr.a $(LDFLAGS)

clean:
	rm -f $(OBJS) $(OUT)/xvr

.PHONY: all clean
