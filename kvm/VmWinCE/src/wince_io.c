/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

#include <global.h>
#include <windows.h>
#include <wince_io.h>
#include <windowsx.h>

#define MAXPRINTS 10000
#define MAXBUFFER 512

/* Directory where stdout and stderr are stored */
WCHAR KVM_DIR[] = TEXT("\\KVM");

FILE *fout;
FILE *ferr;
int stdoutversion = 0;
int stderrversion = 0;
int stdoutprint   = 0;
int stderrprint   = 0;

FILE *xstdout;
FILE *xstderr;

BOOL use_console = FALSE;

void print_raw(const char* s);

void closestdout() {
    if(fout != 0) {
        fclose(fout);
        fout = 0;
    }
}

void unlink(char *filename) {
    WCHAR file[MAX_PATH];

    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
        filename, strlen(filename) + 1,
        file, sizeof(file));

    DeleteFile(file);
}

void ensurestdoutopen() {
    char buffer[MAX_PATH];
    if(stdoutprint++ == MAXPRINTS) {
        stdoutprint = 0;
        closestdout();
    }
    if(fout == 0) {
        sprintf(buffer, "\\KVM\\stdout%04d.txt", stdoutversion++);
        fout = fopen(buffer, "w");
    }
    if(stdoutversion > 200) {
        sprintf(buffer, "\\KVM\\stdout%04d.txt", stdoutversion - 100);
        unlink(buffer);
    }
}

void closestderr() {
    if(ferr != 0) {
        fclose(ferr);
        ferr = 0;
    }
}

void ensurestderropen() {
    char buffer[MAX_PATH];
    if(stderrprint++ == MAXPRINTS) {
        stderrprint = 0;
        closestderr();
    }
    if(ferr == 0) {
        sprintf(buffer, "\\KVM\\stderr%04d.txt", stderrversion++);
        ferr = fopen( buffer, "w" );
    }
    if(stderrversion > 200) {
        sprintf(buffer, "\\KVM\\stderr%04d.txt", stderrversion - 100);
        unlink(buffer);
    }
}

int printf(const char *formatStr, ...) {
    FILE *stream = NULL;
    char line[MAXBUFFER];
    int numChars;
    va_list args;

    ensurestdoutopen();
    stream = fout;

    va_start(args, formatStr);
    numChars = vfprintf(stream, formatStr, args);
    vsprintf(line, formatStr, args);
    if (use_console) {
        print_raw(line);
    }
    va_end(args);
    fflush(stream);
    return numChars;
}

int fprintf(LOGFILEPTR file, const char *formatStr, ...) {
    FILE *stream = NULL;
    char line[MAXBUFFER];
    int numChars;
    va_list args;

    if (file == xstdout) {
        ensurestdoutopen();
        stream = fout;
    } else if (file == xstderr) {
        ensurestderropen();
        stream = ferr;
    } else {
        stream = stdout;
    }

    va_start(args, formatStr);
    numChars = vfprintf(stream, formatStr, args);
    vsprintf(line, formatStr, args);
    if (use_console) {
        print_raw(line);
    }
    va_end(args);
    fflush(stream);
    return numChars;
}

void openstdio(void) {
    CreateDirectory(KVM_DIR, NULL);

    xstdout = stdout;
    xstderr = stderr;
    fprintf(stdout, "stdout opened\n");
    fprintf(stderr, "stderr opened\n");
}

void closestdio(void) {
    closestdout();
    closestderr();
}

/* The following is used to display stdout onto the screen */
BOOL consoleInitInstance(HINSTANCE hInstance);
LRESULT CALLBACK consoleWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

const int max_line = MAXBUFFER;
int  _line_pos     = 0;
char _line_buffer[MAXBUFFER];

HWND hWindow;
HWND hEdit;
HINSTANCE hInst;
TCHAR szConsoleTitle[]  = TEXT("KVM");
TCHAR szConsoleWindow[] = TEXT("KVM");

static PTCHAR Title()                          { return szConsoleTitle; }
static HINSTANCE AppInstance()                 { return hInst; }
static HWND MainWindow()                       { return hWindow; }
static HWND EditWindow()                       { return hEdit; }

/* Creates the window */
void openConsole(void) {
   HINSTANCE    hInstance = GetModuleHandle(NULL);

   use_console = TRUE;

    /* Check if an instance of this application is running already */
    hWindow = FindWindow(szConsoleWindow, szConsoleTitle);
    if (hWindow) {
        SetForegroundWindow(hWindow);
        return;
    }

    /* Initialize the application */
    if (!consoleInitInstance(hInstance)) {
        return;
    }
}

/* Creates the main window */
BOOL consoleInitInstance(HINSTANCE hInstance) {
    WNDCLASS wc;

    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
                    WS_BORDER | ES_LEFT | ES_MULTILINE | ES_NOHIDESEL |
                    ES_AUTOHSCROLL | ES_AUTOVSCROLL;

    memset(&wc, 0, sizeof(wc));

    /* Setup the window class */
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.hCursor       = 0;
    wc.hIcon         = 0;
    wc.hInstance     = hInstance;
    wc.lpfnWndProc   = (WNDPROC) consoleWndProc;
    wc.lpszClassName = szConsoleWindow;
    wc.lpszMenuName  = 0;
    wc.style         = CS_HREDRAW | CS_VREDRAW;

    /* Register the window class */
    RegisterClass(&wc);

    /* Create the window */
    hWindow = CreateWindowEx(WS_EX_CAPTIONOKBTN,
                        szConsoleWindow,     // Class
                        szConsoleTitle,      // Title
                        WS_VISIBLE,          // Style
                        CW_USEDEFAULT,       // x-position
                        CW_USEDEFAULT,       // y-position
                        CW_USEDEFAULT,       // x-size
                        CW_USEDEFAULT,       // y-size
                        NULL,                // Parent handle
                        NULL,                // Menu handle
                        hInstance,           // Instance handle
                        NULL);               // Ignored

    if (!hWindow) {
        return FALSE;
    }

    /* Create the Textedit window */
    hEdit = CreateWindow(TEXT("edit"),   // Class name
                         NULL,           // Window text
                         dwStyle,        // Window style
                         0,              // x-coordinate of the upper-left corner
                         0,              // y-coordinate of the upper-left corner
                         235,            // Width of the window for the edit
                         270,            // Height of the window for the edit
                         MainWindow(),   // Window handle to the parent window
                         0,              // Control identifier
                         AppInstance(),  // Instance handle
                         NULL);          // Specify NULL for this parameter when

    SendMessage(hEdit, EM_SETLIMITTEXT, 0x7fffffff, 0);
    ShowWindow(MainWindow(), SW_SHOWNORMAL);
    UpdateWindow(MainWindow());
    return TRUE;
}

LRESULT CALLBACK consoleWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    LRESULT lResult = TRUE;

    switch(msg){
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wp,lp)) {
        case IDOK:
          SendMessage(hwnd,WM_CLOSE,0,0);
          break;;
        default:
          return DefWindowProc(hwnd, msg, wp, lp);
        }
        ShowWindow( hEdit, SW_SHOW );
        UpdateWindow( hEdit );
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        lResult = DefWindowProc(hwnd, msg, wp, lp);
        break;
    }
    return lResult;
}

// s must be NULL terminated; flush line only at new line characters
void print_raw(const char* s) {
    // convert \n to \r\n
    MSG msg;

    BOOL flush_line = FALSE;
    TCHAR tmp[MAXBUFFER];
    int res;
    int lng;
    int si;

    // we add one or two characters per iteration
    for (si = 0; _line_pos < max_line-2; _line_pos++, si++) {
        const char ch = s[si];
        if (ch == '\n') {
            flush_line = TRUE;
            _line_buffer[_line_pos++] = '\r';
        }
        _line_buffer[_line_pos] = ch;
        if (ch == 0x0) {
            break;
        }
    }
    if (flush_line || _line_pos >= max_line-2) {
        _line_pos = 0;
        // convert to Unicode and display
        res = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, _line_buffer, -1, tmp, max_line);
        lng = GetWindowTextLength(EditWindow());
        SetFocus(EditWindow());
        SendMessage(EditWindow(), EM_SETSEL, (WPARAM)lng, (LPARAM)lng);
        SendMessage(EditWindow(), EM_REPLACESEL, 0, (LPARAM) ((LPSTR) tmp));
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) == TRUE) {
            TranslateMessage (&msg);
            DispatchMessage(&msg);
        }
    }
}

int closeConsole(void) {
    MSG msg;

    use_console = FALSE;

    while (GetMessage(&msg, NULL, 0,0) == TRUE) {
      TranslateMessage (&msg);
      DispatchMessage(&msg);
    }
    return (msg.wParam);
}
