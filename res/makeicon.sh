#!/bin/sh
ICNS_BASE=../dist/macos/bundle/InputLeap.app/Contents/Resources
if ! which magick >/dev/null 2>&1; then
    echo "Need ImageMagick for this"
    exit 10
fi
cd "$(dirname "$0")" || exit $?
if [ ! -r input-leap.png ]; then
    echo "Use inkscape (or another vector graphics editor) to create input-leap.png from input-leap.svg first"
    exit 10
fi
rm -rf work || exit $?
mkdir -p work || exit $?
for s in 16 24 32 48 64 128 256 512 1024; do
    magick convert input-leap.png -resize "${s}x${s}" -depth 8 "work/${s}.png" || exit $?
done
# windows icon
magick convert work/{16,24,32,48,64,128}.png input-leap.png input-leap.ico || exit $?
# macos icon
png2icns "$ICNS_BASE/InputLeap.icns" work/{16,32,256,512,1024}.png || exit $?
rm -rf work
echo Done
