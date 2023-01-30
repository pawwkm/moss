// References:
// http://www.winprog.org/tutorial/start.html
// http://www.catch22.net/tuts/win32/flicker-free-drawing

#include "../moss.h"

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

static Editor editor;
static HDC hdc;
static HDC font_hdc;
static HBITMAP font_sheet;
static POINT minimum_size;

static HWND hwnd;

static double frequency;
double time_in_us(void)
{
    LARGE_INTEGER captureTime;
    QueryPerformanceCounter(&captureTime);
    return (double)captureTime.QuadPart / frequency;
}

RECT block_to_rect(Block b)
{
    return (RECT)
    {
        .left = b.x,
        .top = b.y,
        .right = b.x + b.width,
        .bottom = b.y + b.height
    };
}

void scroll_region(Region region)
{
    RECT r = block_to_rect(region.block);
    ScrollDC
    (
        hdc, 
        region.scroll_x,
        region.scroll_y,
        &r,
        &r,
        NULL,
        &(RECT){ 0 }
    );
}

void render_block(Color color, Block block)
{
    HBRUSH brush = CreateSolidBrush(RGB(color.r, color.g, color.b));
    RECT r = block_to_rect(block);
        
    FillRect(hdc, &r, brush);

    DeleteObject(brush);
}

void render_character(uint16_t source_x, uint16_t source_y, uint16_t destination_x, uint16_t destination_y, uint16_t width, uint16_t height)
{
    BitBlt(hdc, destination_x, destination_y, width, height, font_hdc, source_x, source_y, SRCCOPY);
}

void set_editor_title(char* path, size_t path_length)
{
    static char title[MAX_PATH + 8] = "Moss - ";
    memcpy(&title[7], path, path_length);
    title[7 + path_length] = '\0';

    SetWindowText(hwnd, title);
}

bool allocate_font_sheet(uint16_t width, uint16_t height)
{
    font_sheet = CreateCompatibleBitmap(hdc, width, height);
    return font_sheet && SelectObject(font_hdc, font_sheet);
}

bool set_font_sheet_pixel(Color color, uint16_t x, uint16_t y)
{
    return SetPixel(font_hdc, x, y, RGB(color.r, color.g, color.b)) != -1;
}

char* read_file(char* const path, size_t* characters_length)
{
    FILE* file = fopen(path, "rb");
    if (!file)
    {
        // log("Could not open %s: %s\n", path, strerror(errno));
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) == -1)
    {
        // log("Could not seek in %s\n", path);
        return NULL;
    }

    *characters_length = ftell(file);
    if (fseek(file, 0, SEEK_SET) == -1)
    {
        // log("Could not seek in %s\n", path);
        return NULL;
    }

    char* characters = malloc(*characters_length);
    if (!characters)
    {
        // log("Could not allocate characters for %s\n", path);
        return NULL;
    }

    size_t characters_read = fread(characters, 1, *characters_length, file);
    if (characters_read != *characters_length)
    {
        // log("Read %u characters out of %u from %s\n", characters_read, *characters_length, path, strerror(errno));
        free(characters);

        if (fclose(file) < 0)
            ;// log("Could not close %s: %s\n", path, strerror(errno));

        return NULL;
    }

    if (fclose(file) < 0)
        ;// log("Could not close %s: %s\n", path, strerror(errno));

    return characters;
}

static bool close;
LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static LPARAM last_lParam;
    if (msg == WM_MOUSEMOVE || msg == WM_NCMOUSEMOVE)
    {
        if (lParam == last_lParam)
            return DefWindowProc(hwnd, msg, wParam, lParam);

        last_lParam = lParam;
    }

    hwnd = h;
    static bool ctrl;
    static bool mouse_visible = true;
    switch(msg)
    {
        case WM_SYSKEYUP:
        case WM_SYSCHAR:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_NCMOUSEMOVE:
        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONUP:
        case WM_NCMBUTTONDOWN:
        case WM_NCMBUTTONUP:
        case WM_NCRBUTTONDOWN:
        case WM_NCRBUTTONUP:
        case WM_KILLFOCUS:
            if (!mouse_visible)
                ShowCursor(mouse_visible = true);

            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
                resize_editor(&editor, (uint16_t)(LOWORD(lParam)), (uint16_t)(HIWORD(lParam)));
                
            break;

        case WM_GETMINMAXINFO:
            ((MINMAXINFO*)lParam)->ptMinTrackSize = minimum_size;
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
            if (LOWORD(wParam) == VK_CONTROL)
                ctrl = !((HIWORD(lParam) & KF_UP) == KF_UP);

            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_CHAR:
            if (mouse_visible)
                ShowCursor(mouse_visible = false);

            interpret_character(&editor, (char)wParam, ctrl);
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
            PAINTSTRUCT ps;
            hdc = BeginPaint(hwnd, &ps);
            render_editor(&editor);
            EndPaint(hwnd, &ps);

            break;

        case WM_CLOSE:
            // TODO: Check that all files are saved before closing.
            close = true;

            DestroyWindow(hwnd);
            break;
        
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    if (editor.invalidated_length)
        InvalidateRect(hwnd, NULL, false);
    
    return 0;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, PSTR cmd, int show_cmd)
{
    (void)previous;
    (void)cmd;

    WNDCLASSEX wc =
    {
        .cbSize = sizeof(wc),
        .style = 0,
        .lpfnWndProc = WndProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = instance,
        .hIcon = LoadIcon(NULL, IDI_APPLICATION),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = NULL,
        .lpszMenuName = NULL,
        .lpszClassName = "MossClassName",
        .hIconSm = LoadIcon(NULL, IDI_APPLICATION)
    };

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    editor = initialize_editor();
    
    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT window_rect = { 0, 0, editor.width, editor.height };
    AdjustWindowRect(&window_rect, style, FALSE);

    minimum_size.x = window_rect.right - window_rect.left;
    minimum_size.y = window_rect.bottom - window_rect.top;

    hwnd = CreateWindowEx
    (
        0,
        wc.lpszClassName,
        "Moss",
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        minimum_size.x,
        minimum_size.y,
        NULL,
        NULL,
        instance,
        NULL
    );

    if (!hwnd)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hdc = GetDC(hwnd);
    if (!hdc)
    {
        MessageBox(NULL, "Could not get window DC!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    font_hdc = CreateCompatibleDC(hdc);
    if (!font_hdc)
    {
        MessageBox(NULL, "Could not font sheet DC!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    if (!initialize_font(&cozette))
    {
        MessageBox(NULL, "Could not initialize font!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ReleaseDC(hwnd, hdc);

    LARGE_INTEGER captureFreq;
    QueryPerformanceFrequency(&captureFreq);
    frequency = (double)captureFreq.QuadPart / 1000000.0;

    for (size_t i = 1; i < __argc; i++)
    {
        // TODO: Have a function to open an array of files
        // so that I don't render the entire screen
        // for every file. Only the last render is seen.
        size_t path_length = strlen(__argv[i]);
        if (path_length > MAX_PATH)
        {
            //// log(&editor, "Path to long");
            continue;
        }
        
        char* path = malloc(path_length + 1);

        strcpy(path, __argv[i]);
        open_buffer_in_active_tab(&editor, path, (uint16_t)path_length);
    }

    ShowWindow(hwnd, show_cmd);

    MSG message;
    while (!close)
    {
        BOOL got_message = GetMessage(&message, hwnd, 0, 0);
        if (got_message == -1)
        {
            // TODO: Handle error.
        }
        else
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }
    
    return (int)message.wParam;
}
