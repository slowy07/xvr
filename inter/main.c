#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_common.h"
#include "xvr_console_colors.h"
#include "xvr_lexer.h"
#include "xvr_module.h"
#include "xvr_module_builder.h"
#include "xvr_module_bundle.h"
#include "xvr_parser.h"
#include "xvr_print.h"
#include "xvr_stack.h"
#include "xvr_string.h"
#include "xvr_value.h"
#include "xvr_vm.h"

#define APPEND(dest, src) \
    strncpy((dest) + (strlen(dest)), (src), strlen((src)) + 1);

#if defined(_WIN32) || defined(_WIN64)
#    define FLIPSLASH(str) \
        for (int i = 0; str[i]; i++) str[i] == '/' ? '\\' : str[i];
#else
#    define FLIPSLASH(str) \
        for (int i = 0; str[i]; i++) str[i] = str[i] == '\\' ? '/' : str[i];
#endif /* if defined (_WIN32) || defined (_WIN64) */

unsigned char* readFile(char* path, int* size) {
    if (path == NULL || size == NULL) {
        return NULL;
    }

    int pathLength = strlen(path);
    char realPath[pathLength + 1];
    strncpy(realPath, path, pathLength);
    realPath[pathLength] = '\0';
    FLIPSLASH(realPath);

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        *size = -1;
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    *size = ftell(file);
    rewind(file);

    unsigned char* buffer = malloc(*size + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    if (fread(buffer, sizeof(unsigned char), *size, file) <
        (unsigned int)(*size)) {
        fclose(file);
        *size = -2;
        return NULL;
    }

    buffer[(*size)++] = '\0';

    fclose(file);
    return buffer;
}

int getFilePath(char* dest, const char* src) {
    char* p = NULL;

    p = strrchr(src, '\\');
    if (p == NULL) {
        p = strchr(src, '/');
    }

    if (p == NULL) {
        int len = strlen(src);
        strncpy(dest, src, len);
        return len;
    }

    int len = p - src + 1;
    strncpy(dest, src, len);
    dest[len] = '\0';

    return len;
}

int getFileName(char* dest, const char* src) {
    char* p = NULL;

    p = strrchr(src, '\\');
    if (p == NULL) {
        p = strrchr(src, '/');
    }

    if (p == NULL) {
        int len = strlen(src);
        strncpy(dest, src, len);
        return len;
    }

    p++;

    int len = strlen(p);
    strncpy(dest, p, len);
    dest[len] = '\0';

    return len;
}

static void printCallback(const char* msg) { fprintf(stdout, "%s\n", msg); }

static void errorAndExitCallback(const char* msg) {
    fprintf(stderr, "Error: %s", msg);
    exit(-1);
}

static void errorAndContinueCallback(const char* msg) {
    fprintf(stderr, "Error: %s\n", msg);
}

static void assertFailureAndExitCallback(const char* msg) {
    fprintf(stderr, "Assert failure: %s\n", msg);
    exit(-1);
}

static void assertFailureAndContinueCallback(const char* msg) {
    fprintf(stderr, "Assert Failure: %s\n", msg);
}

static void noOpCallback(const char* msg) {
    (void)msg;
    // pass
}

static void silentExitCallback(const char* msg) {
    (void)msg;
    exit(-1);
}

typedef struct CmdLine {
    bool error;
    bool help;
    bool version;
    char* infile;
    int infileLength;
    bool silentPrint;
    bool silentAssert;
    bool removeAssert;
    bool verboseDebugPrint;
} CmdLine;

void usageCmdLine(int argc, const char* argv[]) {
    (void)argc;
    printf(
        "Usage: %s [ -h (--help) | -v (--version) | -f (--file) source.xvr "
        "]\n\n",
        argv[0]);
}

void helpCmdLine(int argc, const char* argv[]) {
    usageCmdLine(argc, argv);

    printf("  -h, --help\t\t\tShow this help\n");
    printf("  -v, --version\t\t\tShow version\n");
    printf("  -f, --file infile\t\tParse, compile and run\n");
    printf("      --silent-print\t\tSuppress output from the print keywords\n");
    printf(
        "      --silent-assert\t\tSuppress output from the assert keywords\n");
    printf(
        "      --remove-assert\t\tDo not include the assert statement in the "
        "bytecode\n");
    printf("  -d, --verbose\t\tPrint debug information about Xvr internals\n");
}

void versionCmdLine(int argc, const char* argv[]) {
    (void)argc;
    (void)argv;
    printf("The Xvr Programming Language, Version %d.%d.%d %s\n\n",
           XVR_VERSION_MAJOR, XVR_VERSION_MINOR, XVR_VERSION_PATCH,
           XVR_VERSION_BUILD);
}

CmdLine parseCmdLine(int argc, const char* argv[]) {
    CmdLine cmd = {
        .error = false,
        .help = false,
        .version = false,
        .infile = NULL,
        .infileLength = 0,
        .silentPrint = false,
        .silentAssert = false,
        .removeAssert = false,
        .verboseDebugPrint = false,
    };

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            cmd.help = true;
        }

        else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
            cmd.version = true;
        }

        else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--file")) {
            if (argc < i + 1) {
                cmd.error = true;
            } else {
                if (cmd.infile != NULL) {
                    free(cmd.infile);
                }

                i++;
                cmd.infileLength = strlen(argv[0]) + strlen(argv[i]);
                cmd.infile = malloc(cmd.infileLength + 1);

                if (cmd.infile == NULL) {
                    fprintf(stderr, XVR_CC_ERROR
                            "ERROR: Failed to allocate space while parsing "
                            "the command line, exiting\n" XVR_CC_RESET);
                    exit(-1);
                }

                getFilePath(cmd.infile, argv[0]);
                APPEND(cmd.infile, argv[i]);
                FLIPSLASH(cmd.infile);
            }
        }

        else if (!strcmp(argv[i], "--silent-print")) {
            cmd.silentPrint = true;
        }

        else if (!strcmp(argv[i], "--silent-assert")) {
            cmd.silentAssert = true;
        }

        else if (!strcmp(argv[i], "--remove-assert")) {
            cmd.removeAssert = true;
        }

        else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--verbose")) {
            cmd.verboseDebugPrint = true;
        }

        else {
            cmd.error = true;
        }
    }

    return cmd;
}

int repl(const char* filepath) {
    Xvr_setPrintCallback(printCallback);
    Xvr_setErrorCallback(errorAndContinueCallback);
    Xvr_setAssertFailureCallback(assertFailureAndContinueCallback);

    char prompt[256];
    getFileName(prompt, filepath);
    unsigned int INPUT_BUFFER_SIZE = 4096;
    char inputBuffer[INPUT_BUFFER_SIZE];
    memset(inputBuffer, 0, INPUT_BUFFER_SIZE);

    Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);

    Xvr_VM vm;
    Xvr_initVM(&vm);

    printf("%s >> ", prompt);

    while (fgets(inputBuffer, INPUT_BUFFER_SIZE, stdin)) {
        unsigned int length = strlen(inputBuffer);
        if (inputBuffer[length - 1] == '\n') {
            inputBuffer[--length] = '\0';
        }

        if (length == 0) {
            printf("%s >> ", prompt);
            continue;
        }

        if (strlen(inputBuffer) == 4 &&
            (strncmp(inputBuffer, "exit", 4) == 0 ||
             strncmp(inputBuffer, "quit", 4) == 0)) {
            break;
        }

        Xvr_Lexer lexer;
        Xvr_bindLexer(&lexer, inputBuffer);
        Xvr_Parser parser;
        Xvr_bindParser(&parser, &lexer);
        Xvr_configureParser(&parser, false);
        Xvr_Ast* ast = Xvr_scanParser(&bucket, &parser);

        if (parser.error) {
            printf("%s >> ", prompt);
            continue;
        }

        void* buffer = Xvr_compileModuleBuilder(ast);
        Xvr_Module module = Xvr_parseModule(buffer);
        Xvr_bindVM(&vm, &module);

        Xvr_runVM(&vm);
        Xvr_resetVM(&vm);
        free(buffer);

        printf("%s >> ", prompt);
    }

    Xvr_freeVM(&vm);
    Xvr_freeBucket(&bucket);

    return 0;
}

static void debugStackPrint(Xvr_Stack* stack) {
    if (stack->count > 0) {
        Xvr_Bucket* stringBucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        printf("Stack Dump\n==========\ntype\tvalue\n");
        for (unsigned int i = 0; i < stack->count; i++) {
            Xvr_Value v = ((Xvr_Value*)(stack + 1))[i];

            printf("%s\t", Xvr_private_getValueTypeAsCString(v.type));

            Xvr_String* string =
                Xvr_stringifyValue(&stringBucket, Xvr_unwrapValue(v));
            char* buffer = Xvr_getStringRawBuffer(string);
            printf("%s", buffer);
            free(buffer);
            Xvr_freeString(string);

            printf("\n");
        }
        Xvr_freeBucket(&stringBucket);
    }
}

static void debugScopePrint(Xvr_Scope* scope, int depth) {
    if (scope->table->count > 0) {
        Xvr_Bucket* stringBucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        printf("Scope %d Dump\n==========\ntype\tname\tvalue\n", depth);
        for (unsigned int i = 0; i < scope->table->capacity; i++) {
            if ((XVR_VALUE_IS_STRING(scope->table->data[i].key) &&
                 XVR_VALUE_AS_STRING(scope->table->data[i].key)->info.type ==
                     XVR_STRING_NAME) != true) {
                continue;
            }

            Xvr_Value k = scope->table->data[i].key;
            Xvr_Value v = scope->table->data[i].value;

            printf("%s\t%s\t", Xvr_private_getValueTypeAsCString(v.type),
                   XVR_VALUE_AS_STRING(k)->name.data);

            Xvr_String* string =
                Xvr_stringifyValue(&stringBucket, Xvr_unwrapValue(v));
            char* buffer = Xvr_getStringRawBuffer(string);
            printf("%s", buffer);
            free(buffer);
            Xvr_freeString(string);
            printf("\n");
        }

        Xvr_freeBucket(&stringBucket);
    }

    if (scope->next != NULL) {
        debugScopePrint(scope->next, depth + 1);
    }
}

int main(int argc, const char* argv[]) {
    Xvr_setPrintCallback(printCallback);
    Xvr_setErrorCallback(errorAndExitCallback);
    Xvr_setAssertFailureCallback(assertFailureAndExitCallback);

    CmdLine cmd = parseCmdLine(argc, argv);

    if (cmd.silentPrint) {
        Xvr_setPrintCallback(noOpCallback);
    }

    if (cmd.silentAssert) {
        Xvr_setAssertFailureCallback(silentExitCallback);
    }

    if (cmd.error) {
        usageCmdLine(argc, argv);
    } else if (cmd.help) {
        helpCmdLine(argc, argv);
    } else if (cmd.version) {
        versionCmdLine(argc, argv);
    } else if (cmd.infile != NULL) {
        int size;
        unsigned char* source = readFile(cmd.infile, &size);

        if (source == NULL) {
            if (size == 0) {
                fprintf(stderr,
                        XVR_CC_ERROR
                        "ERROR: Could not parse an empty file '%s', "
                        "exiting\n" XVR_CC_RESET,
                        cmd.infile);
                return -1;
            }

            else if (size == -1) {
                fprintf(stderr,
                        XVR_CC_ERROR
                        "ERROR: File not found '%s', exiting\n" XVR_CC_RESET,
                        cmd.infile);
                return -1;
            }

            else {
                fprintf(stderr,
                        XVR_CC_ERROR
                        "ERROR: Unknown error while reading file '%s', "
                        "exiting\n" XVR_CC_RESET,
                        cmd.infile);
                return -1;
            }
        }

        free(cmd.infile);

        cmd.infile = NULL;
        cmd.infileLength = 0;

        Xvr_Lexer lexer;
        Xvr_bindLexer(&lexer, (char*)source);

        Xvr_Parser parser;
        Xvr_bindParser(&parser, &lexer);

        Xvr_configureParser(&parser, cmd.removeAssert);

        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        Xvr_Ast* ast = Xvr_scanParser(&bucket, &parser);
        void* buffer = Xvr_compileModuleBuilder(ast);
        Xvr_freeBucket(&bucket);
        free(source);

        Xvr_VM vm;
        Xvr_initVM(&vm);

        Xvr_Module module = Xvr_parseModule(buffer);
        Xvr_bindVM(&vm, &module);

        Xvr_runVM(&vm);

        if (cmd.verboseDebugPrint) {
            debugStackPrint(vm.stack);
            debugScopePrint(vm.scope, 0);
        }

        Xvr_freeVM(&vm);
        free(buffer);
    } else {
        repl(argv[0]);
    }

    return 0;
}
