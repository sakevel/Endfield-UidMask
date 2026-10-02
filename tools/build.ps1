param([string]$LupaPath='', [string]$ServicesTestExe='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
& "$PSScriptRoot/bootstrap.ps1"
$args=@('-S',$root,'-B',"$root/build",'-G','Visual Studio 17 2022','-A','x64',"-DZML_LUPA_PATH=$LupaPath","-DZML_SERVICES_TEST_EXE=$ServicesTestExe")
cmake @args
if($LASTEXITCODE){throw 'Configure failed'}
cmake --build "$root/build" --config Release
if($LASTEXITCODE){throw 'Build failed'}
ctest --test-dir "$root/build" -C Release --output-on-failure
if($LASTEXITCODE){throw 'Tests failed'}
