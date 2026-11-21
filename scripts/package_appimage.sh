#!/usr/bin/env bash
# package_appimage.sh - Build a standalone AllegroScore AppImage
# Requires: linuxdeploy, linuxdeploy-plugin-qt, appimagetool
# Outputs to: /artifacts/AllegroScore-x86_64.AppImage

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
ARTIFACTS_DIR="/artifacts"
APPNAME="AllegroScore"
APP_EXECUTABLE="$BUILD_DIR/AllegroScore"

# Verify build exists
if [ ! -x "$APP_EXECUTABLE" ]; then
    echo "ERROR: Build executable not found at $APP_EXECUTABLE"
    echo "Build first: cd $PROJECT_DIR && mkdir build && cmake .. && cmake --build . -j$(nproc)"
    exit 1
fi

# Create a temporary directory for the AppImage filesystem
APPIMAGE_TMP=$(mktemp -d)
APPIMAGE_ROOT="$APPIMAGE_TMP/AppRun-root"

# Copy the executable
mkdir -p "$APPIMAGE_ROOT/usr/bin"
cp "$APP_EXECUTABLE" "$APPIMAGE_ROOT/usr/bin/"

# Copy Qt 6 shared libraries using linuxdeploy
if command -v linuxdeploy &>/dev/null; then
    echo "Bundling Qt 6 shared libraries with linuxdeploy..."
    linuxdeploy --appimage "$APP_EXECUTABLE" --output appimage \
        --plugin qt --exit-whitelist /usr/bin/true 2>&1 || true
fi

# If linuxdeploy is not available, copy common Qt plugins and libraries manually
# This is a best-effort approach for systems without linuxdeploy
if [ ! -d "$APPIMAGE_ROOT/usr/bin/platforms" ]; then
    echo "Copying Qt platform plugins manually..."
    QT_PLUGIN_DIR=$(find /usr -path "*/qt6/plugins/platforms" -type d 2>/dev/null | head -1)
    if [ -n "$QT_PLUGIN_DIR" ]; then
        mkdir -p "$APPIMAGE_ROOT/usr/plugins"
        cp -r "$QT_PLUGIN_DIR"/* "$APPIMAGE_ROOT/usr/plugins/" 2>/dev/null || true
    fi
fi

# Copy imageformats plugin if it exists
QT_IMAGEFORMATS_DIR=$(find /usr -path "*/qt6/plugins/imageformats" -type d 2>/dev/null | head -1)
if [ -n "$QT_IMAGEFORMATS_DIR" ]; then
    mkdir -p "$APPIMAGE_ROOT/usr/plugins/imageformats"
    cp "$QT_IMAGEFORMATS_DIR"/libq*.so "$APPIMAGE_ROOT/usr/plugins/imageformats/" 2>/dev/null || true
fi

# Copy Verovio resources (font data) - point vrv::Resources::SetPath to bundled data
# Verovio bundles fonts in its installation share directory; we copy them
if [ -d "$BUILD_DIR/../_deps/verovio-src/data" ]; then
    echo "Copying Verovio font resources..."
    mkdir -p "$APPIMAGE_ROOT/usr/share/verovio"
    cp -r "$BUILD_DIR/../_deps/verovio-src/data/"* "$APPIMAGE_ROOT/usr/share/verovio/" 2>/dev/null || true
fi

# Copy additional Qt platforms/plugins that might be needed
for plugin in eglfs linuxfb minimal minimalegl vnc; do
    QPLUGIN="/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms/$plugin.so"
    if [ -f "$QPLUGIN" ]; then
        mkdir -p "$APPIMAGE_ROOT/usr/plugins/platforms"
        cp "$QPLUGIN" "$APPIMAGE_ROOT/usr/plugins/platforms/"
    fi
done

# Set environment variables in AppRun for the bundled resources
cat > "$APPIMAGE_ROOT/AppRun" << 'EOF'
#!/bin/bash
export QT_PLUGIN_PATH="${APPIMAGE_ROOT}/usr/plugins:${QT_PLUGIN_PATH:-}"
export QT_QPA_PLATFORM=offscreen
# Point Verovio to bundled font resources
export VEROVIO_DATA_DIR="${APPIMAGE_ROOT}/usr/share/verovio"
export LD_LIBRARY_PATH="${APPIMAGE_ROOT}/usr/lib:${LD_LIBRARY_PATH:-}"
exec "${APPIMAGE_ROOT}/usr/bin/AllegroScore" "$@"
EOF
chmod +x "$APPIMAGE_ROOT/AppRun"

# Create AppStream metadata (optional)
cat > "$APPIMAGE_ROOT/meta/infocenter.xml" << 'EOFDESC'
<?xml version="1.0" encoding="UTF-8"?>
<updates>
</updates>
EOFDESC

# Generate AppImage using appimagetool if available
if command -v appimagetool &>/dev/null; then
    echo "Generating AppImage with appimagetool..."
    cd "$APPIMAGE_TMP"
    appimagetool "$APPIMAGE_TMP" "$ARTIFACTS_DIR/${APPNAME}-x86_64.AppImage"
    echo "AppImage created: $ARTIFACTS_DIR/${APPNAME}-x86_64.AppImage"
else
    echo "WARNING: appimagetool not found - AppImage not generated"
    echo "Run: sudo snap install appimagetool --edge  (or download from github.com/AppImage/appimagetool)"
    echo "The directory $APPIMAGE_TMP contains the bundled filesystem ready for appimagetool"
    ls -la "$APPIMAGE_TMP/"
fi

# Cleanup
rm -rf "$APPIMAGE_TMP"

echo "AppImage packaging complete."