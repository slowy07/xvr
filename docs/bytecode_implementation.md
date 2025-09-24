# Bytecode formats

Four component in header

```
XVR_VERSION_MAJOR
XVR_VERSION_MINOR
XVR_VERSION_PATCH
XVR_VERSION_BUILD
```

for the routine structure, which is potentially recursive

```
.header:
    total size  # <- size of this routine, include all data and subroutine
    N .param count   # <- number of paramter field expected
    N .data count   # <- number of data fields expected
    N .routine count   # <-number of routine present
    .params start
    .datatable start
    .data start
    .routine start
    
.param:
    # list symbol to be using as keys in the environment

.code:
    # instructed read and `executing` by interpreter
    READ 0
    LOAD 0
    ASSERT

.datatable:
    # symbols -> pointer jumptable for quick looking up values in .data and .routines
    0 -> {string, 0x00}
    1 -> {fn, 0xFF}

.data:
    # data that can't really be embedded into .code
    <STRING>, "acumalaka:

.routines:
    # like inner routines, each of which conform to this spec
```

the content of the build string may be anything such as
- the compilation date and time of the interpreter
- marker identifying curretn fork or branch
