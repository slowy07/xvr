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
	$(CC) -DXVR_IMPORT $(CFLAGS) -o $(OUT) $(OBJ) -L$(realpath $(shell pwd)/../$(XVR_OUTDIR)) $(LIBS)

release: all
	strip $(OUT)

$(OBJ): | $(ODIR)

$(ODIR):
	mkdir $(ODIR)

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm -rf $(ODIR)
