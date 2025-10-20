#include <bits/pthreadtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APPEND(dest, src) \
    strncpy((dest) + (strlen(dest)), (src), strlen((src)) + 1);

#if defined(_WIN32) || defined(_WIN64)
#    define FLIPSLASH(str) \
        for (int i = 0; str[i]; i++) str[i] = str[i] == '/' ? '\\' : str[i];
#else
#    define FLIPSLASH(str) \
        for (int i = 0; str[i]; i++) str[i] = str[i] == '\\' ? '/' : str[i];
#endif

unsigned char* readFile(char* path, int* size) {
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

    if (fread(buffer, sizeof(unsigned char), *size, file) < (unsigned int)(*size)) {
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
        p = strrchr(src, '/');
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

int main(void) {
    printf("platform check: ");

#if defined(__linux__)
    printf(" Linux");
#elif defined(_WIN64)
    printf("Win64");
#elif defined(_WIN32)
    printf("Win32");
#elif defined(__APPLE__)
    printf("macOS");
#else
    printf("templeOS");
#endif /* if defined (__linux__) */

    printf("\n");

    {
        char src[256] = "../folder/file.txt";
        char dest[256];
        getFilePath(dest, src);
        printf("Name: %s\n", dest);
    }

    {
        char src[256] = "../folder/file.txt";
        char dest[256];
        getFileName(dest, src);
        printf("Name: %s\n", dest);
    }

    return 0;
}
