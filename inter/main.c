#include "xvr.h"
#include "xvr_print.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char *readFile(char *path, int *size) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    *size = -1;
    return NULL;
  }

  fseek(file, 0L, SEEK_END);
  *size = ftell(file);
  rewind(file);

  unsigned char *buffer = malloc(*size + 1);
  if (buffer == NULL) {
    fclose(file);
    return NULL;
  }

  if (fread(buffer, sizeof(unsigned char), *size, file) < *size) {
    fclose(file);
    *size = -2;
    return NULL;
  }

  fclose(file);

  buffer[(*size)++] = '\0';
  return buffer;
}

int dir(char *dest, const char *src) {

#if defined(_WIN32) || defined(_WIN64)
  char *p = strrchr(src, '\\');
#else
  char *p = strrchr(src, '/');
#endif

  int len = p != NULL ? p - src + 1 : 0;
  strncpy(dest, src, len);
  dest[len] = '\0';

  return len;
}

#define APPEND(dest, src) memcpy((dest) + (strlen(dest)), (src), strlen((src)));

#if defined(_WIN32) || defined(_WIN64)
#define FLIPSLASH(str)                                                         \
  for (int i = 0; str[i]; i++)                                                 \
    str[i] = str[i] == '/' ? '\\' : str[i];
#else
#define FLIPSLASH(str)                                                         \
  for (int i = 0; str[i]; i++)                                                 \
    str[i] = str[i] == '\\' ? '/' : str[i];
#endif

typedef struct CmdLine {
  bool error;
  bool help;
  bool version;
  char *infile;
  int infileLength;
} CmdLine;

void usageCmdLine(int argc, const char *argv[]) {
  printf(
      "Usage: %s [ -h (--help) | -v (--version) | -f (--file) source.xvr ]\n\n",
      argv[0]);
}

void helpCmdLine(int argc, const char *argv[]) {
  usageCmdLine(argc, argv);

  printf("  -h, --help\t\t\tShow this help\n");
  printf("  -v, --version\t\t\tShow version\n");
  printf("  -f, --file infile\t\tParse, compile and run\n");
}

void versionCmdLine(int argc, const char *argv[]) {
  printf("The Xvr Programming Language, Version %d.%d.%d %s\n\n",
         XVR_VERSION_MAJOR, XVR_VERSION_MINOR, XVR_VERSION_PATCH,
         XVR_VERSION_BUILD);
}

CmdLine parseCmdLine(int argc, const char *argv[]) {
  CmdLine cmd = {.error = false,
                 .help = false,
                 .version = false,
                 .infile = NULL,
                 .infileLength = 0};

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
        cmd.infile = malloc(cmd.infileLength);

        if (cmd.infile == NULL) {
          fprintf(stderr,
                  XVR_CC_ERROR "ERROR: Failed to allocate space while parsing "
                               "the command line, exiting\n" XVR_CC_RESET);
          exit(-1);
        }

        dir(cmd.infile, argv[0]);
        APPEND(cmd.infile, argv[i]);
        FLIPSLASH(cmd.infile);
      }
    }

    else {
      cmd.error = true;
    }
  }

  return cmd;
}

static void errorAndExit(const char *msg) {
  fprintf(stderr, "%s", msg);
  exit(-1);
}

int main(int argc, const char *argv[]) {
  Xvr_setErrorCallback(errorAndExit);
  Xvr_setAssertFailureCallback(errorAndExit);
  CmdLine cmd = parseCmdLine(argc, argv);

  if (cmd.error) {
    usageCmdLine(argc, argv);
  } else if (cmd.help) {
    helpCmdLine(argc, argv);
  } else if (cmd.version) {
    versionCmdLine(argc, argv);
  } else if (cmd.infile != NULL) {
    int size;
    unsigned char *source = readFile(cmd.infile, &size);

    if (source == NULL) {
      if (size == 0) {
        fprintf(
            stderr,
            XVR_CC_ERROR
            "ERROR: Could not parse an empty file '%s', exiting\n" XVR_CC_RESET,
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
                XVR_CC_ERROR "ERROR: Unknown error while reading file '%s', "
                             "exiting\n" XVR_CC_RESET,
                cmd.infile);
        return -1;
      }
    }

    free(cmd.infile);

    cmd.infile = NULL;
    cmd.infileLength = 0;

    Xvr_Lexer lexer;
    Xvr_bindLexer(&lexer, (char *)source);

    Xvr_Parser parser;
    Xvr_bindParser(&parser, &lexer);

    Xvr_Bucket *bucket = Xvr_allocateBucket(sizeof(Xvr_Ast) * 32);
    Xvr_Ast *ast = Xvr_scanParser(&bucket, &parser);

    Xvr_Bytecode bc = Xvr_compileBytecode(ast);

    // run the setup
    Xvr_VM vm;
    Xvr_bindVM(&vm, bc.ptr);

    // run
    Xvr_runVM(&vm);

    // debugging result
    printf("printing the stack result\n\ntype\tvalue\n");
    for (int i = 0; i < vm.stack->count; i++) {
      Xvr_Value v = ((Xvr_Value *)(vm.stack + 1))[i];

      printf(" %d\t ", v.type);

      switch (v.type) {
      case XVR_VALUE_NULL:
        printf("null");
        break;

      case XVR_VALUE_BOOLEAN:
        printf("%s", XVR_VALUE_AS_BOOLEAN(v) ? "true" : "false");
        break;

      case XVR_VALUE_INTEGER:
        printf("%d", XVR_VALUE_AS_INTEGER(v));
        break;

      case XVR_VALUE_FLOAT:
        printf("%f", XVR_VALUE_AS_FLOAT(v));
        break;

      case XVR_VALUE_STRING:
      case XVR_VALUE_ARRAY:
      case XVR_VALUE_DICTIONARY:
      case XVR_VALUE_FUNCTION:
      case XVR_VALUE_OPAQUE:
        printf("???");
        break;
      }

      printf("\n");
    }

    Xvr_freeVM(&vm);
    Xvr_freeBucket(&bucket);
    free(source);
  } else {
    usageCmdLine(argc, argv);
  }

  return 0;
}
