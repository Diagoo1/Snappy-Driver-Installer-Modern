param([Parameter(Mandatory=$true)][string]$QtSource)
$ErrorActionPreference='Stop'
$QtSource=[IO.Path]::GetFullPath($QtSource)
$rules=@(
    @{file='src/widgets/kernel/qwidget.cpp';old='context->painter->device()->devicePixelRatio()';new='context->painter->device()->devicePixelRatioF()';before='9cb4753e8d9ccf3a8de1edfa4ea14cd410602b2e62f060fd5afc4db2f324a29e';after='801a0668785e7362dc6b73f49a4a1b3d4cbb9574fbccb0d72da9b9cea63bb45b'},
    @{file='src/widgets/itemviews/qtreeview.cpp';old='q->devicePixelRatio()';new='q->devicePixelRatioF()';before='04b3917a589f1af8e6ec03383262aec6d64fb95ddc3ca714b9e14f642426f386';intermediate='22321ca16e11a680024a5608547f7f7f76ef6c5a96cdfbcb971cf750c5c204a0';cropOld='painter->drawPixmap(0, start, top, 0, end - current - 1, top.width(), top.height());';cropNew='painter->drawPixmap(0, start, top, 0, qRound((end - current - 1) * top.devicePixelRatio()), top.width(), top.height());';after='222c712c20d28ddbfa64c2df0c6f4713a7fcd0cd4bbfc7e11557993f2bc3970b'}
)
foreach($rule in $rules){
    $path=Join-Path $QtSource $rule.file
    $hash=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if($hash -eq $rule.after){continue}
    if($hash -ne $rule.before -and $hash -ne $rule.intermediate){throw ('Unexpected Qt 5.6.3 source: '+$rule.file+'. Review the SDK source before applying this patch.')}
    $text=[IO.File]::ReadAllText($path,[Text.Encoding]::UTF8).Replace($rule.old,$rule.new)
    if($rule.ContainsKey('cropOld')){$text=$text.Replace($rule.cropOld,$rule.cropNew)}
    [IO.File]::WriteAllText($path,$text,(New-Object Text.UTF8Encoding($false)))
    if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $rule.after){throw ('Qt patch verification failed: '+$rule.file)}
}
$receipt=@{schema=1;id='qt563-fractional-dpi-animation-cache';qt='5.6.3';archiveSha256='fef48529a6fc2617a30d75d952cb327c6be341fd104154993922184b3b3b4da1';description='Keep opacity-effect and animated-tree caches at the actual fractional device pixel ratio.';files=$rules}
[IO.File]::WriteAllText((Join-Path $QtSource 'SDIO_FRACTIONAL_DPI_PATCH.json'),($receipt|ConvertTo-Json -Depth 5),(New-Object Text.UTF8Encoding($false)))
Write-Output 'Qt 5.6.3 fractional-DPI source patch verified.'
