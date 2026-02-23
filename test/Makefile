OUT = ../out
OBJDIR = obj

# Test sources
TESTS = $(wildcard test_*.c)
TEST_BINS = $(TESTS:test_%.c=$(OUT)/test_%)

# Library sources needed for tests
LIB_SRCS = $(wildcard ../src/*.c)
LIB_OBJS = $(LIB_SRCS:../src/%.c=$(OBJDIR)/%.o)

# Inter library sources
INTER_SRCS = $(wildcard ../inter/lib_*.c) ../inter/inter_tools.c
INTER_OBJS = $(INTER_SRCS:../inter/%.c=$(OBJDIR)/%.o)

# Test objects
TEST_OBJS = $(TESTS:test_%.c=$(OBJDIR)/test_%.o)

# All objects
OBJS = $(LIB_OBJS) $(INTER_OBJS) $(TEST_OBJS)

# Compiler flags
CFLAGS = $(CFLAGS_BASE) -I. -I../src -I../inter

all: $(TEST_BINS)

# Build each test executable
$(OUT)/test_%: $(OBJDIR)/test_%.o $(LIB_OBJS) $(INTER_OBJS)
	$(CC) -o $@ $^

# Build test object files
$(OBJDIR)/test_%.o: test_%.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

# Build library object files
$(OBJDIR)/%.o: ../src/%.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

# Build inter library object files
$(OBJDIR)/%.o: ../inter/%.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -rf $(OBJDIR)
	rm -f $(OUT)/test_*

.PHONY: all clean
