#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

// TODO(nikita): This is a global for now.
global_variable bool GlobalRunning;

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

global_variable win32_offscreen_buffer GlobalBackbuffer;

internal win32_window_dimension GetWindowDimension(HWND window)
{
    win32_window_dimension result;
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.Width = clientRect.right - clientRect.left;
    result.Height = clientRect.bottom - clientRect.top;

    return result;
}

internal void RenderWeirdGradient(win32_offscreen_buffer buffer, int xOffset, int yOffset)
{
    // TODO(nikita): Let's see what the optimizer does
    uint8* row = (uint8*)buffer.Memory;
    for (int y = 0; y < buffer.Height; y++)
    {
        uint32* pixel = (uint32*)row;
        for (int x = 0; x < buffer.Width; x++)
        {
            uint8 Blue = (x + xOffset);
            uint8 Green = (y + yOffset);
            uint8 Red = 128;
            *pixel++ = (Blue | (Green << 8) | (Red << 16));
        }

        row += buffer.Pitch;
    }
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer* buffer, int width, int height)
{
    if (buffer->Memory)
    {
        VirtualFree(buffer->Memory, 0, MEM_RELEASE);
    }

    buffer->Width = width;
    buffer->Height = height;
    buffer->BytesPerPixel = 4;

    // NOTE(nikita): When the biHeight field is negative, this is the clue to   
    // Windows to treat this bitmap as top-down, not bottom-up, meaning that
    // the first three bytes of the image are the color for the top left pixel
    // in the bitmap, not the bottom left!
    buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth = buffer->Width;
    buffer->Info.bmiHeader.biHeight = -buffer->Height;
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = (buffer->Width* buffer->Height) * buffer->BytesPerPixel;

    buffer->Memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    buffer->Pitch = buffer->Width * buffer->BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(
    HDC deviceContext, int windowWidth, int windowHeight,
    win32_offscreen_buffer buffer,
    int x, int y, int width, int height)
{
    // TODO(nikita): Aspect ration correction
    StretchDIBits(
        deviceContext,
        0, 0, windowWidth, windowHeight,
        0, 0, buffer.Width, buffer.Height,
        buffer.Memory,
        &buffer.Info,
        DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
    {
       
    } break;

    case WM_CLOSE:
    {
        GlobalRunning = false;
    } break;

    case WM_DESTROY:
    {
        GlobalRunning = false;
    } break;
        
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(window, &ps);

        int x = ps.rcPaint.left;
        int y = ps.rcPaint.top;
        int width = ps.rcPaint.right - ps.rcPaint.left;
        int height = ps.rcPaint.bottom - ps.rcPaint.top;

        win32_window_dimension dimension = GetWindowDimension(window);
        Win32DisplayBufferInWindow(hdc, dimension.Width, dimension.Height, GlobalBackbuffer, x, y, width, height);
        EndPaint(window, &ps);
    }
    return 0;

    }
    return DefWindowProc(window, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prevInstance, PWSTR cmdArgs, int showCode)
{
    const wchar_t CLASS_NAME[] = L"Sample Window Class";
    WNDCLASS wc = { };

    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Win32MainWindowCallback;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window.

    HWND window = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Learn to Program Windows",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        instance,
        NULL
    );

    if (window == NULL)
        return 0;

    ShowWindow(window, showCode);

    GlobalRunning = true;
    int xOffset = 0;
    int yOffset = 0;
    MSG msg;

    while (GlobalRunning)
    {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                GlobalRunning = false;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        RenderWeirdGradient(GlobalBackbuffer, xOffset, yOffset);
        win32_window_dimension dimension = GetWindowDimension(window);

        HDC hdc = GetDC(window);
        Win32DisplayBufferInWindow(hdc, dimension.Width, dimension.Height, GlobalBackbuffer, 0, 0, dimension.Width, dimension.Height);
        ReleaseDC(window, hdc);

        xOffset++;
        yOffset += 2;
    }

    return 0;
}
