# syntax=docker/dockerfile:1.4
# EV Nova build kit: the one build host for every target (README.md). Build it from the repository root:
#   docker build --platform linux/amd64 --build-context inputs=input-files -t evnova-buildkit buildkit
# Wine runs the Metrowerks tools, Unicorn the Mac compiler plugin; Apple gcc_40-5494, cctools-port 877.8, ld64-97.17
# and the Darwin qsort are built from source; unpack/ lays out the rest from the inputs folder. The image holds
# Apple's SDK, the Metrowerks tools and your game copies: keep it private.
FROM --platform=linux/amd64 ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN dpkg --add-architecture i386 && apt-get update && apt-get install -y --no-install-recommends \
        wine wine32 wine64 python3 python3-pip p7zip-full unar unshield cpio gzip ca-certificates patch \
        build-essential gcc-7 g++-7 flex bison texinfo m4 make file clang llvm-dev libxml2-dev uuid-dev \
        libssl-dev libblocksruntime-dev autoconf automake libtool \
    && rm -rf /var/lib/apt/lists/*
RUN python3 -m pip install --no-cache-dir unicorn==2.1.4 importlib_resources

# a 32-bit Wine prefix, made once; no Mono or Gecko prompts
ENV WINEARCH=win32 WINEPREFIX=/root/.wine WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml="
RUN wineboot --init && wineserver -w

COPY applegcc/ /src/
ENV CCTOOLS=f01442232f185291c671578a81d74fe60eb189a2

# mac111's toolchain, from the sources in input-files/mac111-source (without them, or the Xcode dmg, mac111 is left
# out): Apple gcc 4.0.1 (build 5494) for i686 and powerpc with its Objective-C drivers, cctools-port at the commit
# the identical builds used, the Darwin qsort
RUN --mount=type=bind,from=inputs,source=mac111-source,target=/msrc set -e; cd /msrc; \
    if [ -f gcc_40-5494.tar.gz ]; then \
        sha256sum -c /src/mac111-source.sha256; \
        mkdir -p /root/build && tar xzf cctools-port-$CCTOOLS.tar.gz -C /root/build \
        && mv /root/build/cctools-port-$CCTOOLS /root/build/cctools-port; \
        bash /src/build_applegcc.sh /msrc/gcc_40-5494.tar.gz all; \
        cp /root/build/gcc_40-5494/gcc/tree-optimize.c /root/build/orig-tree-optimize.c; \
        cp /root/build/gcc_40-5494/gcc/tree-ssanames.c /root/build/orig-tree-ssanames.c; \
        bash /src/build_cc1obj.sh i686 && bash /src/build_cc1obj.sh powerpc && bash /src/build_darwin_qsort.sh; \
        rm -rf /root/build /root/build-objc; \
    else echo "no mac111-source: mac111 is left out"; fi

# the 10.4u SDK and the gcc4.0 package from your Xcode dmg, then ld64-97.17 (it needs the dmg's Mach-O headers)
RUN --mount=type=bind,from=inputs,target=/inputs set -e; D=/inputs/xcode_3.2.6_and_ios_sdk_4.3.dmg; \
    if [ -f "$D" ] && [ -f /opt/applegcc/i686/bin/cc1 ]; then \
        echo "c23582588df37f1bd62d1bd66abbec813802af2e60daf9a61bfca629028ee063  $D" | sha256sum -c -; \
        bash /src/extract_sdk.sh "$D" && rm -rf /opt/applegcc/work; \
        mkdir -p /opt/applegcc/ld64/dl && cp /inputs/mac111-source/ld64-97.17.tar.gz /opt/applegcc/ld64/dl/ \
        && cp /inputs/mac111-source/ld64-97-standalone-libunwind-headers.patch /opt/applegcc/ld64/dl/libunwind-headers.patch; \
        bash /src/build_ld64.sh && rm -rf /opt/applegcc/ld64/src /opt/applegcc/ld64/dl; \
    else echo "no Xcode dmg or mac111-source: mac111 is left out"; fi

# the Metrowerks tools, Apple's stub libraries and the game copies, unpacked from the inputs folder (unpack/)
COPY unpack/ /src/unpack/
RUN --mount=type=bind,from=inputs,target=/inputs python3 /src/unpack/unpack.py /inputs /opt/evnova

WORKDIR /repo
CMD ["python3", "/repo/buildkit/build.py", "all"]
