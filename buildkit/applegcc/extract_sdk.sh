#!/bin/bash
# extract_sdk.sh [DMG] -- unpack the Xcode 3.2.6 pieces the mac 1.1.1 project needs, from the
# user's OWN copy of xcode_3.2.6_and_ios_sdk_4.3.dmg (input-files/; the Dockerfile checks its SHA-256 and
# passes it here).
# Needs p7zip-full, cpio, gzip. Nothing here is ever run (they are Mach-O); nothing extracted is
# committed (scratch/ is git-ignored, /opt is outside the tree).
#
# Output (Linux filesystem: the SDK has symlinks that NTFS cannot keep):
#   /opt/applegcc/xcode326/<pkg>/      each package payload, Xcode's /Developer-relative layout
#   /opt/applegcc/sdk/MacOSX10.4u.sdk  symlink to the 10.4u SDK inside xcode326/MacOSX10.4.Universal
#   /opt/applegcc/work/                the dmg's package files (deletable afterwards)
# Each step skips work already done, so a re-run is cheap.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
DMG=${1:-$REPO/scratch/xcode326/dl/xcode_3.2.6_and_ios_sdk_4.3.dmg}
ROOT=${APPLEGCC:-/opt/applegcc}
W=$ROOT/work
PKGS="MacOSX10.4.Universal DeveloperTools DeveloperToolsCLI gcc4.0 DevSDK DeveloperToolsSystemSupport VersionedDeveloperToolsSystemSupport"
D="Xcode and iOS SDK/Packages"

[ -f "$DMG" ] || { echo "no dmg: $DMG"; exit 2; }
command -v 7z >/dev/null || { echo "apt-get -y install p7zip-full cpio"; exit 2; }
mkdir -p "$W/dmg" "$ROOT/xcode326" "$ROOT/sdk"

# 1. the packages out of the HFS+ image (7z decompresses the dmg stream: a few minutes)
need=
for p in $PKGS; do [ -f "$W/dmg/$D/$p.pkg" ] || need="$need $p"; done
if [ -n "$need" ]; then
    args=()
    for p in $PKGS; do args+=("$D/$p.pkg"); done
    args+=("$D/.hidden/version.plist")
    7z x -y -o"$W/dmg" "$DMG" "${args[@]}" > "$W/7z-dmg.log" 2>&1
fi

# 2. each .pkg is a xar; its Payload is gzip(cpio). cpio keeps the symlinks and hard links.
for p in $PKGS; do
    out=$ROOT/xcode326/$p
    [ -f "$out/.done" ] && continue
    rm -rf -- "$W/pkg/$p"
    mkdir -p "$W/pkg/$p" "$out"
    7z x -y -o"$W/pkg/$p" "$W/dmg/$D/$p.pkg" > "$W/7z-$p.log" 2>&1 || [ $? -eq 1 ]   # 1 = warning only
    # 7z may already have gunzipped the payload (it leaves it as `Payload~`)
    pl=$W/pkg/$p/Payload
    [ -f "$pl" ] || pl=$W/pkg/$p/Payload~
    case $(head -c 2 "$pl" | od -An -tx1 | tr -d ' ') in
        1f8b) un="gzip -dc" ;;
        *)    un=cat ;;
    esac
    (cd "$out" && $un "$pl" | cpio -idm --no-absolute-filenames --no-preserve-owner) \
        > "$W/cpio-$p.log" 2>&1
    cp "$W/pkg/$p/PackageInfo" "$out/.PackageInfo" 2>/dev/null || true
    touch "$out/.done"
    echo "unpacked $p"
done

# 3. the SDK where the mac compile line looks for it
SDK=$(find "$ROOT/xcode326/MacOSX10.4.Universal" -maxdepth 3 -name MacOSX10.4u.sdk | head -1)
[ -n "$SDK" ] || { echo "MacOSX10.4u.sdk not found in the 10.4 package"; exit 1; }
ln -sfn "$SDK" "$ROOT/sdk/MacOSX10.4u.sdk"
cp "$W/dmg/$D/.hidden/version.plist" "$ROOT/xcode326/version.plist" 2>/dev/null || true
echo "SDK: $ROOT/sdk/MacOSX10.4u.sdk -> $SDK"
