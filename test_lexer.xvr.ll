; ModuleID = 'test_lexer'
source_filename = "test_lexer"

@fmt_str = private unnamed_addr constant [7 x i8] c"C: %d\0A\00", align 1

define i32 @main() {
entry:
  %printf_call = call i32 (ptr, i32, ...) @printf(ptr @fmt_str, i32 0)
  ret i32 0
}

declare i32 @printf(...)
