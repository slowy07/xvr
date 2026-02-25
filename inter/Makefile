SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

CFLAGS = $(CFLAGS_BASE) -DXVR_IMPORT -DXVR_EXPORT_LLVM -I. -I../src -I../src/backend $(shell llvm-config --cflags)

LDFLAGS = $(shell llvm-config --libs core bitwriter)

OUT = ../$(XVR_OUTDIR)

all: $(OUT)/xvr

$(OUT)/xvr: $(OBJS)
	$(CC) -o $@ $^ $(OUT)/libxvr.a $(LDFLAGS)

clean:
	rm -f $(OBJS) $(OUT)/xvr

.PHONY: all clean
