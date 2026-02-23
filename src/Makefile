SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

CFLAGS = $(CFLAGS_BASE) -I. -fPIC

OUT = ../$(XVR_OUTDIR)

all: $(OUT)/libxvr.a $(OUT)/libxvr.so

$(OUT)/libxvr.a: $(OBJS)
	$(AR) crs $@ $^
	$(RANLIB) $@

$(OUT)/libxvr.so: $(OBJS)
	$(CC) -shared -fPIC -o $@ $^

clean:
	rm -f $(OBJS) $(OUT)/libxvr.a $(OUT)/libxvr.so

.PHONY: all clean
