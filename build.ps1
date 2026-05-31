taskkill /f /im plister.exe
rc /fo resource.res resource.rc
cl src/process_lister.c /MT /EHsc /Fe:plister.exe -I. resource.res user32.lib gdi32.lib psapi.lib comctl32.lib -DPSAPI_VERSION=1

if ($LASTEXITCODE -eq 0) {
    Write-Host "compiled to -> plister.exe"
    .\plister.exe

} else {
    Write-Host "compile error"
    exit 1
}
