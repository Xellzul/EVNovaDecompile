#!/bin/bash
# build_applegcc.sh -- Apple gcc_40-5494 (gcc 4.0.1, Apple Inc. build 5494) as a Linux-hosted
# cross compiler for i686-apple-darwin10 and powerpc-apple-darwin10 (driver xgcc + cc1, enough
# for -S), plus the cctools-port assembler (i386 and ppc) for objects. Run by the Dockerfile (Ubuntu 20.04);
# each step is idempotent. About 3 min per gcc target and 1 min for cctools.
#   usage: build_applegcc.sh <gcc_40-5494.tar.gz> [i686|powerpc|cctools|all]
# Results: /opt/applegcc/<i686|powerpc>/bin/{xgcc,cc1,specs}: run xgcc -B<that dir>/ -S ...
#          /opt/applegcc/<i686|powerpc>/include: gcc's own headers (-isystem)
#          /opt/applegcc/cctools/libexec/as/<i386|ppc>/as -arch <i386|ppc> x.s -o x.o
#          /opt/applegcc/cctools/bin/{libtool,lipo,install_name_tool}: the link's archive and fat steps
# The gcc tarball: distfiles.macports.org/apple-gcc40/gcc_40-5494.tar.gz, SHA-256 as in the
# MacPorts apple-gcc40 Portfile.
set -e
TARBALL=${1:?tarball}
WHICH=${2:-all}
ROOT=/root/build
PREFIX=/opt/applegcc
export DEBIAN_FRONTEND=noninteractive

# (the host packages, gcc-7 among them, are the Dockerfile's)

# 1. source
mkdir -p $ROOT; cd $ROOT
if [ ! -d gcc_40-5494 ]; then tar xzf "$TARBALL"; fi
SRC=$ROOT/gcc_40-5494

# patch 1 (the only source patch): bison 3 no longer defines YYLEX, which Apple's c-parse.in
# calls directly (its CW-asm code). The parser is front end only; codegen is untouched.
if ! grep -q "M2 PATCH YYLEX" $SRC/gcc/c-parse.in; then
  sed -i '36a /* M2 PATCH YYLEX */\n#ifndef YYLEX\n#define YYLEX yylex ()\n#endif' $SRC/gcc/c-parse.in
fi

# 3. host compiler flags: gnu89 inline semantics, common symbols, no fortify, no warnings
HOSTCC="gcc-7"
HOSTCFLAGS="-O1 -g0 -std=gnu89 -fgnu89-inline -fcommon -D_FORTIFY_SOURCE=0 -w"

build_one() {
  local t=$1 extra=$2
  local obj=$ROOT/obj-$t
  mkdir -p $obj; cd $obj
  if [ ! -f Makefile ]; then
    CC="$HOSTCC" CFLAGS="$HOSTCFLAGS" \
    $SRC/configure --disable-checking --disable-nls --disable-werror \
      --enable-languages=c --prefix=$PREFIX/$t \
      --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=$t-apple-darwin10 $extra
  fi
  # Stop at the driver and cc1: libgcc would need the darwin assembler and linker.
  make -j16 all-build-libiberty all-libiberty all-libcpp CFLAGS="$HOSTCFLAGS"
  make configure-gcc CFLAGS="$HOSTCFLAGS"
  make -C gcc -j16 CFLAGS="$HOSTCFLAGS" xgcc cc1 specs
  mkdir -p $PREFIX/$t/bin
  cp gcc/xgcc gcc/cc1 gcc/specs $PREFIX/$t/bin/
  # gcc's own headers (stddef.h, stdarg.h, float.h ...), as the installed compiler has them
  mkdir -p $PREFIX/$t/include
  cp $SRC/gcc/ginclude/*.h $PREFIX/$t/include/
}

build_cctools() {
  # Branch 877.8-ld64-253.9-ppc keeps the ppc assembler and needs no libdispatch.
  cd $ROOT
  [ -d cctools-port ] || { echo "no $ROOT/cctools-port (the Dockerfile unpacks it from input-files/mac111-source)"; exit 2; }
  cd cctools-port/cctools
  [ -f configure ] || ./autogen.sh
  [ -f Makefile ] || CC=clang CXX=clang++ ./configure --prefix=$PREFIX/cctools --target=i686-apple-darwin10
  make -j16 -C libstuff
  make -j16 -C as
  make -C as install
  # the mac 1.1.1 link's archive, fat and install-name steps (buildlib/mac111.py)
  make -j2 -C misc libtool lipo install_name_tool
  mkdir -p $PREFIX/cctools/bin
  cp misc/libtool misc/lipo misc/install_name_tool $PREFIX/cctools/bin/
}

# Apple's build_gcc configures the i686 compilers --with-arch=apple --with-tune=generic.
case $WHICH in
  i686)    build_one i686 "--with-arch=apple --with-tune=generic" ;;
  powerpc) build_one powerpc "" ;;
  cctools) build_cctools ;;
  all)     build_one i686 "--with-arch=apple --with-tune=generic"; build_one powerpc ""; build_cctools ;;
esac
echo BUILD_OK
