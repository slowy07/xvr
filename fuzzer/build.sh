#!/bin/bash
set -e

FUZZER_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$FUZZER_DIR"

echo "========================================="
echo "XVR Compiler Fuzzer Build System"
echo "========================================="
echo ""

usage() {
    echo "Usage: $0 <afl|libfuzzer> [clean]"
    echo ""
    echo "Options:"
    echo "  afl        - Build with AFL instrumentation"
    echo "  libfuzzer  - Build with libFuzzer (requires clang)"
    echo "  clean      - Clean build artifacts"
    echo ""
    exit 1
}

MODE="${1:-}"
ACTION="${2:-}"

if [ -z "$MODE" ]; then
    usage
fi

case "$MODE" in
    afl)
        echo "Building with AFL instrumentation..."
        
        if ! command -v afl-clang &> /dev/null; then
            echo "Error: AFL/AFL++ not found."
            echo "Please install AFL++ from: https://github.com/AFLplusplus/AFLplusplus"
            echo ""
            echo "Quick install:"
            echo "  cd ~/Downloads"
            echo "  wget https://github.com/AFLplusplus/AFLplusplus/archive/stable.tar.gz"
            echo "  tar -xf stable.tar.gz"
            echo "  cd AFLplusplus-stable"
            echo "  make distrib"
            echo "  sudo make install"
            exit 1
        fi
        
        cd build
        make -f Makefile.afl clean || true
        make -f Makefile.afl all
        echo ""
        echo "AFL build complete!"
        echo "Run: cd build && afl-fuzz -i corpus -o output -M main -- ./out/xvr @@"
        ;;
        
    libfuzzer)
        echo "Building with libFuzzer..."
        
        if ! command -v clang &> /dev/null; then
            echo "Error: clang not found"
            exit 1
        fi
        
        if [ ! -f ../out/xvr ]; then
            echo "Building XVR library..."
            make -C ../src CC=clang CFLAGS_BASE="-std=c18 -g -O1 -fno-omit-frame-pointer" || exit 1
            
            echo "Building XVR compiler..."
            make -C ../compiler CC=clang CFLAGS_BASE="-std=c18 -g -O1 -fno-omit-frame-pointer" || exit 1
        else
            echo "Using existing XVR build..."
        fi
        
        echo "Building libFuzzer harness..."
        clang -g -O1 -fno-omit-frame-pointer -fsanitize=fuzzer,address \
            -o harness/fuzz_libfuzzer harness/fuzz_libfuzzer.c
        
        echo ""
        echo "libFuzzer build complete!"
        echo "Run: ./harness/fuzz_libfuzzer corpus/"
        ;;
        
    clean)
        echo "Cleaning build artifacts..."
        make -C ../src clean 2>/dev/null || true
        make -C ../compiler clean 2>/dev/null || true
        rm -f fuzz_libfuzzer
        rm -rf build/output build/corpus_seed
        echo "Clean complete!"
        ;;
        
    *)
        usage
        ;;
esac