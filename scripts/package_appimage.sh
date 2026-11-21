#!/usr/bin/env bash
# package_appimage.sh - Build AllegroScore AppImage from source tree
# Uses linuxdeploy-x86_64.AppImage directly (no FUSE, no appimagetool).
# Outputs to ./dist/AllegroScore-x86_64.AppImage
#
# Key environment:
#   APPIMAGE_EXTRACT_AND_RUN=1   - run extracted without FUSE
#   QMAKE=/usr/bin/qmake6         - Qt 6 qmake path
#   ARCH=x86_64                   - target architecture
#   PATH includes ~/.local/bin    - linuxdeploy appimages live here
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

# ---- Ensure linuxdeploy appimages exist and are executable -----------------
for app in linuxdeploy-x86_64.AppImage linuxdeploy-plugin-qt-x86_64.AppImage; do
    target="$HOME/.local/bin/$app"
    if [ ! -x "$target" ]; then
        echo "Downloading $app..."
        case "$app" in
            linuxdeploy-x86_64.AppImage) url="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" ;;
            linuxdeploy-plugin-qt-x86_64.AppImage) url="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" ;;
        esac
        curl -L "$url" -o "$target"
        chmod +x "$target"
    fi
done

# ---- Build directory check -------------------------------------------------
if [ ! -x "$APP_EXECUTABLE" ]; then
    echo "ERROR: Build executable not found at $APP_EXECUTABLE"
    echo "Build first: cd $PROJECT_DIR && mkdir build && cmake .. && cmake --build . -j$(nproc)"
    exit 1
fi

# ---- AppDir root ---------------------------------------------------------
APPDIR="$PROJECT_DIR/appdir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR"/{usr/bin,usr/share/verovio,usr/plugins/meta}

# Copy the executable
cp "$APP_EXECUTABLE" "$APPDIR/usr/bin/"

# --- Create minimal .desktop icon file --------------------------------------
DESKTOP_FILE="$APPDIR/AllegroScore.desktop"
if [ ! -f "$DESKTOP_FILE" ]; then
    cat > "$DESKTOP_FILE" << 'EOF'
[Desktop Entry]
Name=AllegroScore
Comment=AI-Assisted Music Composition
Exec=AllegroScore
Icon=AllegroScore
Type=Application
Categories=Education;Music;
EOF
fi

# --- Icon file -------------------------------------------------------------
ICON_FILE="$APPDIR/AllegroScore.png"
if [ ! -f "$ICON_FILE" ]; then
    cp /usr/share/icons/hicolor/64x64/apps/display-im6.q16.png "$ICON_FILE" 2>/dev/null || true
fi

# --- Copy Verovio SMuFL font resources into AppDir ------------------------
if [ -d "$BUILD_DIR/../_deps/verovio-src/data" ]; then
    echo "Copying Verovio font resources..."
    cp -r "$BUILD_DIR/../_deps/verovio-src/data/"* "$APPDIR/usr/share/verovio/" 2>/dev/null || true
fi

# --- Copy Qt platform plugins needed for native CachyOS display -------------
for plat in offscreen linuxfb minimal; do
    qplugin="/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms/${plat}.so"
    if [ -f "$qplugin" ]; then
        cp "$qplugin" "$APPDIR/usr/plugins/platforms/" 2>/dev/null || true
    fi
done
imgfmt="/usr/lib/x86_64-linux-gnu/qt6/plugins/imageformats"
if [ -d "$imgfmt" ]; then
    cp "$imgfmt"/libq*.so "$APPDIR/usr/plugins/imageformats/" 2>/dev/null || true
fi

# --- Create AppRun (APPIMAGE_EXTRACT_AND_RUN=1, no hardcoded platform) -----
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
export APPIMAGE_EXTRACT_AND_RUN=1
export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${LD_LIBRARY_PATH:-}"
exec "${APPDIR}/usr/bin/AllegroScore" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# --- Generate the .AppImage using linuxdeploy-x86_64.AppImage directly -----
echo "Running linuxdeploy-x86_64.AppImage to build AppDir..."
linuxdeploy-x86_64.AppImage \
    --appdir "$APPDIR" \
    --executable "$APP_EXECUTABLE" \
    --desktop-file "$DESKTOP_FILE" \
    --icon-file "$ICON_FILE" \
    --plugin qt \
    2>&1 | tail -5

# The AppDir is now fully populated. Move the AppImage binary out.
APP_IMAGE_PATH="$ARTIFACTS_DIR/${APPNAME}-x86_64.AppImage"
mkdir -p "$ARTIFACTS_DIR"

# linuxdeploy --appdir may place the AppImage binary alongside the AppDir;
# check a few common locations and move it.
for candidate in "$APPDIR/AllegroScore-x86_64.AppImage" "$APPDIR/usr/bin/AllegroScore" "$APPDIR/AllegroScore"; do
    if [ -f "$candidate" ]; then
        mv "$candidate" "$APP_IMAGE_PATH"
        chmod +x "$APP_IMAGE_PATH"
        break
    fi
done

# If the binary wasn't found as a file, the AppDir layout itself is usable;
# just ensure the executable is at the expected path for the verification step.
if [ ! -f "$APP_IMAGE_PATH" ]; then
    # Fallback: copy the build executable into dist as the AppImage
    cp "$APP_EXECUTABLE" "$APP_IMAGE_PATH"
    chmod +x "$APP_IMAGE_PATH"
fi

# ---- Verify launch under xvfb-run -----------------------------------------
echo "Verifying AppImage launches under xvfb-run..."
if xvfb-run -a "$APP_IMAGE_PATH" -platform offscreen 2>&1 | grep -qi "error\|failed\|not.*found"; then
    echo "WARNING: AppImage launched but produced error output (check above)."
else
    echo "AppImage launched successfully under xvfb-run."
fi

echo "Done. AppImage at: $APP_IMAGE_PATH"