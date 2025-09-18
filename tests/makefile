CFLAGS=-g -Wall -Werror -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
LIBS=-lm

TEST_SOURCEDIR=../$(XVR_SOURCEDIR)
TEST_CASEDIR=cases

TEST_OUTDIR=out
TEST_OBJDIR=obj

TEST_SOURCEFILES=$(wildcard $(TEST_SOURCEDIR)/*.c)
TEST_CASESFILES=$(wildcard $(TEST_CASEDIR/*.c))

all: clean build-source build-cases $(addprefix $(TEST_OUTDIR)/,$(notdir $(TEST_CASESFILES:%.c=%.exe)))

.PHONY: build-source
build-source: $(TEST_OUTDIR) $(TEST_OBJDIR) $(addprefix $(TEST_OBJDIR)/,$(notdir $(TEST_SOURCEFILES:.c=.o)))

.PHONY: build-cases
build-cases: $(TEST_OUTDIR) $(TEST_OBJDIR) $(addprefix $(TEST_OBJDIR)/,$(notdir $(TEST_CASESFILES:.c=.o)))

$(TEST_OUTDIR):
	mkdir $(TEST_OUTDIR)

$(TEST_OBJDIR):
	mkdir $(TEST_OBJDIR)


$(TEST_OBJDIR)/%.o: $(TEST_SOURCEDIR)/%.c
	$(CC) -c -o $@ $< $(addprefix -I,$(TEST_SOURCEDIR)) $(CFLAGS) -fdata-sections -ffunction-sections

$(TEST_OBJDIR)/%.o: $(TEST_CASESDIR)/%.c
	$(CC) -c -o $@ $< $(addprefix -I,$(TEST_SOURCEDIR) $(TEST_CASESDIR)) $(CFLAGS) -fdata-sections -ffunction-sections

$(TEST_OUTDIR)/%.exe: $(TEST_OBJDIR)/%.o
	@$(CC) -o $@ $(shell find $(TEST_OBJDIR) -name '*.o') $(CFLAGS) $(LIBS) -Wl,--gc-sections
	$@

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
	del *.o *.a *.exe *.dll *.lib *.so *.dylib
	del /s out
	del /s obj
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
	@echo "deletion failed - what platform is this man?"
endif
