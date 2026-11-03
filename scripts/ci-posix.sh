#!/bin/sh
set -eu
platform=${1:?platform argument required}
export PATH="/usr/local/bin:/usr/pkg/bin:$PATH"
python_bin=python3
if ! command -v "$python_bin" >/dev/null 2>&1; then
    for candidate in python3.14 python3.13 python3.12 python3.11; do
        if command -v "$candidate" >/dev/null 2>&1; then
            python_bin=$candidate
            break
        fi
    done
fi
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DYTWE_WERROR=ON "-DPython3_EXECUTABLE=$(command -v "$python_bin")"
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
"$python_bin" scripts/package.py --name "ytcui-we-$platform-x86_64"
