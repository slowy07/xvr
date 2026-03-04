# Test Makefile

OUT = ../out
OBJDIR = obj

# LLVM configuration
LLVM_CONFIG := $(shell command -v llvm-config-21 || command -v llvm-config)
LLVM_CFLAGS = $(shell $(LLVM_CONFIG) --cflags)
LLVM_LIBS = $(shell $(LLVM_CONFIG) --libs core bitwriter)

# Compiler flags
CFLAGS = -std=c18 -pedantic -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wstrict-prototypes -g -I. -I../src -I../src/backend -I../inter $(LLVM_CFLAGS)
LDFLAGS = $(LLVM_LIBS)

# All test object files for test_all
TEST_OBJECTS = $(OBJDIR)/test_all.o \
               $(OBJDIR)/test_ast_node.o \
               $(OBJDIR)/test_lexer.o \
               $(OBJDIR)/test_parser.o \
               $(OBJDIR)/test_compiler.o \
               $(OBJDIR)/test_literal.o \
               $(OBJDIR)/test_memory.o \
               $(OBJDIR)/test_opaque.o \
               $(OBJDIR)/test_scope.o \
               $(OBJDIR)/test_llvm_backend.o \
               $(OBJDIR)/test_interpreter.o

# compilere library object files (excluding main_compiler.o which has main)
COMPILER_OBJS = ../compiler/compiler_tools.o ../compiler/lib_about.o \
             ../compiler/lib_runner.o ../compiler/lib_standard.o ../compiler/lib_timer.o

# Individual test executables
ALL_TESTS = $(OUT)/test_all

# Build library first
all: ../out/libxvr.a ../out/libxvr.so test_all

../out/libxvr.a ../out/libxvr.so:
	$(MAKE) -C ..

# Combined test_all
test_all: $(OUT)/test_all

$(OUT)/test_all: $(TEST_OBJECTS) $(COMPILER_OBJS) ../out/libxvr.a
	$(CC) -o $@ $(TEST_OBJECTS) $(COMPILER_OBJS) ../out/libxvr.a $(LDFLAGS)

$(OBJDIR)/test_all.o: test_all.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/test_ast_node.o: test_ast_node.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/test_lexer.o: test_lexer.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/test_parser.o: test_parser.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/test_compiler.o: test_compiler.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/test_literal.o: test_literal.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/test_memory.o: test_memory.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/test_opaque.o: test_opaque.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/test_scope.o: test_scope.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/test_llvm_backend.o: test_llvm_backend.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/test_interpreter.o: test_interpreter.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -rf $(OBJDIR)
	rm -f $(OUT)/test_all

.PHONY: all test_all clean
