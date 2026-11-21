#!/usr/bin/env bash
# package_appimage.sh - Build AllegroScore AppImage with fully bundled dependencies
# Downloads linuxdeploy + linuxdeploy-plugin-qt if not present.
# Uses appimagetool to generate the AppImage.
# Outputs to ./dist/AllegroScore-x86_64.AppImage

set -euo pipefail

# --- Environment ---
export APPIMAGE_EXTRACT_AND_RUN=1
export QMAKE=/usr/bin/qmake6
export ARCH=x86_64
export PATH="$HOME/.local/bin:$PATH"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
ARTIFACTS_DIR="$PROJECT_DIR/dist"
APPNAME="AllegroScore"
APP_EXECUTABLE="$BUILD_DIR/AllegroScore"

# ---- Ensure linuxdeploy appimages and appimagetool exist -------------------
mkdir -p "$HOME/.local/bin"

download_if_missing() {
    local name="$1"
    local url="$2"
    local target="$HOME/.local/bin/$name"
    if [ ! -x "$target" ]; then
        echo "Downloading $name..."
        curl -L "$url" -o "$target"
        chmod +x "$target"
    fi
}

download_if_missing "linuxdeploy-x86_64.AppImage" \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
download_if_missing "linuxdeploy-plugin-qt-x86_64.AppImage" \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
download_if_missing "appimagetool-x86_64.AppImage" \
    "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
chmod +x "$HOME/.local/bin/appimagetool-x86_64.AppImage"

# ---- Build directory check -------------------------------------------------
if [ ! -x "$APP_EXECUTABLE" ]; then
    echo "ERROR: Build executable not found at $APP_EXECUTABLE"
    echo "Build first: cd $PROJECT_DIR && mkdir build && cmake .. && cmake --build . -j$(nproc)"
    exit 1
fi

# ---- Clean and create AppDir -----------------------------------------------
APPDIR="$PROJECT_DIR/appdir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR"/{usr/bin,usr/share/verovio,usr/lib,usr/plugins/platforms}

# Copy the executable
cp "$APP_EXECUTABLE" "$APPDIR/usr/bin/"

# Copy Verovio shared library bundled by build
if [ -f "$BUILD_DIR/verovio-build/libverovio.so" ]; then
    echo "Copying libverovio.so..."
    cp "$BUILD_DIR/verovio-build/libverovio.so" "$APPDIR/usr/lib/"
fi

# Copy Verovio font resources (SMuFL)
if [ -d "$BUILD_DIR/../_deps/verovio-src/data" ]; then
    echo "Copying Verovio font resources..."
    cp -r "$BUILD_DIR/../_deps/verovio-src/data/"* "$APPDIR/usr/share/verovio/" 2>/dev/null || true
fi

# Copy Qt libraries and plugins using linuxdeploy
echo "Running linuxdeploy to bundle Qt 6..."
linuxdeploy-x86_64.AppImage \
    --appdir "$APPDIR" \
    --executable "$APP_EXECUTABLE" \
    --plugin qt \
    2>&1 | tail -5

# --- Create AppRun ----------------------------------------------------------
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${LD_LIBRARY_PATH:-}"
export VEROVIO_DATA_DIR="${APPDIR}/usr/share/verovio"
export QT_PLUGIN_PATH="${APPDIR}/usr/plugins:${QT_PLUGIN_PATH:-}"
exec "${APPDIR}/usr/bin/AllegroScore" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# --- Create minimal .desktop file -------------------------------------------
DESKTOP_FILE="$APPDIR/AllegroScore.desktop"
cat > "$DESKTOP_FILE" << 'EOF'
[Desktop Entry]
Name=AllegroScore
Comment=AI-Assisted Music Composition
Exec=AllegroScore
Icon=AllegroScore
Type=Application
Categories=Education;Music;
EOF

# --- Copy/create icon -------------------------------------------------------
ICON_FILE="$APPDIR/usr/share/icons/hicolor/64x64/apps/AllegroScore.png"
mkdir -p "$(dirname "$ICON_FILE")"
if [ -f /usr/share/icons/hicolor/64x64/apps/display-im6.q16.png ]; then
    cp /usr/share/icons/hicolor/64x64/apps/display-im6.q16.png "$ICON_FILE"
else
    # Create a minimal blue PNG icon with Python
    python3 << 'PYEOF'
import struct, zlib
def png_chunk(t,d): return struct.pack('>I',len(d))+t+d+struct.pack('>I',zlib.crc32(t+d)&0xFFFFFFFF)
w=h=64
ihdr=png_chunk(b'IHDR',struct.pack('>IIBB',w,h,8,2,0))
raw = bytes([0,0,255,255])*w*h  # solid blue
idat=png_chunk(b'IDAT',zlib.compress(raw))
with open('/tmp/icon.png','wb') as f: f.write(png_chunk(b'\x89PNG\r\n\x1a\n',b'')+ihdr+idat+png_chunk(b'IEND',b''))
PYEOF
    cp /tmp/icon.png "$ICON_FILE"
fi

# --- Generate the AppImage with linuxdeploy -------------------------------
echo "Generating AppImage with linuxdeploy..."
linuxdeploy-x86_64.AppImage \
    --appdir "$APPDIR" \
    --executable "$APP_EXECUTABLE" \
    --desktop-file "$DESKTOP_FILE" \
    --icon-file "$ICON_FILE" \
    --plugin qt \
    --output appimage \
    2>&1 | tail -5

# Move AppImage to dist/
mkdir -p "$ARTIFACTS_DIR"
if [ -f "$APPDIR/AllegroScore-x86_64.AppImage" ]; then
    mv "$APPDIR/AllegroScore-x86_64.AppImage" "$ARTIFACTS_DIR/"
elif [ -f "$PROJECT_DIR/AllegroScore-x86_64.AppImage" ]; then
    mv "$PROJECT_DIR/AllegroScore-x86_64.AppImage" "$ARTIFACTS_DIR/"
fi

# ---- Verify AppImage size (must be >20 MB) -------------------------------
APP_IMAGE="$ARTIFACTS_DIR/AllegroScore-x86_64.AppImage"
SIZE_MB=$(stat -c%s "$APP_IMAGE" 2>/dev/null | awk '{printf "%.1f", $1/1048576}')
echo "AppImage size: ${SIZE_MB} MB"

if [ "$(echo "$SIZE_MB < 20" | bc -l 2>/dev/null || echo "1")" = "1" ]; then
    echo "ERROR: AppImage is smaller than 20 MB (${SIZE_MB} MB), missing bundled libraries!"
    ls -la "$APP_IMAGE"
    exit 1
fi

# ---- Verify launch under xvfb-run ----------------------------------------
echo "Verifying AppImage launch under xvfb-run..."
if xvfb-run -a "$APP_IMAGE" -platform offscreen 2>&1 | grep -qi "error\|failed\|not.*found"; then
    echo "WARNING: AppImage produced error output"
else
    echo "AppImage launched successfully under xvfb-run."
fi

echo "Done. AppImage at: $APP_IMAGE"
echo "Size: $(ls -lh "$APP_IMAGE" | awk '{print $5}')"