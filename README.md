# Windows Process Lister

Basic windows process lister that lists all running processes with their PID and PPID.

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 C++ dev kit (only to build from source)

## Usage

```powershell
.\plister.exe 
```

Click any column header to sort. Right-click a process to kill it.

## Build

```powershell
.\build.ps1
```
> If you don't trust the binary, build from source.

## Notes

- **Run as admin** for best results. Without admin rights, some processes won't show their uptime (displays as N/A) and can't be killed.
- **You can only kill processes** that your current privilege level allows. System processes will fail silently.

