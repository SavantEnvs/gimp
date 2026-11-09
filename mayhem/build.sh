#!/usr/bin/env bash
#
# mayhem/build.sh: build GIMP's support libraries (libgimpbase/color/config/math) twice with meson,
# once sanitized + SanCov for the fuzz harnesses and once plain for the test oracle, then link
# the harnesses in mayhem/harness/ and the KAT probe in mayhem/oracle/.
#
# GIMP's meson project insists on a gimp-data checkout (an empty submodule in this image) and on
# babl/gegl versions newer than Debian ships. Neither matters for the libraries built here, so the
# build runs on a scratch copy of the tree with stub gimp-data meson files and version-bumped copies
# of the babl/gegl .pc files. The upstream tree is never modified.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

SRC="${SRC:-/mayhem}"
cd "$SRC"
W="$SRC/mayhem/_build"
S="$W/src"
FPC="$W/fpc"
LIBS=(libgimpbase/libgimpbase-3.0.so.0.300.1 libgimpcolor/libgimpcolor-3.0.so.0.300.1
      libgimpconfig/libgimpconfig-3.0.so.0.300.1 libgimpmath/libgimpmath-3.0.so.0.300.1)
# The libgimp* sonames are derived from the version in meson.build; resolve them after configure.

rm -rf "$W"
mkdir -p "$S" "$FPC"

# --- scratch source tree: everything but .git, mayhem/ and the (empty) gimp-data submodule ---
tar -C "$SRC" --exclude=./.git --exclude=./mayhem --exclude=./gimp-data -cf - . | tar -C "$S" -xf -
mkdir -p "$S/gimp-data/images"
cat > "$S/gimp-data/meson.build" <<'MESON'
icons_imgs_sources  = []
cursors_sources     = []
pickers_sources     = []
icons_core_sources  = []
MESON
: > "$S/gimp-data/images/meson.build"

# --- version-bumped .pc files so meson's minimum-version checks pass ---
sed 's/^Version:.*/Version: 0.1.118/' /usr/lib/x86_64-linux-gnu/pkgconfig/babl-0.1.pc > "$FPC/babl-0.1.pc"
sed 's/^Version:.*/Version: 0.4.66/'  /usr/lib/x86_64-linux-gnu/pkgconfig/gegl-0.4.pc > "$FPC/gegl-0.4.pc"
cat > "$FPC/mypaint-brushes-2.0.pc" <<'PC'
Name: mypaint-brushes-2.0
Description: stand-in for the version check
Version: 2.0.2
brushesdir=/usr/share/mypaint-data/2.0/brushes
PC
export PKG_CONFIG_PATH="$FPC${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"

# build_gimp_libs <builddir> <cflags> [extra ninja targets]
build_gimp_libs() {
  local bd="$1" cf="$2"; shift 2
  CFLAGS="$cf" ASAN_OPTIONS=detect_leaks=0 \
    meson setup "$bd" "$S" -Dcheck-update=no -Db_lundef=false --buildtype=plain >"$bd.setup.log" 2>&1 \
    || { tail -40 "$bd.setup.log"; exit 1; }
  ( cd "$bd" && ninja -j"$MAYHEM_JOBS" "${LIBS[@]}" "$@" ) >"$bd.ninja.log" 2>&1 \
    || { tail -40 "$bd.ninja.log"; exit 1; }
}

FUZZ_CF="$SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS -O1"
build_gimp_libs "$W/fuzz"  "$FUZZ_CF" libgimp/libgimp-3.0.so.0.300.1
build_gimp_libs "$W/plain" "-O1 $COVERAGE_FLAGS"

# --- compile + link flags shared by harnesses and the oracle ---
pcflags() { pkg-config --cflags gegl-0.4 gio-2.0 gexiv2 lcms2 babl-0.1 cairo gdk-pixbuf-2.0; }
pclibs()  { pkg-config --libs   gegl-0.4 gio-2.0 gexiv2 lcms2 babl-0.1; }
gimp_inc() { echo "-I$S -I$1 -I$S/libgimpbase"; }
gimp_lib() {
  local bd="$1"
  echo -L"$bd/libgimpconfig" -L"$bd/libgimpcolor" -L"$bd/libgimpbase" -L"$bd/libgimpmath" \
       -lgimpconfig-3.0 -lgimpcolor-3.0 -lgimpbase-3.0 -lgimpmath-3.0 \
       -Wl,-rpath,"$bd/libgimpconfig" -Wl,-rpath,"$bd/libgimpcolor" \
       -Wl,-rpath,"$bd/libgimpbase" -Wl,-rpath,"$bd/libgimpmath"
}

# LeakSanitizer is turned off at build time (SPEC §6.2 item 15).
$CC -x c++ $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$W/lsan_off.o"
$CC -x c $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$W/standalone_main.o"

declare -A RELAX=()

# Plug-in file loaders: the plug-in's own load_image() over mayhem/mock/ (an in-process stand-in for the
# libgimp wire-protocol calls) linked against the real sanitized libgimp.
LOADERS=(loader_xwd)
gtkcf="$(pkg-config --cflags gtk+-3.0)"; gtklf="$(pkg-config --libs gtk+-3.0)"
GL="-L$W/fuzz/libgimp -lgimp-3.0 -Wl,-rpath,$W/fuzz/libgimp"
$CC $SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS -O1 -I"$SRC/mayhem" $(gimp_inc "$W/fuzz") \
    $(pcflags) $gtkcf -c "$SRC/mayhem/mock/mock.c" -o "$W/mock.o" 2>"$W/mock.log" \
    || { cat "$W/mock.log"; exit 1; }
for t in "${LOADERS[@]}"; do
  obj="$W/$t.o"
  $CC $SANITIZER_FLAGS ${RELAX[$t]:-} -fsanitize=fuzzer-no-link $DEBUG_FLAGS -O1 -I"$SRC/mayhem" \
      $(gimp_inc "$W/fuzz") $(pcflags) $gtkcf -c "$SRC/mayhem/harness/$t.c" -o "$obj" 2>"$W/$t.log" \
      || { cat "$W/$t.log"; exit 1; }
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$obj" "$W/mock.o" "$W/lsan_off.o" \
      $GL $(gimp_lib "$W/fuzz") $(pclibs) $gtklf -lm -o "$SRC/$t"
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS "$obj" "$W/mock.o" "$W/standalone_main.o" "$W/lsan_off.o" \
      $GL $(gimp_lib "$W/fuzz") $(pclibs) $gtklf -lm -o "$SRC/$t-standalone"
done

# --- the test oracle: normal flags, plain libraries, dynamically linked ---
$CC -O1 $COVERAGE_FLAGS -I"$SRC/mayhem" $(gimp_inc "$W/plain") $(pcflags) \
    "$SRC/mayhem/oracle/kat.c" $(gimp_lib "$W/plain") $(pclibs) -lm -o "$SRC/mayhem/_build/kat"
