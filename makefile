CFLAGS+=-std=c17 -pedantic -Werror
LIBS=-lm

export XVR_SOURCEDIR=src
export XVR_OUTDIR=out
export XVR_OBJDIR=obj

export XVR_SOURCEFILES=$(wildcard $(XVR_SOURCEDIR)/*.c)

all: clean tests
	@echo no targets ready

.PHONY: src
src:
	$(MAKE) -C src -k

.PHONY: inter
inter: src
	$(MAKE) -C inter -k

.PHONY: tests
tests:
	$(MAKE) -C tests -k

.PHONY: tests-gdb
tests-gdb:
	$(MAKE) -C tests all-gdb -k

$(XVR_OUTDIR):
	mkdir $(XVR_OUTDIR)

$(XVR_OBJDIR):
	mkdir $(XVR_OBJDIR)

$(XVR_OBJDIR)/%.o: $(XVR_SOURCEDIR)/%.c
	$(CC) -c -o $@ $< $(addprefix -I,$(XVR_SOURCEDIR)) $(CFLAGS)

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
	@echo "Deletion failed - what platform is this?"
endif

.PHONY: rebuild
rebuild: clean all
