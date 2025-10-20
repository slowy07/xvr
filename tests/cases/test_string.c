#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include "xvr_string.h"
#include "xvr_value.h"

int test_sizeof_string_64bit(void) {
    {
        if (sizeof(Xvr_String) != 32) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "Error: `Xvr_String` unexpected size in memory: "
                    "expected 32, found %d \n" XVR_CC_RESET,
                    (int)sizeof(Xvr_String));
            return -1;
        }
    }
    return 0;
}

int test_sizeof_string_32bit(void) {
    {
        if (sizeof(Xvr_String) != 24) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "Error: `Xvr_String` unexpected size in memory: "
                    "expected 24, found %d \n" XVR_CC_RESET,
                    (int)sizeof(Xvr_String));
            return -1;
        }
    }
    return 0;
}

int test_string_equal(void) {
    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(1024);
        Xvr_String* helloWorld = Xvr_createString(&bucket, "hello world");
        Xvr_String* helloWorldC = Xvr_createString(&bucket, "hello world");

        int result = 0;

        if ((result = Xvr_compareStrings(helloWorld, helloWorldC)) != 0) {
            char* leftBuffer = Xvr_getStringRawBuffer(helloWorld);
            char* rightBuffer = Xvr_getStringRawBuffer(helloWorldC);
            fprintf(stderr,
                    XVR_CC_ERROR
                    "Error: string equal `%s` == `%s` incorrect, found "
                    "%s\n" XVR_CC_RESET,
                    leftBuffer, rightBuffer,
                    result < 0    ? "<"
                    : result == 0 ? "=="
                                  : ">");
            free(leftBuffer);
            free(rightBuffer);
            Xvr_freeBucket(&bucket);
            return -1;
        }
    }

    return 0;
}

int test_string_allocation(void) {
    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(1024);
        const char* cstring = "hello world";
        Xvr_String* str = Xvr_createString(&bucket, cstring);

        if (str->info.type != XVR_STRING_LEAF || str->info.length != 11 ||
            str->info.refCount != 1 ||
            strcmp(str->leaf.data, "hello world") != 0) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: failed to allocate `Xvr_String` "
                    "with private bucket\n" XVR_CC_RESET);
            Xvr_freeBucket(&bucket);
            return -1;
        }

        Xvr_freeString(str);

        if (bucket->capacity != 1024 ||
            bucket->count != sizeof(Xvr_String) + 12 || bucket->next != NULL) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: unexpected bucket state after "
                    "string was free\n" XVR_CC_RESET);
            Xvr_freeBucket(&bucket);
            return -1;
        }

        if (Xvr_getStringRefCount(str) != 0) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: unexpected string state after it was "
                    "free\n" XVR_CC_RESET);
            Xvr_freeBucket(&bucket);
            return -1;
        }

        Xvr_freeBucket(&bucket);
    }

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(1024);

        const char* cstring = "hello world";
        Xvr_String* str = Xvr_createNameStringLength(
            &bucket, cstring, strlen(cstring), XVR_VALUE_UNKNOWN, false);

        Xvr_String* shallow = Xvr_copyString(str);
        Xvr_String* deep = Xvr_deepCopyString(&bucket, str);

        if (str != shallow || str == deep || shallow->info.refCount != 2 ||
            deep->info.refCount != 1 ||
            strcmp(shallow->name.data, deep->name.data) != 0) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: failed copy name string \n" XVR_CC_RESET);
            Xvr_freeBucket(&bucket);
            return -1;
        }
    }

    return 0;
}

int test_string_diffs(void) {
    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(1024);
        Xvr_String* pangram = Xvr_concatStrings(
            &bucket, Xvr_createString(&bucket, "The quick brown "),
            Xvr_concatStrings(&bucket, Xvr_createString(&bucket, "fox jumps o"),
                              Xvr_createString(&bucket, "ver the lazy dog.")));

        Xvr_String* neckbeard = Xvr_concatStrings(
            &bucket,
            Xvr_createString(&bucket, "The quick brown fox jumps over"),
            Xvr_concatStrings(&bucket, Xvr_createString(&bucket, "the lazy"),
                              Xvr_createString(&bucket, "dog.")));

        int result = 0;

        if (((result = Xvr_compareStrings(pangram, neckbeard)) < 0) == false) {
            char* leftBuffer = Xvr_getStringRawBuffer(pangram);
            char* rightBuffer = Xvr_getStringRawBuffer(neckbeard);
            fprintf(stderr,
                    XVR_CC_ERROR
                    "Error: string diff `%s` == `%s` incorrect, found "
                    "%s\n" XVR_CC_RESET,
                    leftBuffer, rightBuffer,
                    result < 0    ? "<"
                    : result == 0 ? "=="
                                  : ">");
            free(leftBuffer);
            free(rightBuffer);
            Xvr_freeBucket(&bucket);
            return -1;
        }

        Xvr_freeBucket(&bucket);
    }

    return 0;
}

int test_string_fragmenting(void) {
    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(1024);

        const char* cstring =
            "Kami bangsa Indonesia dengan ini menyatakan kemerdekaan "
            "Indonesia. "
            "Hal-hal mengenai pemindahan kekuasaan dan lain-lain, "
            "diselenggarakan "
            "dengan cara saksama dan dalam tempo yang sesingkat-singkatnya. "
            "Djakarta, hari 17 bulan 8 tahun '05. Atas nama bangsa Indonesia, "
            "Soekarno/Hatta";

        Xvr_String* str = Xvr_createString(&bucket, cstring);

        if (str->info.type != XVR_STRING_LEAF ||
            str->info.length != strlen(cstring) || str->info.refCount != 1) {
            fprintf(
                stderr, XVR_CC_ERROR
                "Error: Failed to fragment within Xvr_String\n" XVR_CC_RESET);
            Xvr_freeString(str);
            Xvr_freeBucket(&bucket);
            return -1;
        }

        Xvr_freeString(str);
        Xvr_freeBucket(&bucket);
    }

    return 0;
}

int main(void) {
    printf(XVR_CC_WARN "TESTING: XVR STRING\n" XVR_CC_RESET);
    int total = 0, res = 0;

    {
#if XVR_BITNESS == 64
        res = test_sizeof_string_64bit();
#else
        res = test_sizeof_string_32bit();
#endif /* if XVR_BITNESS == 64 */
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "SIZEOF 32 / 64: PASSED nice one reks\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        res = test_string_equal();
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "STRING EQUAL: PASSED nice one reks\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        res = test_string_allocation();
        if (res == 0) {
            printf(
                XVR_CC_NOTICE
                "STRING ALLOCATION: PASSED nice one aman rek\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        res = test_string_diffs();
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "STRING DIFFS: PASSED nice one loh ya rek\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        res = test_string_fragmenting();
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "STRING FRAGMENTING: PASSED dah bisa test fragment loh ya "
                   "XD. XVR "
                   "dah nda ngambek lagi XD.\n" XVR_CC_RESET);
        }
        total += res;
    }

    return total;
}
