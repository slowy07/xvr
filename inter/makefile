CC=gcc
CFLAGS+=-g -Wall -Werror -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
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
INTER_TARGETNAME=inter

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
	install_name_tool -change ../out/libXvr.dylib @executable_path/libXvr.dylib $@
	otool -L $@
endif

#util commands
.PHONY: clean
clean:
ifeq ($(shell uname),Linux)
	find . -type f -name '*.o' -delete
	find . -type f -name '*.a' -delete
	find . -type f -name '*.exe' -delete
	find . -type f -name '*.dll' -delete
	find . -type f -name '*.lib' -delete
	find . -type f -name '*.so' -delete
	find . -type f -name '*.dylib' -delete
	find . -type d -name 'out' -delete
	find . -type d -name 'obj' -delete
else ifeq ($(OS),Windows_NT)
	$(RM) *.o *.a *.exe *.dll *.lib *.so *.dylib
	$(RM) out
	$(RM) obj
else ifeq ($(shell uname),Darwin)
	find . -type f -name '*.o' -delete
	find . -type f -name '*.a' -delete
	find . -type f -name '*.exe' -delete
	find . -type f -name '*.dll' -delete
	find . -type f -name '*.lib' -delete
	find . -type f -name '*.so' -delete
	find . -type f -name '*.dylib' -delete
	find . -type d -name 'out' -delete
	find . -type d -name 'obj' -delete
else
	@echo "del failed"
endif
