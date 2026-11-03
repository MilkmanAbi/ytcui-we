$clientExecutable = Join-Path $PSScriptRoot 'bin\ytcui-we.exe'
if (!(Test-Path -LiteralPath $clientExecutable)) { throw 'Build the native client with .\build-native.ps1 first.' }
& $clientExecutable @args
exit $LASTEXITCODE
