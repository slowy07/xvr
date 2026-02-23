SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

CFLAGS = $(CFLAGS_BASE) -DXVR_IMPORT -I. -I../src

OUT = ../$(XVR_OUTDIR)

all: $(OUT)/xvr

$(OUT)/xvr: $(OBJS)
	$(CC) -o $@ $^ $(OUT)/libxvr.a

clean:
	rm -f $(OBJS) $(OUT)/xvr

.PHONY: all clean
