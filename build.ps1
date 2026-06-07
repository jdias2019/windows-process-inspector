taskkill /f /im pinspector.exe
rc /fo resource.res resource.rc
cl src/process_inspector.c /MT /EHsc /Fe:pinspector.exe -I. resource.res user32.lib gdi32.lib psapi.lib comctl32.lib -DPSAPI_VERSION=1

if ($LASTEXITCODE -eq 0) {
    Write-Host "compiled to -> pinspector.exe"
    .\pinspector.exe

} else {
    Write-Host "compile error"
    exit 1
}
