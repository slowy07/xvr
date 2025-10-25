CC=gcc
CFLAGS+=-std=c17 -g -Wall -Werror -Wextra -Wpedantic -Wformat=2 -Wno-newline-eof
LIBS+=-lm -lXvr
LDFLAGS+=-Wl,-rpath,'$$ORIGIN'

#directories
INTER_ROOTDIR=..
INTER_INTERDIR=.
INTER_SOURCEDIR=$(INTER_ROOTDIR)/src

INTER_OUTDIR=$(INTER_ROOTDIR)/out
INTER_OBJDIR=obj

#file names
INTER_INTERFILES=$(wildcard $(INTER_INTERDIR)/*.c)
INTER_OBJFILES=$(addprefix $(INTER_OBJDIR)/,$(notdir $(INTER_INTERFILES:.c=.o)))
INTER_TARGETNAME=xvr

#linker fix
LDFLAGS+=-L$(realpath $(INTER_OUTDIR))

#build the object files, compile the test cases, and run
all: build link

#targets for each step
.PHONY: build
build: $(INTER_OBJDIR) $(INTER_OBJFILES)

.PHONY: link
link: $(INTER_OUTDIR) $(INTER_OUTDIR)/$(INTER_TARGETNAME)

#util targets
$(INTER_OUTDIR):
	mkdir $(INTER_OUTDIR)

$(INTER_OBJDIR):
	mkdir $(INTER_OBJDIR)

#compilation steps
$(INTER_OBJDIR)/%.o: $(INTER_INTERDIR)/%.c
	$(CC) -c -o $@ $< $(addprefix -I,$(INTER_INTERDIR)) $(addprefix -I,$(INTER_SOURCEDIR)) $(CFLAGS)

$(INTER_OUTDIR)/$(INTER_TARGETNAME): $(INTER_OBJFILES)
	$(CC) -DXVR_IMPORT $(CFLAGS) -o $@ $(INTER_OBJFILES) $(LDFLAGS) $(LIBS)
ifeq ($(shell uname),Darwin) #dylib fix
	otool -L $@
	install_name_tool -add_rpath @executable_path/. $@
	install_name_tool -change $(INTER_OUTDIR)/libXvr.dylib @executable_path/libXvr.dylib $@
	otool -L $@
endif
