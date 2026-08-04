param(
    [string]$BuildDir = "$PSScriptRoot/build/Desktop_Qt_6_11_1_MSVC2022_64bit_Release",
    [ValidateSet("Release", "RelWithDebInfo", "Debug")]
    [string]$Configuration = "RelWithDebInfo",
    [string]$OutputDir = "$PSScriptRoot/dist",
    [string]$QtCreatorDir = "C:/Qt/Tools/qtcreator",
    [switch]$AllowDebug
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$buildPath = [System.IO.Path]::GetFullPath($BuildDir)
$outputPath = [System.IO.Path]::GetFullPath($OutputDir)
$pluginInfo = Join-Path $QtCreatorDir "bin/qtplugininfo.exe"

if (-not (Test-Path $buildPath -PathType Container)) {
    throw "Build directory does not exist: $buildPath"
}

$candidates = @(
    (Join-Path $buildPath "lib/qtcreator/plugins/DsComplete.dll"),
    (Join-Path $buildPath "$Configuration/lib/qtcreator/plugins/DsComplete.dll"),
    (Join-Path $buildPath "lib/qtcreator/plugins/$Configuration/DsComplete.dll")
)
$pluginPath = $candidates | Where-Object { Test-Path $_ -PathType Leaf } | Select-Object -First 1

if (-not $pluginPath) {
    throw "DsComplete.dll was not found under: $buildPath"
}

if (-not (Test-Path $pluginInfo -PathType Leaf)) {
    throw "qtplugininfo.exe was not found: $pluginInfo"
}

$metadataText = & $pluginInfo --full-json $pluginPath | Out-String
if ($LASTEXITCODE -ne 0) {
    throw "qtplugininfo failed with exit code $LASTEXITCODE."
}
$metadata = $metadataText | ConvertFrom-Json

if ($metadata.MetaData.Id -ne "dscomplete") {
    throw "Unexpected plugin ID: $($metadata.MetaData.Id)"
}
if ($metadata.debug -and -not $AllowDebug) {
    throw "Refusing to package a Debug plugin. Build Release or RelWithDebInfo, or pass -AllowDebug."
}

$version = $metadata.MetaData.Version
$nativeArchitecture = if ($env:PROCESSOR_ARCHITEW6432) {
    $env:PROCESSOR_ARCHITEW6432
} else {
    $env:PROCESSOR_ARCHITECTURE
}
$architecture = switch ($nativeArchitecture.ToUpperInvariant()) {
    "AMD64" { "x64" }
    "ARM64" { "arm64" }
    "X86" { "x86" }
    default { $nativeArchitecture.ToLowerInvariant() }
}
$packageName = "DsComplete-$version-windows-$architecture.zip"
$packagePath = Join-Path $outputPath $packageName
$checksumPath = "$packagePath.sha256"

New-Item -ItemType Directory -Force -Path $outputPath | Out-Null
if (Test-Path $packagePath) {
    Remove-Item $packagePath -Force
}

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::Open(
    $packagePath,
    [System.IO.Compression.ZipArchiveMode]::Create)
try {
    [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
        $archive,
        $pluginPath,
        "DsComplete.dll",
        [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
} finally {
    $archive.Dispose()
}

$archive = [System.IO.Compression.ZipFile]::OpenRead($packagePath)
try {
    if ($archive.Entries.Count -ne 1 -or $archive.Entries[0].FullName -ne "DsComplete.dll") {
        throw "The generated archive has an invalid plugin layout."
    }
} finally {
    $archive.Dispose()
}

$hash = (Get-FileHash -Path $packagePath -Algorithm SHA256).Hash.ToLowerInvariant()
[System.IO.File]::WriteAllText(
    $checksumPath,
    "$hash  $packageName`n",
    [System.Text.UTF8Encoding]::new($false))

Write-Host "Plugin:       $pluginPath"
Write-Host "Version:      $version"
Write-Host "Compat:       $($metadata.MetaData.CompatVersion)"
Write-Host "Package:      $packagePath"
Write-Host "SHA-256:      $hash"
Write-Host "Checksum file: $checksumPath"
