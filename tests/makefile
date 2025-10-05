CC=gcc
CFLAGS=-std=c17 -g -Wall -Werror -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
LIBS=-lm
LDFLAGS+=

ifeq ($(shell uname),Linux)
LDFLAGS=-Wl,--gc-sections
else ifeq ($(OS),Windows_NT)
LDFLAGS=-Wl,--gc-sections
else ifeq ($(shell uname),Darwin)
LDFLAGS=-Wl,-dead_strip
else
	@echo "LDFLAGS set failed - are you using temple os?"
endif

TEST_ROOTDIR=..
TEST_SOURCEDIR=$(TEST_ROOTDIR)/$(XVR_SOURCEDIR)
TEST_CASESDIR=cases

TEST_OUTDIR=out
TEST_OBJDIR=obj

TEST_SOURCEFILES=$(addprefix $(XVR_ROOTDIR)/,$(XVR_SOURCEFILES))
TEST_CASESFILES=$(wildcard $(TEST_CASESDIR)/*.c)

all: clean build-source build-cases build-link build-run

.PHONY: build-source
build-source: $(TEST_OUTDIR) $(TEST_OBJDIR) $(addprefix $(TEST_OBJDIR)/,$(notdir $(TEST_SOURCEFILES:.c=.o)))

.PHONY: build-cases
build-cases: $(TEST_OUTDIR) $(TEST_OBJDIR) $(addprefix $(TEST_OBJDIR)/,$(notdir $(TEST_CASESFILES:.c=.o)))

.PHONY: build-link
build-link: $(TEST_OUTDIR) $(TEST_OBJDIR) $(addprefix $(TEST_OUTDIR)/,$(notdir $(TEST_CASESFILES:%.c=%.exe)))

.PHONY: build-run
build-run: $(addprefix $(TEST_OUTDIR)/,$(notdir $(TEST_CASESFILES:%.c=%.exe))) $(addprefix $(TEST_OUTDIR)/,$(notdir $(TEST_CASESFILES:%.c=%.run)))

$(TEST_OUTDIR):
	mkdir $(TEST_OUTDIR)

$(TEST_OBJDIR):
	mkdir $(TEST_OBJDIR)

$(TEST_OBJDIR)/%.o: $(TEST_SOURCEDIR)/%.c
	$(CC) -c -o $@ $< $(addprefix -I,$(TEST_SOURCEDIR)) $(CFLAGS) -fdata-sections -ffunction-sections

$(TEST_OBJDIR)/%.o: $(TEST_CASESDIR)/%.c
	$(CC) -c -o $@ $< $(addprefix -I,$(TEST_SOURCEDIR) $(TEST_CASESDIR)) $(CFLAGS) -fdata-sections -ffunction-sections

$(TEST_OUTDIR)/%.exe: $(TEST_OBJDIR)/%.o
	@$(CC) -o $@ $< $(addprefix $(TEST_OBJDIR)/,$(notdir $(TEST_SOURCEFILES:.c=.o))) $(CFLAGS) $(LIBS) $(LDFLAGS)

.PRECIOUS: $(TEST_OUTDIR)/%.run
$(TEST_OUTDIR)/%.run: $(TEST_OUTDIR)/%.exe
	$<

all-gdb: clean build-source build-cases build-link build-run-gdb

.PHONY: build-run-gdb
build-run-gdb: $(addprefix $(TEST_OUTDIR)/,$(notdir $(TEST_CASESFILES:%.c=%.exe))) $(addprefix $(TEST_OUTDIR)/,$(notdir $(TEST_CASESFILES:%.c=%.run-gdb)))

.PRECIOUS: $(TEST_OUTDIR)/%.run-gdb
$(TEST_OUTDIR)/%.run-gdb: $(TEST_OUTDIR)/%.exe
	gdb $< -ex "run" --batch

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
	@echo "Deletion failed - what platform is this?"
endif
