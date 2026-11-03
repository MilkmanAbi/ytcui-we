param([string]$MpvDirectory = '', [string]$ToolchainPrefix = 'C:\msys64\ucrt64')
$ErrorActionPreference = 'Stop'
$toolchainBin = Join-Path $ToolchainPrefix 'bin'
if (!(Test-Path -LiteralPath (Join-Path $toolchainBin 'g++.exe'))) { throw 'Install MSYS2 UCRT64 GCC, curl, OpenSSL, zlib and Ninja development packages first.' }
$originalTaskPath = $env:Path
try {
    $env:Path = $toolchainBin + ';' + $env:Path
    & cmake -S $PSScriptRoot -B (Join-Path $PSScriptRoot 'build-win') -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_CXX_COMPILER=$toolchainBin/g++.exe" "-DCMAKE_PREFIX_PATH=$ToolchainPrefix"
    if ($LASTEXITCODE) { throw 'CMake configuration failed.' }
    & cmake --build (Join-Path $PSScriptRoot 'build-win') --parallel 2
    if ($LASTEXITCODE) { throw 'Native build failed.' }
    & ctest --test-dir (Join-Path $PSScriptRoot 'build-win') --output-on-failure
    if ($LASTEXITCODE) { throw 'Native tests failed.' }
    $packageDirectory = Join-Path $PSScriptRoot 'bin'
    New-Item -ItemType Directory -Force -Path $packageDirectory | Out-Null
    $binary = Join-Path $PSScriptRoot 'build-win\ytcui-we.exe'
    Copy-Item -LiteralPath $binary -Destination $packageDirectory -Force
    $queue = [System.Collections.Generic.Queue[string]]::new()
    $seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $queue.Enqueue($binary)
    while ($queue.Count) {
        $current = $queue.Dequeue()
        foreach ($line in (& (Join-Path $toolchainBin 'objdump.exe') -p $current)) {
            if ($line -match 'DLL Name:\s*(\S+)') {
                $name = $Matches[1]
                if (!$seen.Add($name)) { continue }
                $dependency = Join-Path $toolchainBin $name
                if (Test-Path -LiteralPath $dependency) {
                    Copy-Item -LiteralPath $dependency -Destination $packageDirectory -Force
                    $queue.Enqueue($dependency)
                }
            }
        }
    }
    $assets = Join-Path $packageDirectory 'vendor\ejs'
    New-Item -ItemType Directory -Force -Path $assets | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'vendor\ejs') -File | Copy-Item -Destination $assets -Force
    if ($MpvDirectory) {
        foreach ($name in @('mpv.exe', 'mpv.com', 'd3dcompiler_43.dll')) {
            $source = Join-Path $MpvDirectory $name
            if (Test-Path -LiteralPath $source) { Copy-Item -LiteralPath $source -Destination $packageDirectory -Force }
        }
    }
    & (Join-Path $packageDirectory 'ytcui-we.exe') --version
    if ($LASTEXITCODE) { throw 'Packaged executable failed its smoke check.' }
} finally { $env:Path = $originalTaskPath }
