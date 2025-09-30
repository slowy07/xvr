#include "xvr_print.h"
#include <stdio.h>

static void outDefault(const char *msg) { fprintf(stdout, "%s", msg); }

static void errDefault(const char *msg) { fprintf(stderr, "%s", msg); }

static Xvr_callbackType printCallback = outDefault;
static Xvr_callbackType errorCallback = errDefault;
static Xvr_callbackType assertCallback = errDefault;

void Xvr_print(const char *msg) { printCallback(msg); }

void Xvr_error(const char *msg) { assertCallback(msg); }

void Xvr_setPrintCallback(Xvr_callbackType cb) { printCallback = cb; }

void Xvr_setErrorCallback(Xvr_callbackType cb) { errorCallback = cb; }

void Xvr_setAssertFailureCallback(Xvr_callbackType cb) { assertCallback = cb; }

void Xvr_resetPrintCallback() { printCallback = outDefault; }

void Xvr_resetErrorCallback() { errorCallback = errDefault; }

void Xvr_resetAssertFailureCallback() { assertCallback = errDefault; }
