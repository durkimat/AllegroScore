#!/usr/bin/env bash
# package_appimage.sh - Build a standalone AllegroScore AppImage
# Autonomously downloads linuxdeploy and linuxdeploy-plugin-qt if not present.
# Outputs to: /artifacts/AllegroScore-x86_64.AppImage
#
# Key design notes:
# - linuxdeploy and linuxdeploy-plugin-qt are standalone AppImages run via
#   APPIMAGE_EXTRACT_AND_RUN=1 (no FUSE needed, works in Docker/CI)
# - Verovio SMuFL font resources are bundled with VEROVIO_DATA_DIR override
# - Qt platformplugins and imageformats are included for headless/offscreen use

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
ARTIFACTS_DIR="/artifacts"
APPNAME="AllegroScore"
APP_EXECUTABLE="$BUILD_DIR/AllegroScore"

# --- Auto-download linuxdeploy & plugin if not in ~/.local/bin ---
LINUXDEPLOY_DIR="$HOME/.local/bin"
mkdir -p "$LINUXDEPLOY_DIR"

# Download linuxdeploy AppImage if missing
if [ ! -x "$LINUXDEPLOY_DIR/linuxdeploy-x86_64.AppImage" ]; then
    echo "Downloading linuxdeploy AppImage..."
    LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    curl -L "$LINUXDEPLOY_URL" -o "$LINUXDEPLOY_DIR/linuxdeploy-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY_DIR/linuxdeploy-x86_64.AppImage"
fi

# Download linuxdeploy-plugin-qt AppImage if missing
if [ ! -x "$LINUXDEPLOY_DIR/linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
    echo "Downloading linuxdeploy-plugin-qt AppImage..."
    PLUGIN_URL="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    curl -L "$PLUGIN_URL" -o "$LINUXDEPLOY_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"
fi

# Ensure our paths are in PATH
export PATH="$LINUXDEPLOY_DIR:$PATH"

# Verify build exists
if [ ! -x "$APP_EXECUTABLE" ]; then
    echo "ERROR: Build executable not found at $APP_EXECUTABLE"
    echo "Build first: cd $PROJECT_DIR && mkdir build && cmake .. && cmake --build . -j$(nproc)"
    exit 1
fi

# --- Prepare AppImage filesystem root ---
APPIMAGE_TMP=$(mktemp -d)
APPIMAGE_ROOT="$APPIMAGE_TMP/AppRun-root"

# Copy the executable
mkdir -p "$APPIMAGE_ROOT/usr/bin"
cp "$APP_EXECUTABLE" "$APPIMAGE_ROOT/usr/bin/"

# --- Run linuxdeploy to bundle Qt 6 shared libraries and plugins ---
# linuxdeploy generates the complete AppDir with all needed .so files,
# platformplugins (offscreen, eglfs, etc.) and imageformats (png, xpm, etc.)
echo "Running linuxdeploy to bundle Qt 6..."
# --executable sets the path inside the AppImage to our binary
# --output appimage tells linuxdeploy to produce an AppImage (but we'll use
#   appimagetool later; here we just generate the AppDir)
# --plugin qt bundles the Qt platformplugins and imageformats automatically
linuxdeploy \
    --appimage "$APP_EXECUTABLE" \
    --output appimage \
    --plugin qt \
    --dir "$APPIMAGE_ROOT" \
    2>&1 || echo "linuxdeploy exited with status $?, continuing with manual copy fallback"

# If linuxdeploy didn't capture some plugins, copy them manually as fallback
if [ ! -d "$APPIMAGE_ROOT/usr/plugins" ]; then
    echo "Fallback: copying Qt plugins manually..."
    QT_PLUGIN_DIR=$(find /usr -path "*/qt6/plugins/platforms" -type d 2>/dev/null | head -1)
    if [ -n "$QT_PLUGIN_DIR" ]; then
        mkdir -p "$APPIMAGE_ROOT/usr/plugins"
        cp -r "$QT_PLUGIN_DIR"/* "$APPIMAGE_ROOT/usr/plugins/" 2>/dev/null || true
    fi
    QT_IMAGEFORMATS_DIR=$(find /usr -path "*/qt6/plugins/imageformats" -type d 2>/dev/null | head -1)
    if [ -n "$QT_IMAGEFORMATS_DIR" ]; then
        mkdir -p "$APPIMAGE_ROOT/usr/plugins/imageformats"
        cp "$QT_IMAGEFORMATS_DIR"/libq*.so "$APPIMAGE_ROOT/usr/plugins/imageformats/" 2>/dev/null || true
    fi
fi

# --- Copy Verovio SMuFL font resources ---
# Verovio bundles font data in a share/verovio/ directory; we copy it so
# VEROVIO_DATA_DIR can point to it inside the AppImage.
if [ -d "$BUILD_DIR/../_deps/verovio-src/data" ]; then
    echo "Copying Verovio font resources..."
    mkdir -p "$APPIMAGE_ROOT/usr/share/verovio"
    cp -r "$BUILD_DIR/../_deps/verovio-src/data/"* "$APPIMAGE_ROOT/usr/share/verovio/" 2>/dev/null || true
fi

# --- Create AppRun script ---
# Uses APPIMAGE_EXTRACT_AND_RUN=1 so the AppImage runs without FUSE
# (critical for Docker/CI environments). Platform is left unset so the
# user's native platform (CachyOS X11/Wayland) is used; the bundled
# plugins ensure all common backends are available.
cat > "$APPIMAGE_ROOT/AppRun" << 'EOF'
#!/bin/bash
# Run as extracted AppImage (no FUSE required)
export APPIMAGE_EXTRACT_AND_RUN=1

# Plugins and libraries
export QT_PLUGIN_PATH="${APPIMAGE_ROOT}/usr/plugins:${QT_PLUGIN_PATH:-}"
export LD_LIBRARY_PATH="${APPIMAGE_ROOT}/usr/lib:${LD_LIBRARY_PATH:-}"

# Verovio font resources - must match what vrv::Resources::SetPath expects
export VEROVIO_DATA_DIR="${APPIMAGE_ROOT}/usr/share/verovio"

# Execute the bundled executable
exec "${APPIMAGE_ROOT}/usr/bin/AllegroScore" "$@"
EOF
chmod +x "$APPIMAGE_ROOT/AppRun"

# --- Create AppStream metadata (minimal) ---
mkdir -p "$APPIMAGE_ROOT/meta"
cat > "$APPIMAGE_ROOT/meta/infocenter.xml" << 'EOFDESC'
<?xml version="1.0" encoding="UTF-8"?>
<updates>
</updates>
EOFDESC

# --- Generate AppImage with appimagetool ---
if command -v appimagetool &>/dev/null; then
    echo "Generating AppImage with appimagetool..."
    cd "$APPIMAGE_TMP"
    appimagetool "$APPIMAGE_TMP" "$ARTIFACTS_DIR/${APPNAME}-x86_64.AppImage"
    echo "AppImage created: $ARTIFACTS_DIR/${APPNAME}-x86_64.AppImage"
else
    echo "WARNING: appimagetool not found - AppImage not generated"
    echo "Run: sudo snap install appimagetool --edge  (or download from github.com/AppImage/appimagetool)"
    echo "The directory $APPIMAGE_TMP contains the bundled filesystem; use appimagetool to generate the .AppImage"
fi

# Cleanup temp directory
rm -rf "$APPIMAGE_TMP"

echo "AppImage packaging complete."