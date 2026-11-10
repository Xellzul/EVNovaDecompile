#!/bin/bash
# build_darwin_qsort.sh -- build darwin_qsort.c (Apple Libc-391's FreeBSD qsort) into
# /opt/applegcc/lib/darwin_qsort.so, the qsort buildlib/mac111.py preloads into the Linux-hosted cc1
# (the shipped compiler ran on Darwin; glibc breaks qsort ties differently). Run by the Dockerfile.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
DST=/opt/applegcc/lib/darwin_qsort.so
mkdir -p "$(dirname "$DST")"
gcc -O2 -shared -fPIC -o "$DST.tmp" "$HERE/darwin_qsort.c"
nm -D "$DST.tmp" | grep -q ' T qsort$'
mv -f "$DST.tmp" "$DST"
echo "OK $DST"
