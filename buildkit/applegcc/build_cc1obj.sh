#!/bin/bash
# build_cc1obj.sh <i686|powerpc> -- Apple gcc_40-5494 with the Objective-C front end, into /opt/applegcc/<t>/bin-objc/.
# build_applegcc.sh builds `--enable-languages=c` only; its xgcc does not know `-x objective-c` and
# there is no cc1obj. This rebuilds the same source tree (copied from /root/build/gcc_40-5494, the evtrace
# patches of tree-optimize.c / tree-ssanames.c reverted from /root/build/orig-tree-*.c) with
# `--enable-languages=c,objc` in /root/build-objc and installs xgcc, cc1, cc1obj and specs into
# /opt/applegcc/<t>/bin-objc/. buildlib/mac111.py uses that directory for a .m file. About 3 min per target.
# The bin-objc cc1 compiles ai.c to the same asm as the deployed one (both targets).
set -e
T=$1
ROOT=/root/build-objc
SRC=$ROOT/src
mkdir -p $ROOT
if [ ! -d $SRC ]; then
  cp -a /root/build/gcc_40-5494 $SRC
  cp /root/build/orig-tree-optimize.c $SRC/gcc/tree-optimize.c
  cp /root/build/orig-tree-ssanames.c $SRC/gcc/tree-ssanames.c
  find $SRC -name '*.o' -delete
fi
HOSTCC=gcc-7
HOSTCFLAGS="-O1 -g0 -std=gnu89 -fgnu89-inline -fcommon -D_FORTIFY_SOURCE=0 -w"
OBJ=$ROOT/obj-$T
mkdir -p $OBJ; cd $OBJ
case $T in i686) EXTRA="--with-arch=apple --with-tune=generic";; powerpc) EXTRA="";; esac
if [ ! -f Makefile ]; then
  CC="$HOSTCC" CFLAGS="$HOSTCFLAGS" $SRC/configure --disable-checking --disable-nls --disable-werror \
    --enable-languages=c,objc --prefix=/opt/applegcc/$T-objc \
    --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=$T-apple-darwin10 $EXTRA
fi
make -j4 all-build-libiberty all-libiberty all-libcpp CFLAGS="$HOSTCFLAGS"
make configure-gcc CFLAGS="$HOSTCFLAGS"
make -C gcc -j4 CFLAGS="$HOSTCFLAGS" xgcc cc1 cc1obj specs
D=/opt/applegcc/$T/bin-objc
mkdir -p $D
cp gcc/xgcc gcc/cc1 gcc/cc1obj gcc/specs $D/
echo BUILD_OK $T
