#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

TARGET=""
BUILD_TYPE="Debug"
CLEAN=0
RUN_TESTS=0
RUN_BENCH=0
# 0=auto (on for test targets), 1=force on, 2=force off
SANITIZE_MODE=0
JOBS="$(nproc 2>/dev/null || echo 2)"

usage() {
    cat <<'EOF'
Usage: ./build.sh <target> [options]
       ./build.sh [options] <target>

Targets:
  server          Build server
  client          Build client
  tests           Build and run modbus_tests (ASan/UBSan/LSan by default)
  modbus_tests    Build tests only (sanitizers by default)
  modbus_bench    Build benchmarks only (no sanitizers)
  bench           Build and run modbus_bench (prefer -t Release)
  check           Unit tests under sanitizers, then benchmarks without them
  asan            Alias for tests (kept for compatibility)
  all             Build server/client/bench + run sanitized tests

Options:
  -t, --type TYPE     Build type: Debug (default) or Release (-O3)
  -j, --jobs N        Parallel jobs (default: nproc)
  -c, --clean         Remove relevant build/out dirs before configure
  -s, --sanitize      Force sanitizers on (also for server/client if requested)
  --no-sanitize       Disable sanitizers for tests
  -h, --help          Show this help

Examples:
  ./build.sh tests
  ./build.sh check -t Release
  ./build.sh tests --no-sanitize
  ./build.sh bench -t Release
EOF
}

die() {
    echo "Error: $*" >&2
    exit 1
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            server|client|tests|modbus_tests|modbus_bench|bench|check|asan|all)
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
            -s|--sanitize)
                SANITIZE_MODE=1
                shift
                ;;
            --no-sanitize)
                SANITIZE_MODE=2
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

[[ -n "$TARGET" ]] || die "target is required. See --help"

if [[ "$TARGET" == "asan" ]]; then
    TARGET="tests"
    if [[ "$SANITIZE_MODE" -eq 0 ]]; then
        SANITIZE_MODE=1
    fi
fi

case "$BUILD_TYPE" in
    Debug|Release) ;;
    *) die "unsupported build type '$BUILD_TYPE' (use Debug or Release)" ;;
esac

command -v cmake >/dev/null 2>&1 || die "cmake not found"
command -v g++ >/dev/null 2>&1 || die "g++ not found (install: sudo apt install g++)"

want_sanitize_for_tests() {
    case "$SANITIZE_MODE" in
        1) return 0 ;;
        2) return 1 ;;
        *) return 0 ;; # default: sanitizers ON for tests
    esac
}

want_sanitize_for_apps() {
    [[ "$SANITIZE_MODE" -eq 1 ]]
}

NORMAL_BUILD_DIR="$ROOT_DIR/build"
NORMAL_OUT_DIR="$ROOT_DIR/out/$BUILD_TYPE"
ASAN_BUILD_DIR="$ROOT_DIR/build-asan"
ASAN_OUT_DIR="$ROOT_DIR/out/${BUILD_TYPE}-asan"
CONFIG_SRC_DIR="$ROOT_DIR/config"

# Active dirs used by helpers (set by use_normal_tree / use_asan_tree)
BUILD_DIR=""
OUT_DIR=""
ENABLE_SANITIZERS=0

use_normal_tree() {
    BUILD_DIR="$NORMAL_BUILD_DIR"
    OUT_DIR="$NORMAL_OUT_DIR"
    ENABLE_SANITIZERS=0
}

use_asan_tree() {
    BUILD_DIR="$ASAN_BUILD_DIR"
    OUT_DIR="$ASAN_OUT_DIR"
    ENABLE_SANITIZERS=1
}

configure_tree() {
    local cmake_args=(-DCMAKE_BUILD_TYPE="$BUILD_TYPE")
    if [[ "$ENABLE_SANITIZERS" -eq 1 ]]; then
        cmake_args+=(-DENABLE_SANITIZERS=ON)
        echo "Configure: type=$BUILD_TYPE, sanitizers=ON, build_dir=$BUILD_DIR"
    else
        cmake_args+=(-DENABLE_SANITIZERS=OFF)
        echo "Configure: type=$BUILD_TYPE, sanitizers=OFF, build_dir=$BUILD_DIR"
    fi
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" "${cmake_args[@]}"
}

build_one() {
    local name="$1"
    echo "Build: $name -> $OUT_DIR/"
    cmake --build "$BUILD_DIR" --target "$name" -j "$JOBS"
}

sync_config_templates() {
    local dest="$1/config"
    mkdir -p "$dest"
    cp "$CONFIG_SRC_DIR/server.conf" "$dest/server.conf"
    cp "$CONFIG_SRC_DIR/client.conf" "$dest/client.conf"
    cp "$CONFIG_SRC_DIR/client2.conf" "$dest/client2.conf"
    echo "Config templates: $dest/"
}

run_tests() {
    local log junit_file status=0
    log="$(mktemp)"
    junit_file="$(mktemp "${TMPDIR:-/tmp}/ctest-results.XXXXXX.xml")"

    echo "Run tests"
    local ctest_jobs="$JOBS"
    if [[ "$ENABLE_SANITIZERS" -eq 1 ]]; then
        ctest_jobs=1
    fi

    set +e
    ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1:halt_on_error=1}" \
    UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1:print_stacktrace=1}" \
    ctest --test-dir "$BUILD_DIR" --output-on-failure -j "$ctest_jobs" \
        --output-junit "$junit_file" >"$log" 2>&1
    status=$?
    set -e

    if [[ $status -ne 0 ]]; then
        cat "$log"
    fi

    local passed=0 failed=0 unavailable=0
    if [[ -f "$junit_file" ]] && grep -q '<testcase ' "$junit_file"; then
        local total errors
        total=$(grep -c '<testcase ' "$junit_file" || true)
        failed=$(grep -c '<failure' "$junit_file" || true)
        errors=$(grep -c '<error' "$junit_file" || true)
        failed=$((failed + errors))
        unavailable=$(grep -c 'status="notrun"' "$junit_file" || true)
        passed=$((total - failed - unavailable))
    else
        passed=$(grep -Ec '[[:space:]]+Passed[[:space:]]+' "$log" || true)
        failed=$(grep -Ec '\*\*\*Failed|\*\*\*Timeout' "$log" || true)
        unavailable=$(grep -Ec '\*\*\*Not Run|\*\*\*Disabled|\*\*\*Exception' "$log" || true)
    fi

    if [[ "$ENABLE_SANITIZERS" -eq 1 ]]; then
        if grep -Eq 'ERROR: (AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer)' "$log"; then
            echo "SANITIZER: FAIL (see log above)"
            status=1
        else
            echo "SANITIZER: OK (ASan/UBSan/LSan)"
        fi
    else
        echo "SANITIZER: skipped (--no-sanitize)"
    fi

    echo "PASSED: $passed, FAILED: $failed, UNAVAILABLE: $unavailable"
    rm -f "$log" "$junit_file"
    return "$status"
}

run_bench() {
    local bench_bin="$OUT_DIR/modbus_bench"
    local log status=0
    log="$(mktemp)"

    if [[ "$BUILD_TYPE" != "Release" ]]; then
        echo "Warning: benchmarks ideally run with -t Release (-O3); current type=$BUILD_TYPE"
    fi

    echo "Run benchmarks"
    set +e
    "$bench_bin" --benchmark_min_time=0.05s >"$log" 2>&1
    status=$?
    set -e

    if [[ $status -ne 0 ]]; then
        cat "$log"
        rm -f "$log"
        return "$status"
    fi

    local count
    count=$(grep -cE '^BM_' "$log" || true)
    echo "BENCHMARKS: $count"
    grep -E '^BM_|^[-]+$|^Benchmark' "$log" || cat "$log"
    rm -f "$log"
    return 0
}

maybe_clean() {
    if [[ "$CLEAN" -ne 1 ]]; then
        return 0
    fi
    echo "Cleaning $BUILD_DIR and $OUT_DIR"
    rm -rf "$BUILD_DIR" "$OUT_DIR"
}

build_tests_tree() {
    if want_sanitize_for_tests; then
        use_asan_tree
    else
        use_normal_tree
    fi
    maybe_clean
    configure_tree
    build_one modbus_tests
}

build_apps_tree() {
    if want_sanitize_for_apps; then
        use_asan_tree
    else
        use_normal_tree
    fi
    maybe_clean
    configure_tree
}

case "$TARGET" in
    server)
        build_apps_tree
        build_one server
        sync_config_templates "$OUT_DIR"
        ;;
    client)
        build_apps_tree
        build_one client
        sync_config_templates "$OUT_DIR"
        ;;
    modbus_tests)
        build_tests_tree
        sync_config_templates "$OUT_DIR"
        ;;
    tests)
        build_tests_tree
        RUN_TESTS=1
        run_tests
        sync_config_templates "$OUT_DIR"
        ;;
    modbus_bench)
        use_normal_tree
        maybe_clean
        configure_tree
        build_one modbus_bench
        ;;
    bench)
        use_normal_tree
        maybe_clean
        configure_tree
        build_one modbus_bench
        run_bench
        ;;
    check)
        build_tests_tree
        RUN_TESTS=1
        run_tests
        sync_config_templates "$OUT_DIR"

        use_normal_tree
        # Don't wipe asan tree on -c for the second half if already cleaned above;
        # only clean normal tree when -c was requested.
        if [[ "$CLEAN" -eq 1 ]]; then
            echo "Cleaning $BUILD_DIR and $OUT_DIR"
            rm -rf "$BUILD_DIR" "$OUT_DIR"
        fi
        configure_tree
        build_one modbus_bench
        run_bench
        ;;
    all)
        build_apps_tree
        build_one server
        build_one client
        build_one modbus_bench
        sync_config_templates "$OUT_DIR"

        build_tests_tree
        RUN_TESTS=1
        run_tests
        sync_config_templates "$OUT_DIR"
        ;;
esac

echo "Done."
if [[ -n "$OUT_DIR" && -d "$OUT_DIR" ]]; then
    echo "Artifacts: $OUT_DIR/"
    ls -la "$OUT_DIR"
fi
