/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

#include <windows.h>
#include <commctrl.h>
#include <kvmutil.h>
#include "resource.h"

/*
 * Registry location of where the settings are stored
 */
#define kvmHKey HKEY_CURRENT_USER
WCHAR kvmKey[] = TEXT("SOFTWARE\\Sun Microsystems\\java\\KVM");

/* Prototypes */
static void GeneralDlgDefaults(HWND hWnd);
static void TraceDlgDefaults(HWND hWnd);
static void DebuggerDlgDefault(HWND hWnd);
static void
SaveToReg(HKEY hkey, DWORD type, WCHAR *keys, WCHAR *value, DWORD dwValue);
static bool_t
ReadFromReg(HKEY hkey, DWORD *type, WCHAR *key, BYTE *value, DWORD *size);

/* Creates the kvmutil window */
bool_t StartKVMUtil(HINSTANCE hInstance) {
    CreateRegKeys();
    return CreatePropertySheet(hInstance);
}

int CreatePropertySheet(HINSTANCE hInstance) {
    int numPages = 1;
    int page = 0;

    PROPSHEETHEADER psh;

    PROPSHEETPAGE *psp;

#if INCLUDEDEBUGCODE

    numPages += 2;

#endif

#if ENABLE_JAVA_DEBUGGER

    numPages++;

#endif

    psp = (PROPSHEETPAGE *)malloc(sizeof(PROPSHEETPAGE) * numPages);

    psp[page].dwSize      = sizeof(PROPSHEETPAGE);
    psp[page].dwFlags     = PSP_USETITLE;
    psp[page].hInstance   = hInstance;
    psp[page].pszTemplate = MAKEINTRESOURCE(IDD_GENERAL);
    psp[page].pszIcon     = NULL;
    psp[page].pfnDlgProc  = GeneralDlgProc;
    psp[page].pszTitle    = TEXT("General");
    psp[page].lParam      = 0;

#if INCLUDEDEBUGCODE

    page++;

    psp[page].dwSize      = sizeof(PROPSHEETPAGE);
    psp[page].dwFlags     = PSP_USETITLE;
    psp[page].hInstance   = hInstance;
    psp[page].pszTemplate = MAKEINTRESOURCE(IDD_TRACE);
    psp[page].pszIcon     = NULL;
    psp[page].pfnDlgProc  = TraceDlgProc;
    psp[page].pszTitle    = TEXT("Trace");
    psp[page].lParam      = 0;

    page++;

    psp[page].dwSize      = sizeof(PROPSHEETPAGE);
    psp[page].dwFlags     = PSP_USETITLE;
    psp[page].hInstance   = hInstance;
    psp[page].pszTemplate = MAKEINTRESOURCE(IDD_TRACE2);
    psp[page].pszIcon     = NULL;
    psp[page].pfnDlgProc  = Trace2DlgProc;
    psp[page].pszTitle    = TEXT("Trace (2)");
    psp[page].lParam      = 0;

#endif

#if ENABLE_JAVA_DEBUGGER

    page++;

    psp[page].dwSize      = sizeof(PROPSHEETPAGE);
    psp[page].dwFlags     = PSP_USETITLE;
    psp[page].hInstance   = hInstance;
    psp[page].pszTemplate = MAKEINTRESOURCE(IDD_DEBUGGER);
    psp[page].pszIcon     = NULL;
    psp[page].pfnDlgProc  = DebuggerDlgProc;
    psp[page].pszTitle    = TEXT("Debugger");
    psp[page].lParam      = 0;

#endif

    psh.dwSize     = sizeof(PROPSHEETHEADER);
    psh.dwFlags    = PSH_PROPSHEETPAGE;
    psh.hwndParent = NULL;
    psh.hInstance  = hInstance;
    psh.pszIcon    = NULL;
    psh.pszCaption = TEXT("KVM Properties");
    psh.nPages     = numPages;
    psh.ppsp       = psp;

    if (PropertySheet(&psh) < 0) {
        MessageBox( NULL,
            TEXT("KVM Util could not be loaded"),
            TEXT("Error"),
            MB_OK |
            MB_ICONERROR |
            MB_DEFBUTTON1 |
            MB_APPLMODAL |
            MB_SETFOREGROUND );
        return -1;
    }
    return 0;
}

/* Sets the Defaults for the General Page */
static void GeneralDlgDefaults(HWND hWnd) {
    int  heapsize;
    char buf[10];
    WCHAR buffer[10];

    if (GetDWFromReg(TEXT("output"))) {
        SendDlgItemMessage(hWnd, IDC_OUTPUT, BM_SETCHECK, BST_CHECKED, 0);
    }

    if (GetDWFromReg(TEXT("stdout"))) {
        SendDlgItemMessage(hWnd, IDC_STDOUT, BM_SETCHECK, BST_CHECKED, 0);
    }

    heapsize = GetDWFromReg(TEXT("heapsize"));
    if (!heapsize) {
        heapsize = DEFAULTHEAPSIZE/1024;
    } else {
        heapsize = heapsize/1024;
    }
    sprintf(buf, "%d", heapsize);
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buf, strlen(buf) + 1,
        buffer, sizeof(buffer));
    SendDlgItemMessage(hWnd, IDC_HEAPSIZE, TBM_SETPOS, TRUE, heapsize);
    SendDlgItemMessage(hWnd, IDC_HEAPDISPLAY, WM_SETTEXT, 0, (long)buffer);
}

/* Current Memory range is MIN_HEAPSIZE to MAX_HEAPSIZE */
BOOL CALLBACK
GeneralDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HKEY hkey;
    DWORD pos;
    char buf[10];
    WCHAR heapsize[10];

    switch(uMsg) {
        case WM_INITDIALOG:
                SendDlgItemMessage(hWnd, IDC_HEAPSIZE,
                    TBM_SETRANGE, TRUE, MAKELONG(MIN_HEAPSIZE, MAX_HEAPSIZE));
                SendDlgItemMessage(hWnd, IDC_HEAPSIZE,
                    TBM_SETTICFREQ, HEAPSIZE_INCR, 0);
                GeneralDlgDefaults(hWnd);
            return TRUE;
        case WM_HSCROLL:
            switch LOWORD(wParam) {
                case TB_BOTTOM:
                case TB_THUMBPOSITION:
                case TB_LINEUP:
                case TB_LINEDOWN:
                case TB_PAGEUP:
                case TB_PAGEDOWN:
                case TB_TOP:
                    pos = SendDlgItemMessage(hWnd,
                        IDC_HEAPSIZE, TBM_GETPOS, 0, 0);
                    sprintf(buf, "%d", pos -= pos % CELL);
                    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                        buf, strlen(buf) + 1,
                        heapsize, sizeof(heapsize));
                    SendDlgItemMessage(hWnd, IDC_HEAPDISPLAY,
                        WM_SETTEXT, 0, (long)heapsize);
                    return TRUE;
                }
            break;
        case WM_NOTIFY:
            switch(((NMHDR FAR *)lParam)->code) {
                case PSN_APPLY:

                    hkey = OpenRegKeys(kvmKey);

                    if (hkey != NULL) {
                        /* The output checkbox */
                        SaveDWORDToReg(hkey, TEXT("output"),
                            isChecked(hWnd, IDC_OUTPUT));

                        /* The stdout checkbox */
                        SaveDWORDToReg(hkey, TEXT("stdout"),
                            isChecked(hWnd, IDC_STDOUT));

                        /* The memory settings */
                        pos = SendDlgItemMessage(hWnd, IDC_HEAPSIZE,
                            TBM_GETPOS, 0, 0);
                        pos = pos * 1024;
                        pos -= pos % CELL;
                        SaveDWORDToReg(hkey, TEXT("heapsize"), pos);
                        CloseRegKeys(hkey);
                    }
                    break;
            }
            return TRUE;
    }
    return FALSE;
}

#if INCLUDEDEBUGCODE

static void TraceDlgDefaults(HWND hWnd) {

    if (GetDWFromReg(TEXT("tracebytecodes"))) {
        SendDlgItemMessage(hWnd, IDC_BYTECODES, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("traceclassloading"))) {
        SendDlgItemMessage(hWnd, IDC_CLASSLOADING, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("traceclassloadingverbose"))) {
        SendDlgItemMessage(hWnd, IDC_CLASSLOADINGVERBOSE, BM_SETCHECK, BST_CHECKED, 0);
    }
#if ENABLE_JAVA_DEBUGGER
    if (GetDWFromReg(TEXT("tracedebugger"))) {
        SendDlgItemMessage(hWnd, IDC_DEBUGGERTRACE, BM_SETCHECK, BST_CHECKED, 0);
    }
#endif
    if (GetDWFromReg(TEXT("traceexceptions"))) {
        SendDlgItemMessage(hWnd, IDC_EXCEPTIONS, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("traceevents"))) {
        SendDlgItemMessage(hWnd, IDC_EVENTS, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("traceframes"))) {
        SendDlgItemMessage(hWnd, IDC_FRAMES, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("tracegarbagecollection"))) {
        SendDlgItemMessage(hWnd, IDC_GC, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("tracegarbagecollectionverbose"))) {
        SendDlgItemMessage(hWnd, IDC_GCVERBOSE, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("tracememoryallocation"))) {
        SendDlgItemMessage(hWnd, IDC_MEMALLOC, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("tracemethodcalls"))) {
        SendDlgItemMessage(hWnd, IDC_METHODCALLS, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("tracemethodcallsverbose"))) {
        SendDlgItemMessage(hWnd, IDC_METHODCALLSVERBOSE, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("tracemonitors"))) {
        SendDlgItemMessage(hWnd, IDC_MONITORS, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("tracenetworking"))) {
        SendDlgItemMessage(hWnd, IDC_NETWORKING, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("tracestackchunks"))) {
        SendDlgItemMessage(hWnd, IDC_STACKCHUNKS, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("tracestackmaps"))) {
        SendDlgItemMessage(hWnd, IDC_STACKMAPS, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("tracethreading"))) {
        SendDlgItemMessage(hWnd, IDC_THREADING, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (GetDWFromReg(TEXT("traceverifier"))) {
        SendDlgItemMessage(hWnd, IDC_VERIFIER, BM_SETCHECK, BST_CHECKED, 0);
    }
}

/*
 * Using 2 TraceDlgProc because once the page is changed the
 * Checkboxes state are not saved when the okay button is pressed
 */

BOOL CALLBACK TraceDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HKEY hkey;

    switch(uMsg) {
        case WM_INITDIALOG:
            TraceDlgDefaults(hWnd);
            return TRUE;
        case WM_NOTIFY:
            switch(((NMHDR FAR *)lParam)->code) {
                case PSN_APPLY:

                    hkey = OpenRegKeys(kvmKey);

                    if (hkey != NULL) {
                            SaveDWORDToReg(hkey, TEXT("tracebytecodes"),
                                isChecked(hWnd, IDC_BYTECODES));
                            SaveDWORDToReg(hkey, TEXT("traceclassloading"),
                                isChecked(hWnd, IDC_CLASSLOADING));
                            SaveDWORDToReg(hkey, TEXT("traceclassloadingverbose"),
                                isChecked(hWnd, IDC_CLASSLOADINGVERBOSE));
#if ENABLE_JAVA_DEBUGGER
                            SaveDWORDToReg(hkey, TEXT("tracedebugger"),
                                isChecked(hWnd, IDC_DEBUGGERTRACE));
#endif
                            SaveDWORDToReg(hkey, TEXT("traceexceptions"),
                                isChecked(hWnd, IDC_EXCEPTIONS));
                            SaveDWORDToReg(hkey, TEXT("traceevents"),
                                isChecked(hWnd, IDC_EVENTS));
                            SaveDWORDToReg(hkey, TEXT("traceframes"),
                                isChecked(hWnd, IDC_FRAMES));
                            SaveDWORDToReg(hkey, TEXT("tracegarbagecollection"),
                                isChecked(hWnd, IDC_GC));
                            SaveDWORDToReg(hkey, TEXT("tracegarbagecollectionverbose"),
                                isChecked(hWnd, IDC_GCVERBOSE));
                            CloseRegKeys(hkey);
                    }
            }
            return TRUE;
    }
    return FALSE;
}

BOOL CALLBACK Trace2DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HKEY hkey;

    switch(uMsg) {
        case WM_INITDIALOG:
            TraceDlgDefaults(hWnd);
            return TRUE;
        case WM_NOTIFY:
            switch(((NMHDR FAR *)lParam)->code) {
                case PSN_APPLY:

                    hkey = OpenRegKeys(kvmKey);

                    if (hkey != NULL) {
                            SaveDWORDToReg(hkey, TEXT("tracememoryallocation"),
                                isChecked(hWnd, IDC_MEMALLOC));
                            SaveDWORDToReg(hkey, TEXT("tracemethodcalls"),
                                isChecked(hWnd, IDC_METHODCALLS));
                            SaveDWORDToReg(hkey, TEXT("tracemethodcallsverbose"),
                                isChecked(hWnd, IDC_METHODCALLSVERBOSE));
                            SaveDWORDToReg(hkey, TEXT("tracemonitors"),
                                isChecked(hWnd, IDC_MONITORS));
                            SaveDWORDToReg(hkey, TEXT("tracenetworking"),
                                isChecked(hWnd, IDC_NETWORKING));
                            SaveDWORDToReg(hkey, TEXT("tracestackchunks"),
                                isChecked(hWnd, IDC_STACKCHUNKS));
                            SaveDWORDToReg(hkey, TEXT("tracestackmaps"),
                                isChecked(hWnd, IDC_STACKMAPS));
                            SaveDWORDToReg(hkey, TEXT("tracethreading"),
                                isChecked(hWnd, IDC_THREADING));
                            SaveDWORDToReg(hkey, TEXT("traceverifier"),
                                isChecked(hWnd, IDC_VERIFIER));
                            CloseRegKeys(hkey);
                    }
            }
            return TRUE;
    }
    return FALSE;
}

#endif /* INCLUDEDEBUGCODE */

#if ENABLE_JAVA_DEBUGGER

static void DebuggerDlgDefault(HWND hWnd) {
    DWORD port;
    char buf[10];
    WCHAR buffer[10];

    if (GetDWFromReg(TEXT("debuggerActive"))) {
        SendDlgItemMessage(hWnd, IDC_DEBUGGER, BM_SETCHECK, BST_CHECKED, 0);
        port = GetDWFromReg(TEXT("debuggerPort"));
    } else {
        port = DEFAULT_DEBUG_PORT;
    }
    sprintf(buf, "%d", port);
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
        buf, strlen(buf) + 1,
        buffer, sizeof(buffer));
    SendDlgItemMessage(hWnd, IDC_DEBUGPORT, WM_SETTEXT, 0, (long)buffer);

    SendDlgItemMessage(hWnd, IDC_DEBUGGERSUSPEND, BM_SETCHECK, BST_CHECKED, 0);
}

/*
 * The default port is 2800
 */
BOOL CALLBACK DebuggerDlgProc(HWND hWnd, UINT uMsg,
    WPARAM wParam, LPARAM lParam) {

    int length;
    HKEY hkey;
    WCHAR port[10];
    char portNum[10];

    switch(uMsg) {
        case WM_INITDIALOG:
            DebuggerDlgDefault(hWnd);
            return TRUE;
        case WM_COMMAND:
            return TRUE;
        case WM_NOTIFY:
            switch(((NMHDR FAR *)lParam)->code) {
                case PSN_APPLY:

                    hkey = OpenRegKeys(kvmKey);

                    if (hkey != NULL) {
                        SaveDWORDToReg(hkey, TEXT("debuggerActive"),
                            isChecked(hWnd, IDC_DEBUGGER));
                        if (isChecked(hWnd, IDC_DEBUGGER)) {
                            /* Debugger Suspend */
                            SaveDWORDToReg(hkey, TEXT("debuggerSuspend"),
                                isChecked(hWnd, IDC_DEBUGGERSUSPEND));
                            /* Debugger Port */
                            length = (WORD)SendDlgItemMessage(hWnd,
                                IDC_DEBUGPORT, EM_LINELENGTH, 0, 0);
                            if (length == 0) {
                                MessageBox(NULL,
                                    TEXT("No port number specified using Default 2800"),
                                    TEXT("Error"),
                                    MB_OK | MB_ICONERROR |
                                    MB_DEFBUTTON1 | MB_APPLMODAL | MB_SETFOREGROUND);
                                SaveDWORDToReg(hkey,
                                    TEXT("debuggerPort"), DEFAULT_DEBUG_PORT);
                                break;
                            }
                            /* Store the number of characters into the first word */
                            *((LPWORD)port) = length;
                            SendDlgItemMessage(hWnd,
                                IDC_DEBUGPORT, EM_GETLINE, 0, (long)port);
                            WideCharToMultiByte(CP_ACP, 0,
                                port, -1, portNum,
                                sizeof(port), NULL, NULL);
                            SaveDWORDToReg(hkey,
                                TEXT("debuggerPort"), atoi(portNum));
                        }
                        CloseRegKeys(hkey);
                    }
                    break;
            }
            return TRUE;
    }
    return FALSE;
}

#endif /* ENABLE_JAVA_DEBUGGER */

DWORD isChecked(HWND hWnd, int key) {
    return SendDlgItemMessage(hWnd, key, BM_GETCHECK, 0, 0);
}

/*
 * Registry functions
 * OpenRegKeys - Opens the registry
 * CloseRegKeys - Closes the registry
 * SaveDWORDToReg - Saves a DWORD to the registry
 * SaveSZToReg - Saves a SZ to the registry
 * GetDWFromReg - Gets a DWORF from the registry
 */

/* Create the key HKEY_CURRENT_USER */
void CreateRegKeys() {
    HKEY hkey;
    DWORD status;

    if (RegOpenKeyEx(kvmHKey, kvmKey, 0, 0, &hkey)
        != ERROR_SUCCESS) {

        RegCreateKeyEx(kvmHKey, kvmKey, 0, NULL, 0, 0, NULL,
            &hkey, &status);
    }
    RegCloseKey(hkey);
}

HKEY OpenRegKeys(WCHAR *key) {
    HKEY hkey;

    if (RegOpenKeyEx(kvmHKey, key, 0, 0, &hkey)
        == ERROR_SUCCESS) {
        return hkey;
    }
    return NULL;
}

void CloseRegKeys(HKEY hKey) {
    RegCloseKey(hKey);
}

void SaveDWORDToReg(HKEY hkey, WCHAR *keys, DWORD dwValue) {
    SaveToReg(hkey, REG_DWORD, keys, NULL, dwValue);
}

void SaveSZToReg(HKEY hkey, WCHAR *keys, WCHAR *value) {
    SaveToReg(hkey, REG_DWORD, keys, value, 0);
}

static void SaveToReg(HKEY hkey, DWORD type,
    WCHAR *keys, WCHAR *value, DWORD dwValue) {

    if (RegOpenKeyEx(kvmHKey, kvmKey, 0, 0, &hkey)
        == ERROR_SUCCESS) {

        if (type == REG_SZ) {
            RegSetValueEx(hkey, keys, 0, REG_SZ,
                (BYTE *)value, (wcslen(value) + 1) * 2);
        } else if (type == REG_DWORD) {
            RegSetValueEx(hkey, keys, 0, REG_DWORD,
                (BYTE *) &dwValue, sizeof(dwValue));
        }
    }
}

static bool_t ReadFromReg(HKEY hkey, DWORD *type,
    WCHAR *key, BYTE *value, DWORD *size) {

    if (RegQueryValueEx(hkey, key, 0, type, value, size)
        == ERROR_SUCCESS) {
        return TRUE;
    }
    value = NULL;
    return FALSE;
}

DWORD GetDWFromReg(WCHAR *key) {
    HKEY hkey;
    DWORD type;
    DWORD size;
    long value;

    size = sizeof(value);

    hkey = OpenRegKeys(kvmKey);
    if (ReadFromReg(hkey, &type, key, (void *)&value, &size)) {
        return value;
    }
    CloseRegKeys(hkey);

    return 0;
}
