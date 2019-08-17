#include <stdio.h>
#include <stdlib.h>

//#include <windows.h> // needed?
#include <winuser.h>
#include <wingdi.h>
#include <gdiplus.h>

// Arguments
char * arg0;
bool drawCursor = true;
char * logFileName;
wchar_t * outputFileName;

void ParseArguments(int argc, char * argv[]);
void ShowUsage(FILE* where);

//
FILE * logFile = NULL;

void ShowEncoderList();
void Screenshot(BITMAPINFO * outBitmapInfo, BYTE ** outBitmapData);
void ImageToPNG(BITMAPINFO * bitmapInfo, BYTE ** bitmapData);

//
wchar_t * CharToWchar_t(char * charPointer);
void InitGdiplus();
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
CLSID encoderPNG;

int main(int argc, char * argv[]) {

	ParseArguments(argc, argv);

	InitGdiplus();

	ShowEncoderList();

	BITMAPINFO screenBitmapInfo;
	BYTE * screenBitmapData;

	Screenshot(&screenBitmapInfo, &screenBitmapData);
	ImageToPNG(&screenBitmapInfo, &screenBitmapData);

	fprintf(logFile, "\n");

	return 0;
}

void ParseArguments(int argc, char * argv[]) {

	arg0 = argv[0];

	int requiredArguments = 1;

	int i = 1;
	while (i<argc) {

		if (strcmp(argv[i], "--help")==0 || strcmp(argv[i], "-h")==0) {
			ShowUsage(stdout);
			exit(0);

		} else if (strcmp(argv[i], "--nocursor")==0 || strcmp(argv[i], "-nc")==0) {
			drawCursor = false;
		
		} else if (strcmp(argv[i], "--log")==0 || strcmp(argv[i], "-l")==0) {
			i++;

			if (i<argc) {
				logFileName = argv[i];
				logFile = fopen(logFileName, "a");

				if (logFile == NULL) {
					fprintf(stderr, "Failed to open log file!\n");
				}
			} else {
				fprintf(stderr, "No log file provided!\n");
			}

		} else if (requiredArguments>0) {

			outputFileName = CharToWchar_t(argv[i]);

			requiredArguments--;

		} else {
			fprintf(logFile, "Extra argument '%s' ignored\n", argv[i]);
		}

		i++;
	}

	if (requiredArguments!=0) {
		ShowUsage(stderr);
		exit(1);
	}

	fprintf(logFile, "- Parsed arguments:\n");
	fprintf(logFile, "arg0: %s\n", arg0);
	fprintf(logFile, "drawCursor: %d\n", drawCursor);
	fprintf(logFile, "logFileName: %s\n", logFileName);
	fwprintf(logFile, L"outputFileName: %s\n", outputFileName);
}

// Displays help.
void ShowUsage(FILE* where) {
	fprintf(where, "Usage: %s [-h/--help] [-l/--log [LOGFILE]] [-nc/--nocursor] OUTPUTFILE.PNG\n", arg0);
}

// Shows list of avaliable encoders.
void ShowEncoderList() {

	using namespace Gdiplus;

	fprintf(logFile, "- List of encoders:\n");

	UINT num = 0; // number of image encoders
	UINT size = 0; // size of the image encoder array in bytes

	GetImageEncodersSize(&num, &size);
	if (size == 0) {
		fprintf(logFile, "No encoders!\n");
	}

	ImageCodecInfo * imageCodecInfoArray = new ImageCodecInfo[size];

	Gdiplus::Status status = GetImageEncoders(num, size, imageCodecInfoArray);
	fprintf(logFile, "Gdiplus::GetImageEncoders returns: %d\n", status);
	if ( status != Gdiplus::Ok) {
		fprintf(logFile, "Failed to get image encoders!\n");
	}

	for (UINT i = 0; i < num; ++i) {
		fprintf(logFile, "[#%d]\n", i);
		fwprintf(logFile, L"Clsid: %p\n", imageCodecInfoArray[i].Clsid);
		fwprintf(logFile, L"CodecName: %s\n", imageCodecInfoArray[i].CodecName);
		fwprintf(logFile, L"FormatDescription: %s\n", imageCodecInfoArray[i].FormatDescription);
		fwprintf(logFile, L"FilenameExtension: %s\n", imageCodecInfoArray[i].FilenameExtension);
		fwprintf(logFile, L"MimeType: %s\n", imageCodecInfoArray[i].MimeType);
	}

}

// Takes screenshot and draws mouse cursor
void Screenshot(BITMAPINFO * outBitmapInfo, BYTE ** outBitmapData) {

	fprintf(logFile, "- Taking a screenshot.\n");

	// Gets screen image
	HDC hScreenDC = GetDC(NULL);

	// Creates temporary memory DC
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

	// Get size of screen
	int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	fprintf(logFile, "Size of screen: %dx%d\n", width, height);

	// Creates a bitmap for the screen
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);

	// Puts the bitmap in the memory, or something like that
	SelectObject(hMemoryDC, hBitmap);

	// Copies from the screen to the memory (consequently, the bitmap)
	BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

	// Get information about the cursor
	CURSORINFO cursorInfo;
	cursorInfo.cbSize = sizeof(CURSORINFO);
	GetCursorInfo(&cursorInfo);

	fprintf(logFile, "Cursor position: (%ld, %ld)\n", cursorInfo.ptScreenPos.x, cursorInfo.ptScreenPos.y);

	// Get information about the cursor icon
	ICONINFO iconInfo;
	GetIconInfo(cursorInfo.hCursor, &iconInfo);

	fprintf(logFile, "Cursor hotspot: (%ld, %ld)\n", iconInfo.xHotspot, iconInfo.yHotspot);

	// Figure actual position to draw
	long iconPosX = cursorInfo.ptScreenPos.x - iconInfo.xHotspot;
	long iconPosY = cursorInfo.ptScreenPos.y - iconInfo.yHotspot;

	fprintf(logFile, "Proper icon position: (%ld, %ld)\n", iconPosX, iconPosY);

	if (drawCursor) {
		// Draws the cursor icon at the right proper position
		DrawIcon(hMemoryDC, iconPosX, iconPosY, cursorInfo.hCursor);
	}

	// Getting bits from hmemorydc and hbitmap

	// Make a bitmap info object
	BITMAPINFO bitmapInfo;
	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biHeight = -height;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 24;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

	size_t pixelSize = bitmapInfo.bmiHeader.biBitCount / 8;
	size_t scanlineSize = (pixelSize * bitmapInfo.bmiHeader.biWidth + 3) & ~3; // magic
	size_t bitmapSize = -(bitmapInfo.bmiHeader.biHeight) * scanlineSize;

	fprintf(logFile, "Bytes per pixel: %d\n", pixelSize);
	fprintf(logFile, "Bytes per line: %d\n", scanlineSize);
	fprintf(logFile, "Total bytes: %d\n", bitmapSize);

	// Allocate bitmap data
	BYTE * bitmapData = new BYTE[bitmapSize];

	GetDIBits(hMemoryDC, hBitmap, 0, height, bitmapData, &bitmapInfo, DIB_RGB_COLORS);

	// Output stuff
	*outBitmapInfo = bitmapInfo;
	*outBitmapData = bitmapData;
	
}

// Converts the image to a PNG.
void ImageToPNG(BITMAPINFO * bitmapInfo, BYTE ** bitmapData) {

	fprintf(logFile, "- Saving image to file.\n");

	// Make gdiplus object and save it
	Gdiplus::Bitmap * gdiBitmap = Gdiplus::Bitmap::FromBITMAPINFO(bitmapInfo, *bitmapData);

	int result = GetEncoderClsid(L"image/png", &encoderPNG);

	fprintf(logFile, "GetEncoderClsid returns: %d\n", result);
	if (result == -1) {
		fprintf(logFile, "Failed to get encoder CLSID!\n");
	}

	Gdiplus::Status status = gdiBitmap -> Save(outputFileName, &encoderPNG, NULL);

	fprintf(logFile, "Gdiplus::Save returns: %d\n", status);
	if ( status != Gdiplus::Ok) {
		fprintf(logFile, "Failed to save file!\n");
	}

}

// Converts a char* into a wchar_t*.
wchar_t * CharToWchar_t(char * charPointer) {
	size_t charSize = mbstowcs(NULL, charPointer, 0); 
	wchar_t * charWchar_t = (wchar_t*) malloc((charSize + 1)*sizeof(wchar_t));
	mbstowcs(charWchar_t, charPointer, charSize + 1);

	return charWchar_t;
}

// Starts all sorts of GDI+ related shit.
void InitGdiplus() {

	fprintf(logFile, "- Initilizing GDI+.\n");

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;

	Gdiplus::Status status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	fprintf(logFile, "Gdiplus::GdiplusStartup returns: %d\n", status);

	if ( status != Gdiplus::Ok) {
		fprintf(logFile, "Failed to initiaze Gdiplus!\n");
	}

}

// Copied from https://docs.microsoft.com/en-us/windows/win32/gdiplus/-gdiplus-retrieving-the-class-identifier-for-an-encoder-use
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

   Gdiplus::GetImageEncodersSize(&num, &size);
   if(size == 0)
      return 1;  // Failure

   pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return 1;  // Failure

   Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return 0;  // Success
      }    
   }

   free(pImageCodecInfo);
   return 1;  // Failure
}