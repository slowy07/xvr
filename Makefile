# INFO:
#   make              - Build debug version (default)
#   make release      - Build optimized release version
#   make test         - Run tests
#   make install      - Install libraries and binaries
#   make uninstall    - Remove installed files
#   make clean        - Remove build artifacts

XVR_UNAME := $(shell uname -s 2>/dev/null)

ifeq ($(findstring Linux,$(XVR_UNAME)),Linux)
    XVR_PLATFORM := linux
else ifeq ($(findstring Darwin,$(XVR_UNAME)),Darwin)
    XVR_PLATFORM := darwin
else ifeq ($(findstring CYGWIN,$(XVR_UNAME)),CYGWIN)
    XVR_PLATFORM := cygwin
else ifeq ($(findstring MINGW,$(XVR_UNAME)),MINGW)
    XVR_PLATFORM := windows
else ifeq ($(findstring MSYS,$(XVR_UNAME)),MSYS)
    XVR_PLATFORM := windows
else
    XVR_PLATFORM := unknown
endif

ifeq ($(XVR_PLATFORM),windows)
    XVR_SHLIB_EXT := dll
    XVR_STATIC_EXT := a
    XVR_EXE_EXT := .exe
else ifeq ($(XVR_PLATFORM),darwin)
    XVR_SHLIB_EXT := dylib
    XVR_STATIC_EXT := a
    XVR_EXE_EXT :=
else
    XVR_SHLIB_EXT := so
    XVR_STATIC_EXT := a
    XVR_EXE_EXT :=
endif

XVR_OUTDIR := out
XVR_OBJDIR := obj

XVR_SHLIB_NAME := libxvr.$(XVR_SHLIB_EXT)
XVR_STATIC_NAME := libxvr.$(XVR_STATIC_EXT)
XVR_EXE_NAME := xvr$(XVR_EXE_EXT)

CC ?= cc
AR ?= ar
STRIP ?= strip
RANLIB ?= ranlib

CFLAGS_BASE := -std=c18 -pedantic -Werror -Wall -Wextra
CFLAGS_BASE += -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
CFLAGS_BASE += -Wstrict-prototypes -g

XVR_BUILD_TYPE ?= debug

ifeq ($(XVR_BUILD_TYPE),release)
    CFLAGS_BASE := $(filter-out -g,$(CFLAGS_BASE))
    CFLAGS_BASE += -O2
    XVR_STRIP := $(STRIP)
else
    XVR_STRIP := :
endif

export CC AR STRIP RANLIB
export XVR_PLATFORM XVR_OUTDIR XVR_OBJDIR
export XVR_SHLIB_NAME XVR_STATIC_NAME XVR_EXE_NAME
export XVR_SHLIB_EXT XVR_STATIC_EXT XVR_EXE_EXT
export CFLAGS_BASE XVR_BUILD_TYPE XVR_STRIP
.PHONY: all release clean test install uninstall rebuild

all: out-dirs lib compiler
	@echo "Build complete: $(XVR_EXE_NAME), $(XVR_SHLIB_NAME), $(XVR_STATIC_NAME)"

release: XVR_BUILD_TYPE = release
release: out-dirs lib compiler
	@echo "Release build complete"

out-dirs:
	mkdir -p $(XVR_OUTDIR)

lib: out-dirs
	$(MAKE) -C src

compiler: out-dirs
	$(MAKE) -C compiler

test: clean out-dirs
	$(MAKE) -C test
	@echo ""
	@failed=0; \
	for f in $(XVR_OUTDIR)/test_ast_node $(XVR_OUTDIR)/test_compiler $(XVR_OUTDIR)/test_lexer $(XVR_OUTDIR)/test_literal $(XVR_OUTDIR)/test_memory $(XVR_OUTDIR)/test_opaque $(XVR_OUTDIR)/test_parser $(XVR_OUTDIR)/test_scope $(XVR_OUTDIR)/test_llvm_backend; do \
		if [ -x "$$f" ] && [ -f "$$f" ]; then \
			echo ""; \
			echo "Running: $$f"; \
			if "$$f"; then \
				echo "PASS: $$f"; \
			else \
				echo "FAIL: $$f (exit code: $$?)"; \
				failed=1; \
			fi; \
		fi; \
	done; \
	echo ""; \
	if [ $$failed -eq 0 ]; then \
		echo "tests passed!"; \
	else \
		echo "tests failed!"; \
		exit 1; \
	fi

test-all: clean out-dirs
	$(MAKE) -C test
	@echo ""
	@failed=0; \
	for f in $(XVR_OUTDIR)/test_ast_node $(XVR_OUTDIR)/test_compiler $(XVR_OUTDIR)/test_lexer $(XVR_OUTDIR)/test_literal $(XVR_OUTDIR)/test_memory $(XVR_OUTDIR)/test_opaque $(XVR_OUTDIR)/test_parser $(XVR_OUTDIR)/test_scope $(XVR_OUTDIR)/test_llvm_backend $(XVR_OUTDIR)/test_all; do \
		if [ -x "$$f" ] && [ -f "$$f" ]; then \
			echo ""; \
			echo "Running: $$f"; \
			if "$$f"; then \
				echo "PASS: $$f"; \
			else \
				echo "FAIL: $$f (exit code: $$?)"; \
				failed=1; \
			fi; \
		fi; \
	done; \
	echo ""; \
	if [ $$failed -eq 0 ]; then \
		echo "ALL TESTS PASSED!"; \
	else \
		echo "SOME TESTS FAILED!"; \
		exit 1;
	fi

test-verbose: test-all

install: install-libs install-bin
	@echo "Installation complete"

install-libs: lib shlib
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 $(XVR_OUTDIR)/$(XVR_SHLIB_NAME) $(DESTDIR)$(LIBDIR)/
	install -m 644 $(XVR_OUTDIR)/$(XVR_STATIC_NAME) $(DESTDIR)$(LIBDIR)/

install-bin: compiler
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(XVR_OUTDIR)/$(XVR_EXE_NAME) $(DESTDIR)$(BINDIR)/

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(XVR_EXE_NAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(XVR_SHLIB_NAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(XVR_STATIC_NAME)

clean:
	rm -rf $(XVR_OUTDIR)
	$(MAKE) -C src clean
	$(MAKE) -C compiler clean
	$(MAKE) -C test clean

rebuild: clean all
