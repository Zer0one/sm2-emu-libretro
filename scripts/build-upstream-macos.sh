#!/bin/bash
set -euo pipefail
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
port_dir="$(cd -- "$script_dir/.." && pwd)"
source_dir="${SM2_MAINSTREAM_DIR:-$(dirname -- "$port_dir")/sm2-emu-mainstream}"
build_dir="$source_dir/build-macos-release"
if [[ "$(uname -s)" != Darwin ]]; then
    echo "Questa build richiede macOS." >&2
    exit 1
fi
for tool in cmake ninja clang glslc; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Dipendenza mancante: $tool. Lo script non installa pacchetti." >&2
        exit 1
    fi
done
mkdir -p "$build_dir"
cmake -S "$source_dir" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DSM2_BUILD_VULKAN=ON \
    -DSM2_BUILD_OPENGL_DESKTOP=OFF \
    -DSM2_BUILD_OPENGL_ES=OFF \
    -DSM2_BUILD_TESTS=OFF 2>&1 | tee "$build_dir/configure.log"
cmake --build "$build_dir" --parallel "${SM2_BUILD_JOBS:-6}" \
    2>&1 | tee "$build_dir/build.log"
echo "App: $build_dir/bin/sm2-emu.app"
