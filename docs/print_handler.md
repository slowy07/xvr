# Print Handler API

The print handler provides a flexible abstraction for output operations in the XVR interpreter.

## Overview

The `Xvr_PrintHandler` struct encapsulates both:
1. **Output destination** - where output goes (stdout, stderr, file, custom)
2. **Formatting** - newline behavior

## Usage

### Basic Setup

```c
#include "xvr_print_handler.h"

// Create a handler for stdout with newlines
Xvr_PrintHandler handler;
Xvr_printHandlerInit(&handler, Xvr_printHandlerStdout, true);

// Use with interpreter
Xvr_setInterpreterPrintHandler(&interpreter, handler);
```

### Runtime Configuration

Change newline behavior at runtime:

```c
// Enable/disable newlines based on command-line flag
if (Xvr_commandLine.enablePrintNewline == false) {
    Xvr_printHandlerSetNewline(&handler, false);
}
```

Redirect output to a file:

```c
FILE* logFile = fopen("output.log", "w");
Xvr_printHandlerSetOutput(&handler, (Xvr_PrintOutputFn)fprintf, logFile);
```

### Custom Output Function

Create a custom output function:

```c
void myLogFunction(const char* message) {
    // Write to log file, network, GUI, etc.
    logToFile("app.log", message);
}

Xvr_PrintHandler handler;
Xvr_printHandlerInit(&handler, myLogFunction, true);
```

## API Reference

### Types

```c
typedef void (*Xvr_PrintOutputFn)(const char*);

typedef struct {
    Xvr_PrintOutputFn output;    // Function pointer for output
    bool enableNewline;          // Whether to append newline
} Xvr_PrintHandler;
```

### Functions

| Function | Description |
|----------|-------------|
| `Xvr_printHandlerInit` | Initialize handler with output function and newline setting |
| `Xvr_printHandlerSetOutput` | Change output function |
| `Xvr_printHandlerSetNewline` | Change newline behavior |
| `Xvr_printHandlerPrint` | Output a message through the handler |

### Predefined Output Functions

| Function | Description |
|----------|-------------|
| `Xvr_printHandlerStdout` | Output to stdout |
| `Xvr_printHandlerStderr` | Output to stderr |

## Integration with Interpreter

The interpreter contains three handlers:

```c
typedef struct Xvr_Interpreter {
    // ... other fields ...
    Xvr_PrintHandler printHandler;   // print statement output
    Xvr_PrintHandler assertHandler; // assertion failure messages
    Xvr_PrintHandler errorHandler;  // runtime error messages
    // ... other fields ...
} Xvr_Interpreter;
```

Set handlers independently:

```c
Xvr_setInterpreterPrintHandler(&interpreter, printHandler);
Xvr_setInterpreterAssertHandler(&interpreter, assertHandler);
Xvr_setInterpreterErrorHandler(&interpreter, errorHandler);
```

## Legacy API

For backward compatibility, the old function pointer API still works:

```c
// Deprecated but supported
Xvr_setInterpreterPrint(&interpreter, myPrintFn);
```

This automatically creates a handler with the provided function.
