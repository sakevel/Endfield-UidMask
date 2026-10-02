$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$target=Join-Path $root 'third_party/minhook'
$commit='c3fcafdc10146beb5919319d0683e44e3c30d537'
if(-not(Test-Path -LiteralPath $target)){
    gh repo clone TsudaKageyu/minhook $target -- --branch v1.3.4 --depth 1
    if($LASTEXITCODE){throw 'Authenticated upstream clone failed'}
}
$actual=git -C $target rev-parse HEAD
if($LASTEXITCODE -or $actual -ne $commit){throw 'Unexpected MinHook revision; not overwriting'}
if(git -C $target status --porcelain){throw 'MinHook has local changes; not overwriting'}
Write-Output "MinHook verified: $actual"
