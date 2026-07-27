#!/bin/sh
# ============================================================================
# build-appimage.sh - Build a sharun-based AppImage for an SDL game
# ============================================================================
#
# Place and run the script in the source root directory.
#
# Self-contained: downloads quick-sharun and appimagetool if not found,
# builds the game from source with vendored (static) SDL3, bundles
# everything into a portable AppImage using sharun.
#
# Uses squashfs with zstd compression instead of DwarFS so that file
# manager thumbnailers (KDE Dolphin, GNOME Nautilus) can show the
# embedded icon without third-party plugins.
#
# The resulting AppImage works on any Linux distro (including old and
# musl-based ones) and does not require FUSE.
#
# Prerequisites: cmake, make, a C++ compiler, git, wget or curl, patchelf
#   On Debian/Ubuntu:  sudo apt install build-essential cmake git wget patchelf
#   On Fedora/RHEL:    sudo dnf install gcc-c++ cmake git wget patchelf
#   On openSUSE:       sudo zypper install gcc-c++ cmake git wget patchelf
#   On Arch:           sudo pacman -S base-devel cmake git wget patchelf
#   On Gentoo:         sudo emerge dev-build/cmake dev-vcs/git net-misc/wget dev-util/patchelf
#   On Slackware:      sbopkg -i cmake patchelf (wget/git ship with full install)
#   On WSL:            same as the underlying distro (Ubuntu/Debian/Fedora/etc.)
#
# Usage:
#   chmod +x build-appimage.sh
#   ./build-appimage.sh
#
# The finished AppImage lands in dist/
#
# To adapt this for another SDL game, change the variables below.
# ============================================================================

set -eu

# ---------------------------------------------------------------------------
# Game-specific settings - change these for each game
# ---------------------------------------------------------------------------

# Name of the binary that cmake produces (must match add_executable() name)
APP_NAME="CglpSDL3"

# Reverse-DNS application ID for the .desktop file ( GNOME demands that )
APP_ID="be.willemssoft.cglpsdl3"

# One-line description shown in desktop menus
APP_COMMENT="Classic arcade-like mini-games from Abagames"

# Desktop entry categories (semicolon-separated, must end with semicolon)
# Full list: https://specifications.freedesktop.org/menu-spec/latest/category-registry.html
APP_CATEGORIES="Game;ArcadeGame;"

# Path to the icon file inside the source tree (PNG, ideally 256x256)
APP_ICON="metadata/icon.png"

# Asset directories that ship with the binary.
# The game finds them via SDL_GetBasePath(), so they sit next to the
# executable inside the AppImage.
APP_ASSET_DIRS=""

# CMake flag to build SDL3 from the vendored submodules (static linking).
# Removes the need for SDL3 system packages.
CMAKE_EXTRA_FLAGS="-DUSE_VENDORED_SDL=ON"

# ---------------------------------------------------------------------------
# Build settings
# ---------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SOURCE_DIR="$SCRIPT_DIR"
BUILD_DIR="$SCRIPT_DIR/_build"
DIST_DIR="$SCRIPT_DIR/dist"
TOOLS_DIR="$BUILD_DIR/tools"
ARCH="$(uname -m)"

# Squashfs zstd compression level (1-22). 15 balances size, build speed,
# and decompression speed. Benchmarked against gzip 9, zstd 19, and DwarFS.
SQFS_COMP_LEVEL="${SQFS_COMP_LEVEL:-15}"

# ---------------------------------------------------------------------------
# Appimagetool download URL
# ---------------------------------------------------------------------------
# The new appimagetool from AppImage/appimagetool bundles its own
# mksquashfs with zstd support and auto-downloads the type-2 runtime.
# No system squashfs-tools package needed.

APPIMAGETOOL_BASE="https://github.com/AppImage/appimagetool/releases/download/continuous"

case "$ARCH" in
    x86_64)
        APPIMAGETOOL_URL="$APPIMAGETOOL_BASE/appimagetool-x86_64.AppImage"
        ;;
    aarch64)
        APPIMAGETOOL_URL="$APPIMAGETOOL_BASE/appimagetool-aarch64.AppImage"
        ;;
    *)
        echo "ERROR: Unsupported architecture: $ARCH" >&2
        echo "Only x86_64 and aarch64 are supported." >&2
        exit 1
        ;;
esac

# ---------------------------------------------------------------------------
# Cleanup previous build
# ---------------------------------------------------------------------------

cleanup() {
    echo "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR" "$DIST_DIR" "$TOOLS_DIR"
}

# ---------------------------------------------------------------------------
# Version detection - reads the latest vX.Y.Z git tag from git, otherwise
#                     set the version statically
# ---------------------------------------------------------------------------

detect_version() {
    cd "$SOURCE_DIR"
    APP_VERSION=$(git tag -l 'v*' --sort=-v:refname 2>/dev/null \
        | head -1 | sed 's/^v//')
    if [ -z "$APP_VERSION" ]; then
        APP_VERSION="git-$(git rev-parse --short HEAD 2>/dev/null || echo 'unknown')"
    fi
    export VERSION="$APP_VERSION"
    echo "Version: $APP_VERSION"
}

# ---------------------------------------------------------------------------
# Download quick-sharun if it is not available
# ---------------------------------------------------------------------------
# quick-sharun handles bundling the binary and its shared libraries with
# sharun into an AppDir. We only use it for the AppDir preparation step,
# NOT for the final packaging (that uses appimagetool with squashfs).

get_quick_sharun() {
    QUICK_SHARUN="$TOOLS_DIR/quick-sharun"

    if [ -x "$QUICK_SHARUN" ]; then
        echo "quick-sharun already present."
        return
    fi

    echo "Downloading quick-sharun..."
    _download "$QUICK_SHARUN" \
        "https://raw.githubusercontent.com/pkgforge-dev/Anylinux-AppImages/main/useful-tools/quick-sharun.sh"
    chmod +x "$QUICK_SHARUN"
    echo "quick-sharun ready."
}

# ---------------------------------------------------------------------------
# Download appimagetool if it is not available
# ---------------------------------------------------------------------------

get_appimagetool() {
    APPIMAGETOOL="$TOOLS_DIR/appimagetool"

    if [ -x "$APPIMAGETOOL" ]; then
        echo "appimagetool already present."
        return
    fi

    echo "Downloading appimagetool for $ARCH..."
    _download "$APPIMAGETOOL" "$APPIMAGETOOL_URL"
    chmod +x "$APPIMAGETOOL"
    echo "appimagetool ready."
}

# ---------------------------------------------------------------------------
# Portable download helper (wget preferred, curl fallback)
# ---------------------------------------------------------------------------

_download() {
    _dest="$1"
    _url="$2"
    if command -v wget >/dev/null 2>&1; then
        wget -q -O "$_dest" "$_url"
    elif command -v curl >/dev/null 2>&1; then
        curl -fSL -o "$_dest" "$_url"
    else
        echo "ERROR: wget or curl required" >&2
        exit 1
    fi
}

# ---------------------------------------------------------------------------
# Build the game
# ---------------------------------------------------------------------------

build_game() {
    echo ""
    echo "Building $APP_NAME..."
    echo "---------------------------------------------------------------"

    mkdir -p "$BUILD_DIR/cmake"
    cd "$BUILD_DIR/cmake"

    cmake "$SOURCE_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        $CMAKE_EXTRA_FLAGS

    make -j"$(nproc)"

    echo "$APP_NAME built."
}

# ---------------------------------------------------------------------------
# Create the .desktop file and prepare the icon
# ---------------------------------------------------------------------------

create_desktop_and_icon() {
    DESKTOP_FILE="$BUILD_DIR/${APP_ID}.desktop"
    cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Type=Application
Name=${APP_NAME}
Comment=${APP_COMMENT}
Exec=${APP_NAME}
Icon=${APP_ID}
Categories=${APP_CATEGORIES}
Terminal=false
StartupNotify=true
EOF
    echo "Desktop file: $DESKTOP_FILE"

    # quick-sharun wants ICON and DESKTOP as env vars pointing to files
    SRC_ICON="$SOURCE_DIR/$APP_ICON"
    if [ ! -f "$SRC_ICON" ]; then
        echo "WARNING: icon not found at $SRC_ICON"
        ICON_FILE="DUMMY"
    else
        # Rename to match Icon=${APP_ID} regardless of the source filename
        ICON_FILE="$BUILD_DIR/${APP_ID}.png"
        cp "$SRC_ICON" "$ICON_FILE"
    fi
}

# ---------------------------------------------------------------------------
# Debloat the AppDir
# ---------------------------------------------------------------------------
# quick-sharun pulls in everything ldd finds. For a 2D SDL game most of
# that is waste. We remove it AFTER quick-sharun runs but BEFORE packing.
#
# if it is still too big, run the appimage with the --appimage-help to
# get a full list of command line switches, as well as extracting the data.

debloat_appdir() {
    echo ""
    echo "Debloating AppDir..."
    echo "---------------------------------------------------------------"

    BEFORE=$(du -sm "$APPDIR" | cut -f1)

    # Mesa/OpenGL compiler stack - the game should use the HOST system's
    # GPU driver, not a bundled software renderer. These three alone are
    # ~192 MB.
    #
    # if there are issues like you described with WSL, try to comment that out
    #
    rm -f "$APPDIR"/lib/*/libLLVM*.so*
    rm -f "$APPDIR"/lib/*/libgallium*.so*
    rm -f "$APPDIR"/lib/*/libz3*.so*

    # ibus (input method framework) - 131 MB of data a game never touches
    rm -rf "$APPDIR"/share/ibus/

    # Unicode character database - 39 MB, not needed for game rendering
    rm -rf "$APPDIR"/share/unicode/

    # System icon themes (GIMP, LibreOffice, etc.) - the game's own icon
    # is at the AppDir root, these are just noise from the build host
    rm -rf "$APPDIR"/share/icons/

    # Color profiles - not relevant for a game
    rm -rf "$APPDIR"/share/color/

    # Locale data - the game doesn't use gettext/i18n.
    #               if you have that, you can delete everything except your own locale
    #               files
    rm -rf "$APPDIR"/lib/locale/

    # GNOME/desktop data that slipped in
    # should be inspected if something is needed there (but probably mostly not)
    rm -rf "$APPDIR"/share/gnome/
    rm -rf "$APPDIR"/share/images/
    rm -rf "$APPDIR"/share/menu/
    rm -rf "$APPDIR"/share/pam/

    AFTER=$(du -sm "$APPDIR" | cut -f1)
    echo "Debloated: ${BEFORE} MB -> ${AFTER} MB (removed $((BEFORE - AFTER)) MB)"
}

# ---------------------------------------------------------------------------
# Resolve .DirIcon symlink chains to real files
# ---------------------------------------------------------------------------
# The KDE AppImage thumbnailer (libappimage) cannot follow chained
# symlinks inside squashfs. quick-sharun sometimes creates:
#   .DirIcon -> app.png -> deep/path/icon.png
# We resolve any chain to a real file so every thumbnailer works.

fix_diricon() {
    if [ -L "$APPDIR/.DirIcon" ]; then
        RESOLVED=$(readlink -f "$APPDIR/.DirIcon")
        if [ -f "$RESOLVED" ]; then
            rm "$APPDIR/.DirIcon"
            cp -a "$RESOLVED" "$APPDIR/.DirIcon"
            echo "Resolved .DirIcon symlink chain to real file"
        fi
    fi

    for img in "$APPDIR"/*.png "$APPDIR"/*.svg; do
        if [ -L "$img" ] && [ -f "$img" ]; then
            RESOLVED=$(readlink -f "$img")
            rm "$img"
            cp -a "$RESOLVED" "$img"
        fi
    done

    # The standard appimagetool requires an icon file at the AppDir root
    # whose name matches the desktop file's Icon= field. quick-sharun
    # places the icon under its original name (e.g. blips.png), but the
    # desktop file may use a reverse-DNS id (e.g. be.willemssoft.blips).
    # Copy the icon with the expected name if it is missing.
    ICON_ID=$(grep '^Icon=' "$APPDIR"/*.desktop 2>/dev/null | head -1 | cut -d= -f2)
    if [ -n "$ICON_ID" ] && [ ! -f "$APPDIR/${ICON_ID}.png" ] && [ ! -f "$APPDIR/${ICON_ID}.svg" ]; then
        if [ -f "$APPDIR/.DirIcon" ]; then
            cp -a "$APPDIR/.DirIcon" "$APPDIR/${ICON_ID}.png"
            echo "Created ${ICON_ID}.png from .DirIcon for appimagetool"
        fi
    fi
}

# ---------------------------------------------------------------------------
# Assemble and package
# ---------------------------------------------------------------------------

make_appimage() {
    echo ""
    echo "Creating AppImage with sharun..."
    echo "---------------------------------------------------------------"

    cd "$BUILD_DIR"

    # The binary built by cmake (check CMakeLists.txt for the output directory)
    BINARY="$BUILD_DIR/cmake/Release/$APP_NAME"
    if [ ! -f "$BINARY" ]; then
        # Some cmake configs put it directly in the build dir
        BINARY="$BUILD_DIR/cmake/$APP_NAME"
    fi

    if [ ! -f "$BINARY" ]; then
        echo "ERROR: cannot find built binary" >&2
        exit 1
    fi

    # Tell quick-sharun where to put the result and what to call it
    export ARCH
    export OUTPATH="$DIST_DIR"
    export ICON="$ICON_FILE"
    export DESKTOP="$DESKTOP_FILE"
    export MAIN_BIN="$APP_NAME"

    # Step 1: Deploy the binary with quick-sharun.
    # This bundles the binary, its shared libraries, and the sharun loader
    # into an AppDir structure. For a statically-linked SDL3 game, the only
    # shared deps are glibc and libstdc++.
    "$QUICK_SHARUN" "$BINARY"

    # The AppDir is now at ./AppDir (quick-sharun's default)
    APPDIR="$BUILD_DIR/AppDir"

    # Step 2: Copy game assets next to the binary.
    # SDL_GetBasePath() returns the directory of the running executable,
    # and the game loads assets relative to that. In the sharun AppDir
    # layout the binary ends up in AppDir/bin/.
    for dir in $APP_ASSET_DIRS; do
        if [ -d "$SOURCE_DIR/$dir" ]; then
            cp -a "$SOURCE_DIR/$dir" "$APPDIR/bin/$dir"
        else
            echo "WARNING: asset directory '$dir' not found, skipping"
        fi
    done

    # Step 3: Debloat.
    debloat_appdir

    # Step 4: Fix .DirIcon for thumbnailer compatibility.
    fix_diricon

    # Step 5: Package as squashfs AppImage with zstd compression.
    echo ""
    echo "Packaging AppImage (squashfs, zstd level $SQFS_COMP_LEVEL)..."
    echo "---------------------------------------------------------------"

    APPIMAGE_OUT="$DIST_DIR/${APP_NAME}-${APP_VERSION}-anylinux-${ARCH}.AppImage"

    if ! "$APPIMAGETOOL" --no-appstream \
            --comp zstd \
            --mksquashfs-opt -Xcompression-level \
            --mksquashfs-opt "$SQFS_COMP_LEVEL" \
            "$APPDIR" \
            "$APPIMAGE_OUT"; then
        echo "ERROR: appimagetool failed" >&2
        exit 1
    fi

    echo ""
    echo "---------------------------------------------------------------"
    echo "Done!"
    ls -lh "$APPIMAGE_OUT" 2>/dev/null
    echo ""
    echo "To run it:"
    echo "  chmod +x $APPIMAGE_OUT"
    echo "  ./$APPIMAGE_OUT"
    echo "---------------------------------------------------------------"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

echo "============================================================"
echo "  Building AppImage for $APP_NAME"
echo "============================================================"
echo ""

cleanup
detect_version
get_quick_sharun
get_appimagetool
build_game
create_desktop_and_icon
make_appimage
