#include <stdio.h>
#include <stdlib.h>
#include <winuser.h>
#include <wingdi.h>
#include <gdiplus.h>

void ShowUsage(FILE* where);
void ShowUsage();

void Screenshot();
void InitGdiplus();
int GetEncoderClsid(const wchar_t* format, CLSID* pClsid);

char * arg0;

bool drawCursor = true;
wchar_t * outputFileName;

bool verbose = false;

CLSID encoderPNG;

int main(int argc, char * argv[]) {

	arg0 = argv[0];

	int requiredArguments = 1;

	int i = 1;
	while(i<argc) {

		if (strcmp(argv[i], "--help")==0 || strcmp(argv[i], "-h")==0) {
			ShowUsage();
			return 0;

		} else if (strcmp(argv[i], "--nocursor")==0 || strcmp(argv[i], "-nc")==0) {
			drawCursor = false;
		
		} else if (strcmp(argv[i], "--verbose")==0 || strcmp(argv[i], "-v")==0) {
			verbose = true;

		} else if (requiredArguments > 0) {

			const char * outputChar = argv[i];
			const size_t outputCharSize = strlen(outputChar)+1;

			outputFileName = new wchar_t[outputCharSize];
			mbstowcs(outputFileName, outputChar, outputCharSize);

			requiredArguments--;
		} else {
			if (verbose) printf("Extra argument '%s' ignored\n", argv[i]);
		}

		i++;
	}

	if (requiredArguments != 0) {
		ShowUsage(stderr);
		return 1;
	}

	if (verbose) {
		printf("drawCursor: %d\n", drawCursor);
		printf("outputFileName: ");
		wprintf(outputFileName);
		printf("\n");
	}

	Screenshot();

	return 0;
}

// Displays help.
void ShowUsage(FILE* where) {
	fprintf(where, "Usage: %s [-h/--help] [-v/--verbose] [-nc/--nocursor] OUTPUT.PNG\n", arg0);
}

void ShowUsage() {
	ShowUsage(stdin);
}

// Screenshots.
void Screenshot() {

	if (verbose) printf("Taking screenshot...\n");

	InitGdiplus();

	if (verbose) printf("encoderPNG: %p\n", encoderPNG);

	// Get size of screen
	int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	if (verbose) printf("w: %d, h: %d\n", width, height);

	// Get information about the cursor
	CURSORINFO cursorInfo;
	cursorInfo.cbSize = sizeof(CURSORINFO);
	GetCursorInfo(&cursorInfo);

	if (verbose) {
		printf("cursorInfo.hCursor: %p\n", cursorInfo.hCursor);
		printf("cursorInfo.ptScreenPos: x: %d, y: %d\n", cursorInfo.ptScreenPos.x, cursorInfo.ptScreenPos.y);
	}

	// Get information about the cursor icon
	ICONINFO iconInfo;
	GetIconInfo(cursorInfo.hCursor, &iconInfo);

	if (verbose) {
		printf("iconInfo.xHotspot: %d\n", iconInfo.xHotspot);
		printf("iconInfo.yHotspot: %d\n", iconInfo.yHotspot);
	}

	// Gets screen image
	HDC hScreenDC = GetDC(NULL);

	// Creates temporary memory DC
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

	if (verbose) printf("hScreenDC: %p, hMemoryDC: %p\n", hScreenDC, hMemoryDC);

	// Creates a bitmap for the screen
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);

	if (verbose) printf("hBitmap: %p\n", hBitmap);

	// Puts the bitmap in the memory??
	SelectObject(hMemoryDC, hBitmap);

	// Copies from the screen to the memory (consequently, the bitmap)
	BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

	if (drawCursor) {
		// Draws the cursor icon at the proper position
		DrawIcon(hMemoryDC,
			cursorInfo.ptScreenPos.x - iconInfo.xHotspot,
			cursorInfo.ptScreenPos.y - iconInfo.yHotspot,
			cursorInfo.hCursor);
	}

	// Puts bitmap inside gdiplus bitmap
	Gdiplus::Bitmap gdiBitmap (hBitmap, NULL);

	if (verbose) printf("gdiBitmap: %p\n", &gdiBitmap);

	// Saves bitmap to file
	gdiBitmap.Save(outputFileName, &encoderPNG, NULL);

}

// Starts all sorts of GDI+ related shit.
void InitGdiplus() {

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	GetEncoderClsid(L"image/png", &encoderPNG);

}

// Copied from the documentation!
int GetEncoderClsid(const wchar_t* format, CLSID* pClsid) {
	using namespace Gdiplus;

	UINT num = 0; // number of image encoders
	UINT size = 0; // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0) return -1;

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL) return -1;

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j) {
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}

	free(pImageCodecInfo);
	return -1;
}