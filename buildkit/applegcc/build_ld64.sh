#!/bin/bash
# build_ld64.sh [workdir, default /opt/applegcc/ld64] -- build Apple's ld64-97.17 (the linker of Xcode 3.2.6) as a
# Linux binary; run by the Dockerfile. Inputs: the source tarball and MacPorts' standalone libunwind header
# patch, both put in WORKDIR/dl by the Dockerfile from input-files/mac111-source (SHA-256 checked again here),
# the Mach-O headers of the extracted Xcode 3.2.6 DevSDK (extract_sdk.sh), our shim headers (ld64shim/) and
# ld64-linux.patch.
# Host plumbing only: no MacPorts behaviour patch is taken (its ppc branch-island fix would change what
# the 2011 linker wrote). Research/mac-link.md lists every edit. Takes about two minutes.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
W=${1:-/opt/applegcc/ld64}
DEV=/opt/applegcc/xcode326/DevSDK/usr/include
echo "dc609d295365f8f5853b45e8dbcb44ca85e7dbc7a530e6fb5342f81d3c042db5  $W/dl/ld64-97.17.tar.gz" | sha256sum -c --quiet -
echo "370d02757ea628b5dd145c099e42fc4eb88cc09cf459a59e32d14bbc9b4a105e  $W/dl/libunwind-headers.patch" | sha256sum -c --quiet -

if [ ! -f "$W/src/.patched" ] || [ "$HERE/ld64-linux.patch" -nt "$W/src/.patched" ]; then
  [ -d "$W/src" ] && rm -r -- "$W/src"
  mkdir -p "$W/src"
  tar -xzf "$W/dl/ld64-97.17.tar.gz" -C "$W/src" --strip-components=1
  (cd "$W/src" && patch -s -p0 < "$W/dl/libunwind-headers.patch" && tr -d '\r' < "$HERE/ld64-linux.patch" | patch -s -p1)
  # ppc lazy pointers all carry the name of dyld_stub_binding_helper, so AtomByNameSorter ties on every pair and
  # the order is the std::sort implementation's: Xcode 3.2.6's libstdc++ 4.2.1 (ld64shim/applesort.h), not g++ 9's
  H=src/ld/MachOWriterExecutable.hpp
  sed -i '0,/#include/s//#include "applesort.h"\n#include/' "$W/src/$H"
  sed -i 's/std::sort(fAllSynthesizedLazyPointers\.begin/applestl::sort(fAllSynthesizedLazyPointers.begin/' "$W/src/$H"
  touch "$W/src/.patched"
fi

# Apple's Mach-O ABI headers, copied from the DevSDK: only these, so nothing shadows glibc.
I=$W/include
mkdir -p "$I/mach-o/ppc" "$I/mach-o/x86_64" "$I/mach"
for f in loader.h fat.h nlist.h stab.h reloc.h ranlib.h compact_unwind_encoding.h ppc/reloc.h x86_64/reloc.h; do
  cp -f "$DEV/mach-o/$f" "$I/mach-o/$f"
done
cp -f "$DEV/mach/machine.h" "$DEV/mach/vm_prot.h" "$I/mach/"
cp -f "$DEV/libunwind.h" "$I/"
cp -rf "$HERE"/ld64shim/* "$I/"

cd "$W/src"
: > src/ld/configure.h
FLAGS="-O1 -g -fno-delete-null-pointer-checks -fno-strict-aliasing -fwrapv -I$I -Isrc/abstraction -Isrc/ld -Isrc/ld/parsers -include $I/ld64shim.h -w -fpermissive -fno-access-control -std=gnu++98 -D__LITTLE_ENDIAN__=1"
g++ $FLAGS -c src/ld/ld.cpp -o src/ld/ld.o &
g++ $FLAGS -c src/ld/Options.cpp -o src/ld/Options.o &
wait
gcc -O1 -g -w -c src/ld/debugline.c -o src/ld/debugline.o
gcc -O1 -g -w -c "$I/ld64shim.c" -o src/ld/ld64shim.o
g++ -g src/ld/ld.o src/ld/Options.o src/ld/debugline.o src/ld/ld64shim.o -lcrypto -luuid -ldl -o "$W/ld"
ls -la "$W/ld"
"$W/ld" -v 2>&1 | head -1
