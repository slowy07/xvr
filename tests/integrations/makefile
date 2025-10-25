CC=gcc
CFLAGS+=-std=c17 -g -Wall -Werror -Wextra -Wformat=2 -Wno-newline-eof
LIBS+=-lm
LDFLAGS+=

ifeq ($(shell uname),Linux)
LDFLAGS=-Wl,--gc-sections
else ifeq ($(OS),Windows_NT)
LDFLAGS=-Wl,--gc-sections
else ifeq ($(shell uname),Darwin)
LDFLAGS=-Wl,-dead_strip
else
	@echo "LDFLAGS set failed - what platform is this?"
endif

TEST_ROOTDIR=../..
TEST_SOURCEDIR=$(TEST_ROOTDIR)/$(XVR_SOURCEDIR)
TEST_REPLDIR=$(TEST_ROOTDIR)/$(XVR_REPLDIR)
TEST_SCRIPTDIR=.

TEST_OUTDIR=out
TEST_OBJDIR=obj

TEST_SCRIPTFILES=$(wildcard $(TEST_SCRIPTDIR)/test_*.xvr)
TEST_REPLNAME=repl.exe

all: source repl run

run: $(TEST_SCRIPTFILES:.xvr=.xvr-run)

%.xvr-run: %.xvr
	$(TEST_OUTDIR)/$(TEST_REPLNAME) -f ../$<

gdb: source repl run-gdb

run-gdb: $(TEST_SCRIPTFILES:.xvr=.xvr-gdb)

%.xvr-gdb: %.xvr
	gdb $(TEST_OUTDIR)/$(TEST_REPLNAME) -ix gdb_init -ex=run --batch --return-child-result --args "$(TEST_OUTDIR)/$(TEST_REPLNAME)" "-f" "../$<"

source: $(TEST_OBJDIR) $(TEST_OUTDIR)
	$(MAKE) SRC_OUTDIR=../$(XVR_INTEGRATIONSDIR)/$(TEST_OUTDIR) -C $(TEST_SOURCEDIR)

repl: $(TEST_OBJDIR) $(TEST_OUTDIR) source
	$(MAKE) REPL_TARGETNAME=$(TEST_REPLNAME) REPL_OUTDIR=../$(XVR_INTEGRATIONSDIR)/$(TEST_OUTDIR) -C $(TEST_REPLDIR)

$(TEST_OUTDIR):
	mkdir $(TEST_OUTDIR)

$(TEST_OBJDIR):
	mkdir $(TEST_OBJDIR)
