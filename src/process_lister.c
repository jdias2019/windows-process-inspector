#include<windows.h>
#include<stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <tlhelp32.h>

#define MAX_PROCESSES 1000

WNDPROC g_origListProc;
static float g_scrollAccum = 0.0f;

struct process {
    TCHAR szProcessName[MAX_PATH];
    DWORD processID;
    DWORD parentProcessID;
};

LRESULT CALLBACK ListProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_MOUSEWHEEL) {
        short raw = GET_WHEEL_DELTA_WPARAM(wParam);
        g_scrollAccum += (float)raw / WHEEL_DELTA;

        while (g_scrollAccum >= 1.0f) {
            SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
            g_scrollAccum -= 1.0f;
        }
        while (g_scrollAccum <= -1.0f) {
            SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
            g_scrollAccum += 1.0f;
        }

        return 0;
    }

    return CallWindowProc(g_origListProc, hwnd, msg, wParam, lParam);
}

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

void listProcNameAndID (DWORD processID) {
    
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
        WS_OVERLAPPEDWINDOW,            // window style
        CW_USEDEFAULT, CW_USEDEFAULT,   // x, Y position
        800, 600,                       // width, Height
        NULL,                           // parent window
        NULL,                           // menu
        hInstance,                      // application instance
        NULL                            // additional data
    );

    if (g_hWnd == NULL) {
        return 0;
    }
   
    HWND hList = CreateWindowEx(
        0, "LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
        0, 0, 800, 600,
        g_hWnd, NULL, hInstance, NULL
    );

    g_origListProc = (WNDPROC)SetWindowLongPtr(hList, GWLP_WNDPROC, (LONG_PTR)ListProc);

    for (int i = 0; i < g_counter; i++) {
        char buffer[512];
        sprintf(buffer, "%-125s PID: %-10d PPID: %d",
        g_processes[i].szProcessName,
        g_processes[i].processID,
        g_processes[i].parentProcessID);
        SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)buffer); 
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
