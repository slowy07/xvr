export CFLAGS+=-std=c18 -pedantic -Werror
export XVR_OUTDIR = out

all: $(XVR_OUTDIR) inter

inter: $(XVR_OUTDIR) library static inter ## build shared lib, static lib and xvr interpreter (default)
	$(MAKE) -C inter

inter-static: $(XVR_OUTDIR) static ## build static lib and xvr interpreter
	$(MAKE) -C inter

inter-release: clean $(XVR_OUTDIR) library-release ## build release shared lib and xvr interpreter
	$(MAKE) -C inter release

inter-static-release: clean $(XVR_OUTDIR) static-release ## build release static lib and xvr interpreter
	$(MAKE) -C inter release

library: $(XVR_OUTDIR) ## build shared library
	$(MAKE) -j$(nproc) -C src library

static: $(XVR_OUTDIR) ## build static lib
	$(MAKE) -j$(nproc) -C src static

library-release: $(XVR_OUTDIR) ## build shared lib release version
	$(MAKE) -j$(nproc) -C src library-release

static-release: $(XVR_OUTDIR) ## build static lib release version
	$(MAKE) -j$(nproc) -C src static-release

test: clean $(XVR_OUTDIR) ## clean and test
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

.PHONY: clean

clean: ## clean
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

rebuild: clean all ## clean and build

help: ## show this message
	@cat $(MAKEFILE_LIST) | grep -E '^[a-zA-Z_-]+:.*?## .*$$' | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
