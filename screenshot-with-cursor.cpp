#include <iostream>
#include <winuser.h>
#include <wingdi.h>
#include <gdiplus.h>

void ShowUsage();

void Screenshot();
void InitGdiplus();
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

char * arg0;

bool drawCursor = true;
wchar_t * outputFileName;

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
		
		} else if (requiredArguments > 0) {

			const char * outputChar = argv[i];
			const size_t outputCharSize = strlen(outputChar)+1;

			outputFileName = new wchar_t[outputCharSize];
			mbstowcs(outputFileName, outputChar, outputCharSize);

			requiredArguments--;
		} else {
			std::cout << "Extra argument '" << argv[i] << "' ignored" << std::endl;
		}

		i++;
	}

	if (requiredArguments != 0) {
		ShowUsage();
		return 0;
	}

	std::cout << "drawCursor: " << drawCursor << std::endl;
	std::cout << "outputFileName: ";
	std::wcout << outputFileName << std::endl;

	Screenshot();

	return 0;
}

// Displays help.
void ShowUsage() {
	std::cerr << "Usage: " << arg0 << " [-h/--help] [-nc/--nocursor] OUTPUT.PNG" << std::endl;
}

// Screenshots.
void Screenshot() {

	InitGdiplus();

	// Get size of screen
	int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	// std::cout << "w: " << width << ", h: " << height << "\n";

	// Get information about the cursor
	CURSORINFO cursorInfo;
	cursorInfo.cbSize = sizeof(CURSORINFO);
	GetCursorInfo(&cursorInfo);
	// std::cout << "cursorInfo.hCursor: " << cursorInfo.hCursor << "\n";
	// std::cout << "cursorInfo.ptScreenPos: " << cursorInfo.ptScreenPos.x << ", " << cursorInfo.ptScreenPos.y << "\n";

	// Get information about the cursor icon
	ICONINFO iconInfo;
	GetIconInfo(cursorInfo.hCursor, &iconInfo);
	// std::cout << "iconInfo.xHotspot: " << iconInfo.xHotspot << "\n";
	// std::cout << "iconInfo.yHotspot: " << iconInfo.yHotspot << "\n";

	// Gets screen image
	HDC hScreenDC = GetDC(NULL);

	// Creates temporary memory DC
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
	// std::cout << "hScreenDC: " << hScreenDC << ", hMemoryDC: " << hMemoryDC << "\n";

	// Creates a bitmap for the screen
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
	// std::cout << "hBitmap: " << hBitmap << "\n";

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
	Gdiplus::Bitmap newBitmap (hBitmap, NULL);

	// Saves bitmap to file
	newBitmap.Save(outputFileName, &encoderPNG, NULL);

}

// Starts all sorts of GDI+ related shit.
void InitGdiplus() {

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	GetEncoderClsid(L"image/png", &encoderPNG);

}

// Copied from the documentation!
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
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