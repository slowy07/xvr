# LLVM AOT Compiler Sources
# Excludes interpreter/bytecode files

SRCS = xvr_ast_node.c \
       xvr_cast_emit.c \
       xvr_common.c \
       xvr_format_string.c \
       xvr_keyword_types.c \
       xvr_lexer.c \
       xvr_literal.c \
       xvr_literal_array.c \
       xvr_literal_dictionary.c \
       xvr_memory.c \
       xvr_parser.c \
       xvr_print_handler.c \
       xvr_refstring.c \
       xvr_scope.c \
       xvr_semantic.c \
       xvr_type.c \
       xvr_unused.c \
       backend/xvr_llvm_codegen.c \
       backend/xvr_llvm_context.c \
       backend/xvr_llvm_control_flow.c \
       backend/xvr_llvm_expression_emitter.c \
       backend/xvr_llvm_function_emitter.c \
       backend/xvr_llvm_ir_builder.c \
       backend/xvr_llvm_module_manager.c \
       backend/xvr_llvm_optimizer.c \
       backend/xvr_llvm_target.c \
       backend/xvr_llvm_type_mapper.c

OBJS = $(SRCS:.c=.o)

LLVM_CONFIG := $(shell command -v llvm-config-21 || command -v llvm-config)
CFLAGS = $(CFLAGS_BASE) -I. -Ibackend -fPIC $(shell $(LLVM_CONFIG) --cflags)

LDFLAGS = $(shell $(LLVM_CONFIG) --libs core bitwriter)

OUT = ../$(XVR_OUTDIR)

all: $(OUT)/libxvr.a $(OUT)/libxvr.so

$(OUT)/libxvr.a: $(OBJS)
	$(AR) crs $@ $^
	$(RANLIB) $@

$(OUT)/libxvr.so: $(OBJS)
	$(CC) -shared -fPIC -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(OUT)/libxvr.a $(OUT)/libxvr.so

.PHONY: all clean
