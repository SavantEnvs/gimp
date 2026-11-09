#!/usr/bin/env bash
#
# mayhem/test.sh — RUN this repo's OWN functional test suite (already built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

# Known-answer checks: mayhem/_build/kat (built by build.sh against the plain, unsanitized libgimp*
# libraries) parses fixed inputs with the same code the harnesses fuzz and prints what it decoded. bash
# compares that text with the expected value, so a neutered binary (empty output) fails every check.
KAT="$SRC/mayhem/_build/kat"
[ -x "$KAT" ] || { echo "missing $KAT: mayhem/build.sh did not build it" >&2; emit_ctrf "gimp-kat" 0 1 0; exit 1; }

passed=0; failed=0
check() {  # check <name> <expected> <command...>
  local name="$1" want="$2" got; shift 2
  got="$("$@" 2>/dev/null)"
  if [ "$got" = "$want" ]; then passed=$((passed + 1)); echo "ok   $name"
  else failed=$((failed + 1)); echo "FAIL $name"; echo "  want: $want"; echo "  got:  $got"; fi
}

M="$SRC/mayhem"

check sgi-verbatim "sgi 4x3x1 bpp=1
z0 y0: 10 20 30 40
z0 y1: 50 60 70 80
z0 y2: 90 100 110 120" "$KAT" sgi "$M/oracle/fixtures/gray_verbatim.sgi"

check sgi-rle "sgi 6x2x3 bpp=1
z0 y0: 255 0 0 0 0 0
z0 y1: 255 255 255 255 255 255
z1 y0: 0 0 0 0 0 0
z1 y1: 1 2 3 4 5 6
z2 y0: 7 7 7 7 7 7
z2 y1: 9 9 9 9 9 9" "$KAT" sgi "$M/oracle/fixtures/rgb_rle.sgi"

check color-hex "color 255 128 0 255" "$KAT" color "#ff8000"
check color-rgb-func "color 10 20 30 255" "$KAT" color "rgb(10, 20, 30)"
check config-enum-bool "config mode=2 show-hsv=0 show-rgb-u8=1" "$KAT" config "(mode softproof) (show-hsv no) (show-rgb-u8 yes)"
check icc-srgb "icc rgb=1 gray=0 cmyk=0 linear=0 desc=sRGB built-in" "$KAT" icc "$M/oracle/fixtures/srgb.icc"
check icc-gray "icc rgb=0 gray=1 cmyk=0 linear=0 desc=gray built-in" "$KAT" icc "$M/oracle/fixtures/gray.icc"
check metadata-xml "meta make=Acme model=Cam 1" "$KAT" meta '<metadata><tag name="Exif.Image.Make">Acme</tag><tag name="Exif.Image.Model">Cam 1</tag></metadata>'

emit_ctrf "gimp-kat" "$passed" "$failed" 0
