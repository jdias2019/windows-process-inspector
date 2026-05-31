taskkill /f /im plister.exe
cl src/process_lister.c /MT /EHsc /Fe:plister.exe user32.lib gdi32.lib psapi.lib comctl32.lib dwmapi.lib uxtheme.lib -DPSAPI_VERSION=1

if ($LASTEXITCODE -eq 0) {
    Write-Host "compiled to -> plister.exe"
    .\plister.exe

} else {
    Write-Host "compile error"
    exit 1
}
