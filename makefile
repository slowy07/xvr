export CFLAGS+=-std=c18 -pedantic -Werror
export XVR_OUTDIR = out

ifeq ($(shell uname),Linux)
    BINDIR = /usr/bin
    LIBDIR = /usr/lib64
else ifeq ($(shell uname),Darwin)
    BINDIR = /usr/local/bin
    LIBDIR = /usr/local/lib
else ifeq ($(findstring CYGWIN, $(shell uname)),CYGWIN)
    BINDIR = /usr/bin
    LIBDIR = /usr/lib
else
    BINDIR = /usr/bin
    LIBDIR = /usr/lib
endif

all: $(XVR_OUTDIR) inter

inter: $(XVR_OUTDIR) library static inter
	$(MAKE) -C inter

inter-static: $(XVR_OUTDIR) static
	$(MAKE) -C inter

inter-release: clean $(XVR_OUTDIR) library-release
	$(MAKE) -C inter release

inter-static-release: clean $(XVR_OUTDIR) static-release
	$(MAKE) -C inter release

library: $(XVR_OUTDIR)
	$(MAKE) -j$(nproc) -C src library

static: $(XVR_OUTDIR)
	$(MAKE) -j$(nproc) -C src static

library-release: $(XVR_OUTDIR)
	$(MAKE) -j$(nproc) -C src library-release

static-release: $(XVR_OUTDIR)
	$(MAKE) -j$(nproc) -C src static-release

test: clean $(XVR_OUTDIR)
	$(MAKE) -C test
	@for f in $(XVR_OUTDIR)/*.exe; do \
		if [ -x "$$f" ] && [ -f "$$f" ]; then \
			"$$f"; \
		fi; \
	done

test-sanitized: export CFLAGS+=-fsanitize=address,undefined
test-sanitized: export LIBS+=-static-libasan
test-sanitized: export DISABLE_VALGRIND=true
test-sanitized: clean $(XVR_OUTDIR)
	$(MAKE) -C test

$(XVR_OUTDIR):
	mkdir $(XVR_OUTDIR)

install: inter library static
ifeq ($(shell uname),Linux)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(XVR_OUTDIR)/xvr $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 $(XVR_OUTDIR)/libxvr.so $(DESTDIR)$(LIBDIR)/
	install -m 644 $(XVR_OUTDIR)/libxvr.a $(DESTDIR)$(LIBDIR)/
	ldconfig
else ifeq ($(shell uname),Darwin)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(XVR_OUTDIR)/xvr $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 $(XVR_OUTDIR)/libxvr.dylib $(DESTDIR)$(LIBDIR)/
	install -m 644 $(XVR_OUTDIR)/libxvr.a $(DESTDIR)$(LIBDIR)/
else ifeq ($(findstring CYGWIN, $(shell uname)),CYGWIN)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(XVR_OUTDIR)/xvr.exe $(DESTDIR)$(BINDIR)/
	install -m 755 $(XVR_OUTDIR)/xvr.dll $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)$(LIBDIR)
	install -m 644 $(XVR_OUTDIR)/libxvr.dll.a $(DESTDIR)$(LIBDIR)/
else
	@echo "Install target not supported on this platform."
endif

uninstall:
ifeq ($(shell uname),Linux)
	rm -f $(DESTDIR)$(BINDIR)/xvr
	rm -f $(DESTDIR)$(LIBDIR)/libxvr.so
	rm -f $(DESTDIR)$(LIBDIR)/libxvr.a
	ldconfig
else ifeq ($(shell uname),Darwin)
	rm -f $(DESTDIR)$(BINDIR)/xvr
	rm -f $(DESTDIR)$(LIBDIR)/libxvr.dylib
	rm -f $(DESTDIR)$(LIBDIR)/libxvr.a
else ifeq ($(findstring CYGWIN, $(shell uname)),CYGWIN)
	rm -f $(DESTDIR)$(BINDIR)/xvr.exe
	rm -f $(DESTDIR)$(BINDIR)/xvr.dll
	rm -f $(DESTDIR)$(LIBDIR)/libxvr.dll.a
else
	@echo "Uninstall target not supported on this platform."
endif

clean:
ifeq ($(findstring CYGWIN, $(shell uname)),CYGWIN)
	find . -type f -name '*.o' -exec rm -f -r -v {} \;
	find . -type f -name '*.a' -exec rm -f -r -v {} \;
	find . -type f -name '*.exe' -exec rm -f -r -v {} \;
	find . -type f -name '*.dll' -exec rm -f -r -v {} \;
	find . -type f -name '*.lib' -exec rm -f -r -v {} \;
	find . -type f -name '*.so' -exec rm -f -r -v {} \;
	find . -empty -type d -delete
else ifeq ($(shell uname),Linux)
	find . -type f -name '*.o' -exec rm -f -r -v {} \;
	find . -type f -name '*.a' -exec rm -f -r -v {} \;
	find . -type f -name '*.exe' -exec rm -f -r -v {} \;
	find . -type f -name '*.dll' -exec rm -f -r -v {} \;
	find . -type f -name '*.lib' -exec rm -f -r -v {} \;
	find . -type f -name '*.so' -exec rm -f -r -v {} \;
	rm -rf out
	find . -empty -type d -delete
else ifeq ($(OS),Windows_NT)
	$(RM) *.o *.a *.exe 
else ifeq ($(shell uname),Darwin)
	find . -type f -name '*.o' -exec rm -f -r -v {} \;
	find . -type f -name '*.a' -exec rm -f -r -v {} \;
	find . -type f -name '*.exe' -exec rm -f -r -v {} \;
	find . -type f -name '*.dll' -exec rm -f -r -v {} \;
	find . -type f -name '*.lib' -exec rm -f -r -v {} \;
	find . -type f -name '*.dylib' -exec rm -f -r -v {} \;
	find . -type f -name '*.so' -exec rm -f -r -v {} \;
	rm -rf out
	find . -empty -type d -delete
else
	@echo "deletion failed - are you using temple os?"
endif

rebuild: clean all

.PHONY: all inter inter-static inter-release inter-static-release library static library-release static-release test test-sanitized install uninstall clean rebuild

