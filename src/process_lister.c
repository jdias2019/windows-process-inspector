#include<windows.h>
#include<stdio.h>
#include<tchar.h>
#include<psapi.h>
#include<tlhelp32.h>
#include<commctrl.h>
#include<shellapi.h>
#include"resource.h"

#define MAX_PROCESSES 1000

struct process {
    TCHAR szProcessName[MAX_PATH];
    DWORD processID;
    DWORD parentProcessID;
    FILETIME upTime;
};

HWND g_hWnd; // global window handle
HWND g_hList;
HWND g_hBtn;
struct process g_processes[MAX_PROCESSES];
int g_counter = 0;
int g_sortAscending = 0;
int g_totalProcesses = 0;     
BOOL g_bInitialized = FALSE;
int g_sortColumn = 3;

void FormatUptime(FILETIME ft, TCHAR* buf, int size) {
    FILETIME now;
    GetSystemTimeAsFileTime(&now);
    ULARGE_INTEGER ulNow, ulStart;
    ulNow.LowPart = now.dwLowDateTime; ulNow.HighPart = now.dwHighDateTime;
    ulStart.LowPart = ft.dwLowDateTime; ulStart.HighPart = ft.dwHighDateTime;
    ULONGLONG secs = (ulNow.QuadPart - ulStart.QuadPart) / 10000000ULL;
    int d = (int)(secs / 86400), h = (int)((secs % 86400) / 3600), m = (int)((secs % 3600) / 60);
    if (d > 0) wsprintf(buf, TEXT("%dd %dh"), d, h);
    else if (h > 0) wsprintf(buf, TEXT("%dh %dm"), h, m);
    else wsprintf(buf, TEXT("%dm"), m > 0 ? m : 1);
}

void LogProcessCount(int count) {


}

void loadProcesses() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (pe32.th32ProcessID == 0) continue;
            _tcscpy(g_processes[g_counter].szProcessName, pe32.szExeFile);
            g_processes[g_counter].processID = pe32.th32ProcessID;
            g_processes[g_counter].parentProcessID = pe32.th32ParentProcessID;

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
            if (hProcess) {
                FILETIME ftExit, ftKernel, ftUser;
                GetProcessTimes(hProcess, &g_processes[g_counter].upTime, &ftExit, &ftKernel, &ftUser);
                CloseHandle(hProcess);
            } else {
                g_processes[g_counter].upTime.dwLowDateTime = 0;
                g_processes[g_counter].upTime.dwHighDateTime = 0;
            }
            g_counter++;
            g_totalProcesses++;  

        } while (Process32Next(hSnapshot, &pe32) && g_counter < MAX_PROCESSES);
    }

    g_bInitialized = TRUE; 
    CloseHandle(hSnapshot);
}

void listProcessNameAndID (DWORD processID) {
    
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

    // get process handle 
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
    
    // process name
    if (hProcess != NULL) {
          DWORD size = MAX_PATH;
          QueryFullProcessImageName(hProcess, 0, szProcessName, &size);
    }

    g_processes[g_counter].processID = processID;    

    _tcscpy(g_processes[g_counter].szProcessName, szProcessName);

    g_counter++;

    // release handle to process
    CloseHandle(hProcess);
}

int CompareByNameAsc(const void* a, const void* b) {
    return _tcscmp(((struct process*)a)->szProcessName, ((struct process*)b)->szProcessName);
}

int CompareByNameDesc(const void* a, const void* b) {
    return _tcscmp(((struct process*)b)->szProcessName, ((struct process*)a)->szProcessName);
}

int CompareByPIDAsc(const void* a, const void* b) {
    struct process* pa = (struct process*)a;
    struct process* pb = (struct process*)b;
    if (pa->processID < pb->processID) return -1;
    if (pa->processID > pb->processID) return 1;
    return 0;
}

int CompareByPIDDesc(const void* a, const void* b) {
    struct process* pa = (struct process*)a;
    struct process* pb = (struct process*)b;
    if (pa->processID > pb->processID) return -1;
    if (pa->processID < pb->processID) return 1;
    return 0;
}

int CompareByPPIDAsc(const void* a, const void* b) {
    struct process* pa = (struct process*)a;
    struct process* pb = (struct process*)b;
    if (pa->parentProcessID < pb->parentProcessID) return -1;
    if (pa->parentProcessID > pb->parentProcessID) return 1;
    return 0;
}

int CompareByPPIDDesc(const void* a, const void* b) {
    struct process* pa = (struct process*)a;
    struct process* pb = (struct process*)b;
    if (pa->parentProcessID > pb->parentProcessID) return -1;
    if (pa->parentProcessID < pb->parentProcessID) return 1;
    return 0;
}

int CompareByCreationDesc(const void* a, const void* b) {
    struct process* pa = (struct process*)a;
    struct process* pb = (struct process*)b;
    return CompareFileTime(&pb->upTime, &pa->upTime);
}

int CompareByCreationAsc(const void* a, const void* b) {
    struct process* pa = (struct process*)a;
    struct process* pb = (struct process*)b;
    return CompareFileTime(&pa->upTime, &pb->upTime);
}

void ReloadList() {
    SendMessage(g_hList, WM_SETREDRAW, FALSE, 0);
    SendMessage(g_hList, LVM_DELETEALLITEMS, 0, 0);

    for (int i = 0; i < g_counter; i++) {
        TCHAR pidStr[32], ppidStr[32], uptimeStr[32];
        wsprintf(pidStr, TEXT("%d"), g_processes[i].processID);
        wsprintf(ppidStr, TEXT("%d"), g_processes[i].parentProcessID);
        if (g_processes[i].upTime.dwLowDateTime == 0 && g_processes[i].upTime.dwHighDateTime == 0) {
            _tcscpy(uptimeStr, TEXT("N/A"));
        } else {
            FormatUptime(g_processes[i].upTime, uptimeStr, 32);
        }

        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = g_processes[i].szProcessName;
        SendMessage(g_hList, LVM_INSERTITEM, 0, (LPARAM)&lvi);
        lvi.iSubItem = 1; lvi.pszText = pidStr;
        SendMessage(g_hList, LVM_SETITEM, 0, (LPARAM)&lvi);
        lvi.iSubItem = 2; lvi.pszText = ppidStr;
        SendMessage(g_hList, LVM_SETITEM, 0, (LPARAM)&lvi);
        lvi.iSubItem = 3; lvi.pszText = uptimeStr;
        SendMessage(g_hList, LVM_SETITEM, 0, (LPARAM)&lvi);
    }

    SendMessage(g_hList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hList, NULL, TRUE);
}

// window procedure function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_NOTIFY: {
            LPNMHDR nmh = (LPNMHDR)lParam;
            if (nmh->hwndFrom == g_hList && nmh->code == LVN_COLUMNCLICK) {
                LPNMLISTVIEW lv = (LPNMLISTVIEW)lParam;
                int col = lv->iSubItem;

                if (col == g_sortColumn)
                    g_sortAscending = !g_sortAscending;
                else {
                    g_sortColumn = col;
                    g_sortAscending = 1;
                }

                if (col == 0) {
                    if (g_sortAscending)
                        qsort(g_processes, g_counter, sizeof(struct process), CompareByNameAsc);
                    else
                        qsort(g_processes, g_counter, sizeof(struct process), CompareByNameDesc);
                } else if (col == 1) {
                    if (g_sortAscending)
                        qsort(g_processes, g_counter, sizeof(struct process), CompareByPIDAsc);
                    else
                        qsort(g_processes, g_counter, sizeof(struct process), CompareByPIDDesc);
                } else if (col == 2) {
                    if (g_sortAscending)
                        qsort(g_processes, g_counter, sizeof(struct process), CompareByPPIDAsc);
                    else
                        qsort(g_processes, g_counter, sizeof(struct process), CompareByPPIDDesc);
                } else if (col == 3) {
                    if (g_sortAscending)
                        qsort(g_processes, g_counter, sizeof(struct process), CompareByCreationAsc);
                    else
                        qsort(g_processes, g_counter, sizeof(struct process), CompareByCreationDesc);
                }

                ReloadList();
            }

            break;
        }

        case WM_CONTEXTMENU: {
            int sel = SendMessage(g_hList, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
            if (sel != -1) {
                TCHAR msg[256];
                wsprintf(msg, TEXT("Kill %s?"), g_processes[sel].szProcessName);
                if (MessageBox(hwnd, msg, TEXT("Kill process"), MB_YESNO) == IDYES) {
                    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, g_processes[sel].processID);
                if (h) TerminateProcess(h, 0);
                    CloseHandle(h);
                }
            }

            break;
        }

        case WM_COMMAND:
            if ((HWND)lParam == g_hBtn) {
            g_counter = 0;
            loadProcesses();

            // reaply previous status
            if (g_sortColumn == 0) {
                if (g_sortAscending)
                    qsort(g_processes, g_counter, sizeof(struct process), CompareByNameAsc);
                else
                    qsort(g_processes, g_counter, sizeof(struct process), CompareByNameDesc);
            } else if (g_sortColumn == 1) {
                if (g_sortAscending)
                    qsort(g_processes, g_counter, sizeof(struct process), CompareByPIDAsc);
                else
                    qsort(g_processes, g_counter, sizeof(struct process), CompareByPIDDesc);
            } else if (g_sortColumn == 2) {
                if (g_sortAscending)
                    qsort(g_processes, g_counter, sizeof(struct process), CompareByPPIDAsc);
                else
                    qsort(g_processes, g_counter, sizeof(struct process), CompareByPPIDDesc);
            } else if (g_sortColumn == 3) {
                if (g_sortAscending)
                    qsort(g_processes, g_counter, sizeof(struct process), CompareByCreationAsc);
                else
                    qsort(g_processes, g_counter, sizeof(struct process), CompareByCreationDesc);
            }
              ReloadList();
            }

            break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
   
    loadProcesses();
    
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icex);
    
    // register window class
    const char* CLASS_NAME = "MyWindowClass";

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));

    RegisterClassEx(&wc);

    // create window
    g_hWnd = CreateWindowEx(
        0,                              // optional parameters
        CLASS_NAME,                     // window class name
        "Process Lister",               // window title
        WS_OVERLAPPED | WS_CAPTION | 
        WS_SYSMENU | WS_MINIMIZEBOX,    // window style (not resizeble)
        CW_USEDEFAULT, CW_USEDEFAULT,   // x, Y position
        1200, 650,                      // width, Height
        NULL,                           // parent window
        NULL,                           // menu
        hInstance,                      // application instance
        NULL                            // additional data
    );

    if (g_hWnd == NULL) {
        return 0;
    }
   
    HWND hList = CreateWindowEx(
        0, WC_LISTVIEW, NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LVS_REPORT,
        0, 30, 1200, 610,
        g_hWnd, NULL, hInstance, NULL
    );

    g_hList = hList;

    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMN lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.iSubItem = 0; lvc.pszText = TEXT("Process Name"); lvc.cx = 925;
    SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);    
    lvc.iSubItem = 1; lvc.pszText = TEXT("PID"); lvc.cx = 80;
    SendMessage(hList, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc);
    lvc.iSubItem = 2; lvc.pszText = TEXT("PPID"); lvc.cx = 80;
    SendMessage(hList, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc);
    lvc.iSubItem = 3; lvc.pszText = TEXT("Uptime"); lvc.cx = 100;
    SendMessage(hList, LVM_INSERTCOLUMN, 3, (LPARAM)&lvc);

    HWND hBtn = CreateWindowEx(0, "BUTTON", "Refresh", WS_CHILD | WS_VISIBLE, 0, 0, 80, 25, g_hWnd, (HMENU)101, hInstance, NULL);
    g_hBtn = hBtn;

    qsort(g_processes, g_counter, sizeof(struct process), CompareByCreationDesc);
    ReloadList();

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);


    // message loop
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
