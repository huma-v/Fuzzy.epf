param (
    [ValidateSet("CompOnly", "EpfOnly")]
    [string] $Stage
)

$ErrorActionPreference = 'Stop'

if (Test-Path .env -PathType Leaf) {
    Get-Content .env | ForEach-Object {
        $name, $value = $_.split('=')
        Set-Item env:$name -Value $value
    }
}

if ($Stage -ne "EpfOnly") {
    $BUILD_ROOT = Get-Location

    New-Item build\Windows_x64 -ItemType Directory -Force
    
    $cmake = Start-Process "cmake" ("-G `"Visual Studio 17 2022`" -A x64" + `
                " --toolchain=`"${env:VCPKG_ROOT}\scripts\buildsystems\vcpkg.cmake`"" + `
                " -D1C_LIB_DIR=`"${BUILD_ROOT}\VNCOMPS83`"" + `
                " ..\..\FuzzyComp") -WorkingDirectory build\Windows_x64 -PassThru -Wait -NoNewWindow
    if ($cmake.ExitCode -ne 0) {
        throw "Error configuring x64 external component!";
    }

    $cmake = Start-Process "cmake" "--build . --config Release" `
                -WorkingDirectory build\Windows_x64 -PassThru -Wait -NoNewWindow
    if ($cmake.ExitCode -ne 0) {
        throw "Error building x64 external component!"
    }

    New-Item build\Windows_x86 -ItemType Directory -Force
    
    $cmake = Start-Process "cmake" ("-G `"Visual Studio 17 2022`" -A Win32" + `
                " --toolchain=`"${env:VCPKG_ROOT}\scripts\buildsystems\vcpkg.cmake`"" + `
                " -D1C_LIB_DIR=`"${BUILD_ROOT}\VNCOMPS83`"" + `
                " ..\..\FuzzyComp") -WorkingDirectory build\Windows_x86 -PassThru -Wait -NoNewWindow
    if ($cmake.ExitCode -ne 0) {
        throw "Error configuring x86 external component!";
    }

    $cmake = Start-Process "cmake" "--build . --config Release" `
                -WorkingDirectory build\Windows_x86 -PassThru -Wait -NoNewWindow
    if ($cmake.ExitCode -ne 0) {
        throw "Error building x86 external component!"
    }

    New-Item build\CompDir -ItemType Directory -Force

    Copy-Item build\Windows_x64\Release\FuzzyComp_Win_64.dll build\CompDir -Force
    Copy-Item build\Windows_x86\Release\FuzzyComp_Win_32.dll build\CompDir -Force
    Copy-Item FuzzyComp\MANIFEST.XML build\CompDir -Force

    Compress-Archive build\CompDir\* build\FuzzyComp.zip -Force
}

if ($Stage -ne "CompOnly") {
    Remove-Item build\FuzzyEpf -Recurse -ErrorAction SilentlyContinue
    Copy-Item FuzzyEpf build\FuzzyEpf -Recurse

    New-Item build\FuzzyEpf\FuzzyEpf\Templates\FuzzyComp\Ext -ItemType Directory
    Copy-Item build\FuzzyComp.zip build\FuzzyEpf\FuzzyEpf\Templates\FuzzyComp\Ext\Template.bin -Force

    # Build the epf
    $designer = Start-Process "${env:1C_HOME}\bin\1cv8.exe" "DESIGNER /LoadExternalDataProcessorOrReportFromFiles build\FuzzyEpf\FuzzyEpf.xml build\FuzzyEpf.epf" `
                    -PassThru -Wait -NoNewWindow
    if ($designer.ExitCode -ne 0) {
        throw "Error building epf!";
    }
}