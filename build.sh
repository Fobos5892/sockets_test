#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

TARGET=""
BUILD_TYPE="Debug"
CLEAN=0
RUN_TESTS=0
JOBS="$(nproc 2>/dev/null || echo 2)"

usage() {
    cat <<'EOF'
Usage: ./build.sh <target> [options]
       ./build.sh [options] <target>

Targets:
  server          Build server
  client          Build client
  tests           Build and run modbus_tests
  modbus_tests    Build tests only (no ctest)
  all             Build server, client and tests

Options:
  -t, --type TYPE     Build type: Debug (default) or Release
  -j, --jobs N        Parallel jobs (default: nproc)
  -c, --clean         Remove build/ and out/<TYPE> before configure
  -h, --help          Show this help

Examples:
  ./build.sh server
  ./build.sh client -t Release
  ./build.sh -t Release -c all
  ./build.sh tests --clean
EOF
}

die() {
    echo "Error: $*" >&2
    exit 1
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            server|client|tests|modbus_tests|all)
                [[ -z "$TARGET" ]] || die "target already set to '$TARGET'"
                TARGET="$1"
                shift
                ;;
            -t|--type)
                [[ $# -ge 2 ]] || die "missing value for $1"
                BUILD_TYPE="$2"
                shift 2
                ;;
            -j|--jobs)
                [[ $# -ge 2 ]] || die "missing value for $1"
                JOBS="$2"
                shift 2
                ;;
            -c|--clean)
                CLEAN=1
                shift
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                die "unknown argument: $1 (use --help)"
                ;;
        esac
    done
}

parse_args "$@"

[[ -n "$TARGET" ]] || die "target is required (server|client|tests|modbus_tests|all). See --help"

case "$BUILD_TYPE" in
    Debug|Release) ;;
    *) die "unsupported build type '$BUILD_TYPE' (use Debug or Release)" ;;
esac

command -v cmake >/dev/null 2>&1 || die "cmake not found"
command -v g++ >/dev/null 2>&1 || die "g++ not found (install: sudo apt install g++)"

BUILD_DIR="$ROOT_DIR/build"
OUT_DIR="$ROOT_DIR/out/$BUILD_TYPE"

if [[ "$CLEAN" -eq 1 ]]; then
    echo "Cleaning $BUILD_DIR and $OUT_DIR"
    rm -rf "$BUILD_DIR" "$OUT_DIR"
fi

echo "Configure: type=$BUILD_TYPE, build_dir=$BUILD_DIR"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

build_one() {
    local name="$1"
    echo "Build: $name -> $OUT_DIR/"
    cmake --build "$BUILD_DIR" --target "$name" -j "$JOBS"
}

case "$TARGET" in
    server)
        build_one server
        ;;
    client)
        build_one client
        ;;
    modbus_tests)
        build_one modbus_tests
        ;;
    tests)
        build_one modbus_tests
        RUN_TESTS=1
        ;;
    all)
        build_one server
        build_one client
        build_one modbus_tests
        RUN_TESTS=1
        ;;
esac

if [[ "$RUN_TESTS" -eq 1 ]]; then
    echo "Run tests"
    ctest --test-dir "$BUILD_DIR" --output-on-failure -j "$JOBS"
fi

echo "Done. Artifacts: $OUT_DIR/"
if [[ -d "$OUT_DIR" ]]; then
    ls -la "$OUT_DIR"
fi
