#!/bin/bash
set -e

OPENFHE_INSTALL_DIR="${OPENFHE_INSTALL_DIR:-/usr/local}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
NPROC=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

# macOS + Apple Clang: Homebrew's libomp headers are not on the default
# search path.  CPLUS_INCLUDE_PATH is read by clang unconditionally and
# cannot be overridden by CMake, so this is the most reliable fix.
OMP_PREFIX="${OMP_PREFIX:-$(brew --prefix libomp 2>/dev/null || echo "")}"
if [ -n "$OMP_PREFIX" ] && [ -d "$OMP_PREFIX/include" ]; then
    export CPLUS_INCLUDE_PATH="${OMP_PREFIX}/include${CPLUS_INCLUDE_PATH:+:$CPLUS_INCLUDE_PATH}"
    export C_INCLUDE_PATH="${OMP_PREFIX}/include${C_INCLUDE_PATH:+:$C_INCLUDE_PATH}"
    export LIBRARY_PATH="${OMP_PREFIX}/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"
    echo " libomp detected at   : $OMP_PREFIX"
fi

echo "================================================"
echo " HE-PCA Build Script"
echo "================================================"
echo " OpenFHE install prefix : $OPENFHE_INSTALL_DIR"
echo " Build type             : $BUILD_TYPE"
echo " Parallel jobs          : $NPROC"
echo ""

# -----------------------------------------------------------
# Step 1: Install OpenFHE if not found
# -----------------------------------------------------------
OPENFHE_FOUND=0
if [ -f "$OPENFHE_INSTALL_DIR/lib/cmake/OpenFHE/OpenFHEConfig.cmake" ] || \
   [ -d "$OPENFHE_INSTALL_DIR/include/openfhe" ]; then
    OPENFHE_FOUND=1
fi

if [ "$OPENFHE_FOUND" -eq 0 ]; then
    echo "[1/3] OpenFHE not detected. Building from source ..."

    OPENFHE_VERSION="v1.2.3"
    OPENFHE_SRC_DIR="/tmp/openfhe-src-$$"

    echo "  Cloning OpenFHE $OPENFHE_VERSION (with submodules) ..."
    git clone --depth 1 --branch "$OPENFHE_VERSION" --recurse-submodules \
        https://github.com/openfheorg/openfhe-development.git "$OPENFHE_SRC_DIR"

    echo "  Configuring ..."
    cmake "$OPENFHE_SRC_DIR" \
        -B "$OPENFHE_SRC_DIR/build" \
        -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$OPENFHE_INSTALL_DIR" \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_BENCHMARKS=OFF \
        -DBUILD_UNITTESTS=OFF

    echo "  Building (this may take 5-15 minutes) ..."
    cmake --build "$OPENFHE_SRC_DIR/build" -j "$NPROC"

    echo "  Installing (may require sudo) ..."
    sudo cmake --install "$OPENFHE_SRC_DIR/build"

    rm -rf "$OPENFHE_SRC_DIR"
    echo "  OpenFHE installed to $OPENFHE_INSTALL_DIR"
else
    echo "[1/3] OpenFHE found at $OPENFHE_INSTALL_DIR"
fi

# -----------------------------------------------------------
# Step 2: Build HE-PCA project
# -----------------------------------------------------------
echo ""
echo "[2/3] Building HE-PCA project ..."
cd "$SCRIPT_DIR"

cmake "$SCRIPT_DIR" \
    -B "$SCRIPT_DIR/build" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_PREFIX_PATH="$OPENFHE_INSTALL_DIR"

cmake --build "$SCRIPT_DIR/build" -j "$NPROC"

# -----------------------------------------------------------
# Step 3: Summary
# -----------------------------------------------------------
echo ""
echo "[3/3] Build complete!"
echo ""
echo "Executables are in: $SCRIPT_DIR/build/"
echo ""
echo "  Run tests:"
echo "    ./build/test_packing"
echo "    ./build/test_he_linalg"
echo "    ./build/test_pca"
echo ""
echo "  Run main program:"
echo "    ./build/he_pca --dim 8 --n 200 --k 3 --iter 15"
echo "    ./build/he_pca --data data/sample_data.csv --k 3"
