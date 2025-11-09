CC=gcc

IDIR+=. ../src
CFLAGS+=$(addprefix -I,$(IDIR)) -g -Wall -W -Wno-unused-parameter -Wno-unused-function -Wstrict-prototypes
LIBS+=-lxvr

ODIR = obj
SRC = $(wildcard *.c)
OBJ = $(addprefix $(ODIR)/,$(SRC:.c=.o))
OUTNAME=xvr
OUT=../$(XVR_OUTDIR)/xvr

all: $(OBJ)
ifeq ($(shell uname),Darwin)
	cp $(PWD)/$(XVR_OUTDIR)/lib$(OUTNAME).dylib /usr/local/lib/
	$(CC) -DXVR_IMPORT $(CFLAGS) -o $(OUT) $(OBJ) $(LIBS)
else
	$(CC) -DXVR_IMPORT $(CFLAGS) -o $(OUT) $(OBJ) -Wl,-rpath,. -L$(realpath $(shell pwd)/../$(XVR_OUTDIR)) $(LIBS)
endif

release: all
	strip $(OUT)

$(OBJ): | $(ODIR)

$(ODIR):
	mkdir $(ODIR)

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	$(RM) $(ODIR)
	rm /usr/local/lib/lib$(OUTNAME).dylib
