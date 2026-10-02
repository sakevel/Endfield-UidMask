$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$package=Join-Path $root 'build/package/Release/uid-mask'
$manifest=Get-Content -LiteralPath (Join-Path $package 'zml-package.json') -Raw | ConvertFrom-Json
$dist=Join-Path $root 'dist';New-Item -ItemType Directory -Force $dist | Out-Null
$zip=Join-Path $dist ('EndfieldUidMask-0.1.4-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.zip')
$names=@($manifest.files)+@('zml-package.json')
foreach($name in $names){if($name -match '[\\/:]' -or -not(Test-Path -LiteralPath (Join-Path $package $name) -PathType Leaf)){throw 'Invalid release file'}}
Compress-Archive -LiteralPath @($names | ForEach-Object {Join-Path $package $_}) -DestinationPath $zip
(Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash+'  '+(Split-Path $zip -Leaf) | Set-Content -Encoding ascii ($zip+'.sha256')
Write-Output $zip
