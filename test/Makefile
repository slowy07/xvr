OUT = ../out
OBJDIR = obj

# LLVM configuration
LLVM_CONFIG := $(shell command -v llvm-config-21 || command -v llvm-config)
LLVM_CFLAGS = $(shell $(LLVM_CONFIG) --cflags)
LLVM_LIBS = $(shell $(LLVM_CONFIG) --libs core bitwriter)

# Compiler flags
CFLAGS = $(CFLAGS_BASE) -I. -I../src -I../src/backend -I../inter $(LLVM_CFLAGS)
LDFLAGS = $(LLVM_LIBS)

# Build library first, then test
all: ../out/libxvr.a ../out/libxvr.so test_llvm

../out/libxvr.a ../out/libxvr.so:
	$(MAKE) -C ..

test_llvm: $(OUT)/test_llvm_backend

$(OUT)/test_llvm_backend: $(OBJDIR)/test_llvm_backend.o
	$(CC) -o $@ $^ ../out/libxvr.a $(LDFLAGS)

$(OBJDIR)/test_llvm_backend.o: test_llvm_backend.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -rf $(OBJDIR)
	rm -f $(OUT)/test_llvm_backend

.PHONY: all test_llvm clean

# make test_llvm && ../out/test_llvm_backend
