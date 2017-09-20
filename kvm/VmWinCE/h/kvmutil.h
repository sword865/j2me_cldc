/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

#include <global.h>

#define MIN_HEAPSIZE       32
#define MAX_HEAPSIZE       4096
#define HEAPSIZE_INCR      64  /* This controls the increments for heapsize */

#define DEFAULT_DEBUG_PORT 2800

bool_t StartKVMUtil( HINSTANCE hInstance );
int CreatePropertySheet( HINSTANCE hInstance );

/*
 * Callback functions for all the KVMUtil pages
 */
BOOL CALLBACK
GeneralDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK
TraceDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK
Trace2DlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK
DebuggerDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

DWORD isChecked( HWND hWnd, int key );

/*
 * Functions used to access the registry
 * OpenRegKeys - Opens the registry
 * CloseRegKeys - Closes the registry
 * SaveDWORDToReg - Saves a DWORD to the registry
 * SaveSZToReg - Saves a SZ to the registry
 * GetDWFromReg - Gets a DWORF from the registry
 */
void CreateRegKeys();
HKEY OpenRegKeys( WCHAR *key );
void CloseRegKeys( HKEY hKey );
void SaveDWORDToReg( HKEY hkey, WCHAR *keys, DWORD dwValue );
void SaveSZToReg( HKEY hkey, WCHAR *keys, WCHAR *value );
DWORD GetDWFromReg( WCHAR *key );
