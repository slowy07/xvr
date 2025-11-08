CC=gcc

IDIR +=. ../src ../inter
CFLAGS +=$(addprefix -I,$(IDIR)) -g -Wall -W -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
LIBS +=
ODIR = obj
TARGETS = $(wildcard ../src/*.c) $(wildcard ../inter/lib_*.c) ../inter/inter_tools.c
TESTS = $(wildcard test_*.c)
OBJ = $(addprefix $(ODIR)/,$(TARGETS:../src/%.c=%.o)) $(addprefix $(ODIR)/,$(TESTS:.c=.o))

.PRECIOUS:  $(TESTS:%.c=../$(XVR_OUTDIR)/%.exe)

all: $(OBJ) $(TESTS:%.c=../$(XVR_OUTDIR)/%.exe)

../$(XVR_OUTDIR)/%.exe: $(ODIR)/%.o
	@$(CC) -o $@ $< $(TARGETS:../src/%.c=$(ODIR)/%.o) $(CFLAGS) $(LIBS)

$(OBJ): | $(ODIR)

$(ODIR):
	mkdir $(ODIR)

$(ODIR)/%.o: %.c
	@$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: ../src/%.c
	@$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	$(RM) $(ODIR)
