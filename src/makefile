CC=gcc

IDIR+=.
CFLAGS+=$(addprefix -I,$(IDIR)) -g -Wall -W -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wstrict-prototypes
LIBS+=

ODIR = obj
SRC = $(wildcard *.c)
OBJ = $(addprefix $(ODIR)/,$(SRC:.c=.o))

OUTNAME=xvr

ifeq ($(findstring CYGWIN, $(shell uname)),CYGWIN)
	LIBLINE =-Wl,--out-implib=../$(XVR_OUTDIR)/lib$(OUTNAME).dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive $(OBJ) -Wl,--no-whole-archive
	OUT=../$(XVR_OUTDIR)/$(OUTNAME).dll
else ifeq ($(shell uname),Linux)
	LIBLINE=-Wl,--out-implib=../$(XVR_OUTDIR)/lib$(OUTNAME).a -Wl,--whole-archive $(OBJ) -Wl,--no-whole-archive
	OUT=../$(XVR_OUTDIR)/lib$(OUTNAME).so
	CFLAGS += -fPIC
else ifeq ($(OS),Windows_NT)
	LIBLINE =-Wl,--out-implib=../$(XVR_OUTDIR)/lib$(OUTNAME).dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive $(OBJ) -Wl,--no-whole-archive
	OUT=../$(XVR_OUTDIR)/$(OUTNAME).dll
else ifeq ($(shell uname),Darwin)
	LIBLINE = $(OBJ)
	OUT=../$(XVR_OUTDIR)/lib$(OUTNAME).dylib
else
	@echo "Platform test failed - are you using temple os?"
	exit 1
endif

library: $(OBJ)
	$(CC) -DXVR_EXPORT $(CFLAGS) -shared -o $(OUT) $(LIBLINE)

static: $(OBJ)
	ar crs ../$(XVR_OUTDIR)/lib$(OUTNAME).a $(OBJ)

library-release: $(OBJ) library
	strip $(OUT)

static-release: $(OBJ) static
	strip -d ../$(XVR_OUTDIR)/lib$(OUTNAME).a

$(OBJ): | $(ODIR)

$(ODIR):
	mkdir $(ODIR)

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	$(RM) $(ODIR)
