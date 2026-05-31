#include<windows.h>
#include<stdio.h>
#include<tchar.h>
#include<psapi.h>
#include<tlhelp32.h>
#include<commctrl.h>

#define MAX_PROCESSES 1000

struct process {
    TCHAR szProcessName[MAX_PATH];
    DWORD processID;
    DWORD parentProcessID;
};

HWND g_hWnd; // global window handle
struct process g_processes[MAX_PROCESSES];
int g_counter = 0;

void loadProcesses() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            _tcscpy(g_processes[g_counter].szProcessName, pe32.szExeFile);
            g_processes[g_counter].processID = pe32.th32ProcessID;
            g_processes[g_counter].parentProcessID = pe32.th32ParentProcessID;
            g_counter++;

        } while (Process32Next(hSnapshot, &pe32) && g_counter < MAX_PROCESSES);
    }

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

// window procedure function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    loadProcesses();
    
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icex);
    
    // register window class
    const char* CLASS_NAME = "MyWindowClass";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

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
        0, 0, 1200, 640,
        g_hWnd, NULL, hInstance, NULL
    );

    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMN lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.iSubItem = 0; lvc.pszText = TEXT("Process Name"); lvc.cx = 1025;
    SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);    
    lvc.iSubItem = 1; lvc.pszText = TEXT("PID"); lvc.cx = 80;
    SendMessage(hList, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc);
    lvc.iSubItem = 2; lvc.pszText = TEXT("PPID"); lvc.cx = 80;
    SendMessage(hList, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc);

    for (int i = 0; i < g_counter; i++) {

        TCHAR pidStr[32], ppidStr[32];
        wsprintf(pidStr, TEXT("%d"), g_processes[i].processID);
        wsprintf(ppidStr, TEXT("%d"), g_processes[i].parentProcessID);
        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = g_processes[i].szProcessName;
        SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&lvi);
        lvi.iSubItem = 1; lvi.pszText = pidStr;
        SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&lvi);
        lvi.iSubItem = 2; lvi.pszText = ppidStr;
        SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&lvi);
    }

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
