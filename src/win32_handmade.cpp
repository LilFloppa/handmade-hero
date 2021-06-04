#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

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

typedef int32 bool32;


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

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable x_input_get_state* XInputGetState_;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable x_input_set_state* XInputSetState_;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

// TODO(nikita): This is a global for now.
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

internal void Win32InitDSound(HWND window, int32 samplesPerSecond, int32 bufferSize)
{
	// NOTE(nikita): Load the library
	HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");

	if (dSoundLibrary)
	{
		direct_sound_create* directSoundCreate = (direct_sound_create*)GetProcAddress(dSoundLibrary, "DirectSoundCreate");

		// NOTE(nikita): Get a DirectSound object
		LPDIRECTSOUND directSound;
		if (directSoundCreate && SUCCEEDED(directSoundCreate(0, &directSound, 0)))
		{
			WAVEFORMATEX waveFormat = {};
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2;
			waveFormat.nSamplesPerSec = samplesPerSecond;
			waveFormat.wBitsPerSample = 16;
			waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
			waveFormat.cbSize = 0;

			if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC bufferDescription = {};
				bufferDescription.dwSize = sizeof(bufferDescription);
				bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				// NOTE(nikita): Create a primary buffer
				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
				{
					if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
					{
						// NOTE(nikita): We have finally set the format!
						OutputDebugStringA("Initalized primary buffer");
					}
					else
					{
					}
				}
				else
				{
				}
			}
			else
			{
			}

			// NOTE(nikita): Create a secondary buffer
			DSBUFFERDESC bufferDescription = {};
			bufferDescription.dwSize = sizeof(bufferDescription); 
			bufferDescription.dwFlags = 0;
			bufferDescription.dwBufferBytes = bufferSize;
			bufferDescription.lpwfxFormat = &waveFormat;

			if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &GlobalSecondaryBuffer, 0)))
			{
				if (SUCCEEDED(GlobalSecondaryBuffer->SetFormat(&waveFormat)))
				{
					// NOTE(nikita): We have finally set the format!
				}
				else
				{
				}
			}
			else
			{
			}
		}
		else
		{
		}
	}
	else
	{
	}
}

internal void Win32LoadXInput()
{
	HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (xInputLibrary)
	{
		XInputGetState = (x_input_get_state*)GetProcAddress(xInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(xInputLibrary, "XInputSetState");
	}
}

internal win32_window_dimension Win32GetWindowDimension(HWND window)
{
	win32_window_dimension result;
	RECT clientRect;
	GetClientRect(window, &clientRect);
	result.Width = clientRect.right - clientRect.left;
	result.Height = clientRect.bottom - clientRect.top;

	return result;
}

internal void RenderWeirdGradient(win32_offscreen_buffer* buffer, int xOffset, int yOffset)
{
	// TODO(nikita): Let's see what the optimizer does
	uint8* row = (uint8*)buffer->Memory;
	for (int y = 0; y < buffer->Height; y++)
	{
		uint32* pixel = (uint32*)row;
		for (int x = 0; x < buffer->Width; x++)
		{
			uint8 Blue = (x + xOffset);
			uint8 Green = (y + yOffset);
			uint8 Red = 128;
			*pixel++ = (Blue | (Green << 8) | (Red << 16));
		}

		row += buffer->Pitch;
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

	int bitmapMemorySize = (buffer->Width * buffer->Height) * buffer->BytesPerPixel;

	buffer->Memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	buffer->Pitch = buffer->Width * buffer->BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(
	HDC deviceContext, int windowWidth, int windowHeight,
	win32_offscreen_buffer* buffer, int x, int y, int width, int height)
{
	// TODO(nikita): Aspect ration correction
	StretchDIBits(
		deviceContext,
		0, 0, windowWidth, windowHeight,
		0, 0, buffer->Width, buffer->Height,
		buffer->Memory,
		&buffer->Info,
		DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{

	case WM_CLOSE:
	{
		GlobalRunning = false;
	} break;

	case WM_DESTROY:
	{
		GlobalRunning = false;
	} break;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		uint32 vkCode = wParam;
		bool32 wasDown = ((lParam & (1 << 30)) != 0);
		bool32 isDown = ((lParam & (1 << 31)) == 0);

		if (vkCode == 'W')
		{
		}
		else if (vkCode == 'A')
		{
		}
		else if (vkCode == 'S')
		{
		}
		else if (vkCode == 'D')
		{
		}

		bool32 altKeyWasDown = lParam & (1 << 29);
		if ((vkCode == VK_F4) && altKeyWasDown)
		{
			GlobalRunning = false;
		}

	} break;


	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(window, &ps);

		int x = ps.rcPaint.left;
		int y = ps.rcPaint.top;
		int width = ps.rcPaint.right - ps.rcPaint.left;
		int height = ps.rcPaint.bottom - ps.rcPaint.top;

		win32_window_dimension dimension = Win32GetWindowDimension(window);
		Win32DisplayBufferInWindow(hdc, dimension.Width, dimension.Height, &GlobalBackbuffer, x, y, width, height);
		EndPaint(window, &ps);

		return 0;
	}

	}
	return DefWindowProc(window, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prevInstance, PWSTR cmdArgs, int showCode)
{
	Win32LoadXInput();
	const wchar_t CLASS_NAME[] = L"Sample Window Class";
	WNDCLASS wc = { };

	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
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

	int toneVolume = 16000;
	int samplesPerSecond = 48000;
	int bytesPerSample = 2 * sizeof(int16);
	int hz = 256;
	uint32 runningSampleIndex = 0;
	int squareWavePeriod = samplesPerSecond / hz;
	int halfSquareWavePeriod = squareWavePeriod / 2;
	int bufferSize = samplesPerSecond * bytesPerSample;

	Win32InitDSound(window, samplesPerSecond, bufferSize);

	GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

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

		//TODO(nikita): Should we poll this more frequently
		for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++)
		{
			XINPUT_STATE controllerState;
			if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
			{
				// NOTE(nikita): This controller is pulgged in
				// TODO(nikita): See if controllerState.dwPacketNumber increments too rapidly
				XINPUT_GAMEPAD* pad = &controllerState.Gamepad;

				bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
				bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
				bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
				bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
				bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
				bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
				bool leftShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
				bool rightShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
				bool AButton = (pad->wButtons & XINPUT_GAMEPAD_A);
				bool BButton = (pad->wButtons & XINPUT_GAMEPAD_B);
				bool XButton = (pad->wButtons & XINPUT_GAMEPAD_X);
				bool YButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

				int16 stickX = pad->sThumbLX;
				int16 stickY = pad->sThumbLY;
			}
			else
			{
				// NOTE(nikita): This controller is not available
			}
		}

		// NOTE(nikita): DirectSound output test

		DWORD playCursor;
		DWORD writeCursor;

		GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor);
		DWORD byteToLock = runningSampleIndex * bytesPerSample % bufferSize;
		DWORD bytesToWrite;
		if (byteToLock > playCursor)
		{
			bytesToWrite = bufferSize - byteToLock;
			bytesToWrite += playCursor;
		}
		else
		{
			bytesToWrite = playCursor - byteToLock;
		}


		VOID* region1;
		DWORD region1Size;
		VOID* region2;
		DWORD region2Size;

		GlobalSecondaryBuffer->Lock(byteToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0);

		int16* sampleOut = (int16*)region1;
		DWORD region1SampleCount = region1Size / bytesPerSample;
		for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
		{
			int16 sampleValue = ((runningSampleIndex / halfSquareWavePeriod) % 2)? toneVolume : -toneVolume;
			*sampleOut++ = sampleValue;
			*sampleOut++ = sampleValue;
			runningSampleIndex++;
		}

		sampleOut = (int16*)region2;
		DWORD region2SampleCount = region2Size / bytesPerSample;
		for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
		{
			int16 sampleValue = ((runningSampleIndex / halfSquareWavePeriod) % 2) ? toneVolume : -toneVolume;
			*sampleOut++ = sampleValue;
			*sampleOut++ = sampleValue;
			runningSampleIndex++;
		}

		GlobalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);

		RenderWeirdGradient(&GlobalBackbuffer, xOffset, yOffset);
		win32_window_dimension dimension = Win32GetWindowDimension(window);

		HDC hdc = GetDC(window);
		Win32DisplayBufferInWindow(hdc, dimension.Width, dimension.Height, &GlobalBackbuffer, 0, 0, dimension.Width, dimension.Height);
		ReleaseDC(window, hdc);

		xOffset++;
		yOffset += 2;
	}

	return 0;
}
