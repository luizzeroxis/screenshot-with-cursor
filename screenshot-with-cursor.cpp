#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <winuser.h>
#include <wingdi.h>
#include <gdiplus.h>

// Arguments
char * arg0;

bool saveToFile = false;
bool saveToClipboard = false;
const wchar_t * encoderMime;
bool drawCursor = true;
char * logFileName;
wchar_t * outputFileName;

void ParseArguments(int argc, char * argv[]);
void ShowUsage(FILE* where);
wchar_t * GetExtensionMIMEFormat(wchar_t * fileName);

//
FILE * logFile;

Gdiplus::ImageCodecInfo * GetEncoderList(UINT * num);
void ShowEncoderList();

void Screenshot(HDC * outHDC, HBITMAP * outHBitmap);
void GetHDCAndHBitmapBitmapInfoAndData(HDC * hdc, HBITMAP * hBitmap, BITMAPINFO * bitmapInfo, BYTE ** bitmapData);
void ImageToFile(BITMAPINFO * bitmapInfo, BYTE ** bitmapData);
void ImageToClipboard(HBITMAP * outHBitmap);

//
wchar_t * CharToWchar_t(char * charPointer);
void InitGdiplus();
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
CLSID encoderCLSID;

int main(int argc, char * argv[]) {

	logFile = fopen("NUL", "a");

	ParseArguments(argc, argv);

	InitGdiplus();

	HDC screenHDC;
	HBITMAP screenHBitmap;
	BITMAPINFO screenBitmapInfo;
	BYTE * screenBitmapData;

	Screenshot(&screenHDC, &screenHBitmap);
	GetHDCAndHBitmapBitmapInfoAndData(&screenHDC, &screenHBitmap, &screenBitmapInfo, &screenBitmapData);

	DeleteDC(screenHDC);

	if (saveToFile) {
		ImageToFile(&screenBitmapInfo, &screenBitmapData);
	}

	if (saveToClipboard) {
		ImageToClipboard(&screenHBitmap);
	}

	DeleteObject(screenHBitmap);

	fprintf(logFile, "Exiting\n");

	return 0;
}

void ParseArguments(int argc, char * argv[]) {

	arg0 = argv[0];

	int requiredArguments = 1;

	int i = 1;
	while (i<argc) {

		// help

		if (strcmp(argv[i], "--help")==0 || strcmp(argv[i], "-h")==0) {
			ShowUsage(stdout);

		} else if (strcmp(argv[i], "--list-file-formats")==0) {
			InitGdiplus();
			ShowEncoderList();

		// output

		} else if (strcmp(argv[i], "--clipboard")==0 || strcmp(argv[i], "-c")==0) {

			saveToClipboard = true;

		// options

		} else if (strcmp(argv[i], "--file-format")==0 || strcmp(argv[i], "-f")==0) {

			i++;
			if (i<argc) {
				encoderMime = CharToWchar_t(argv[i]);
			} else {
				fprintf(logFile, "--file-format requires 1 more argument\n");
			}

		} else if (strcmp(argv[i], "--no-cursor")==0) {
			drawCursor = false;
		
		} else if (strcmp(argv[i], "--log")==0 || strcmp(argv[i], "-l")==0) {

			i++;
			if (i<argc) {
				logFileName = argv[i];

				fclose(logFile);

				if (strcmp(logFileName, "-")==0) {
					logFile = stdout;
				} else {
					logFile = fopen(logFileName, "a");
				}

				if (logFile == NULL) {
					fprintf(stderr, "Failed to open log file!\n");
				}
			} else {
				fprintf(stderr, "--log requires 1 more argument\n");
			}

		// default

		} else if (requiredArguments>0) {

			saveToFile = true;
			outputFileName = CharToWchar_t(argv[i]);
			requiredArguments--;

		} else {
			fprintf(logFile, "Unknown argument '%s'\n", argv[i]);
		}

		i++;
	}

	// required things
	if (requiredArguments != 0) {
		if (!saveToClipboard) {
			ShowUsage(stderr);
			exit(1);
		}
	}

	fprintf(logFile, "- Parsed arguments:\n");
	fprintf(logFile, "arg0: %s\n", arg0);
	fprintf(logFile, "saveToClipboard: %d\n", saveToClipboard);
	fwprintf(logFile, L"encoderMime: %s\n", encoderMime);
	fprintf(logFile, "drawCursor: %d\n", drawCursor);
	fprintf(logFile, "logFileName: %s\n", logFileName);
	fprintf(logFile, "saveToFile: %d\n", saveToFile);
	fwprintf(logFile, L"outputFileName: %s\n", outputFileName);
}

// Displays help.
void ShowUsage(FILE* where) {
	
	fprintf(where, "Takes a screenshot with the cursor icon.\n"
		"\n"
		"Usage: %s [-h] [--clipboard] <outputfile>\n"
		"\n"
		"Options:\n"
		"  -h --help            Shows this screen.\n"
		"  --list-file-formats  Shows a list of avaliable file formats.\n"
		"\n"
		"  --clipboard          Copies the screenshot to the clipboard.\n"
		"  --file-format        Sets the format of the output file.\n"
		"  --no-cursor          Do not draw cursor icon, making this pointless.\n"
		"\n"
		"  --log <filename>     Outputs information to a file. Use '-' to use the standard output.\n", arg0);

}

// Get mime format of extension
wchar_t * GetExtensionMIMEFormat(wchar_t * fileName) {

	wchar_t * fileNameCopy = new wchar_t[wcslen(fileName)];
	wcscpy(fileNameCopy, fileName); // must copy because upr is in place

	wchar_t * extension = wcsrchr(fileNameCopy, L'.');
	if (extension == NULL) {
		return NULL;
	}

	_wcsupr(extension);

	UINT num;
	Gdiplus::ImageCodecInfo * encoders = GetEncoderList(&num);

	for (UINT i = 0; i < num; ++i) {
		wchar_t * token = wcstok(encoders[i].FilenameExtension, L";");
		while(token != NULL) {
			if (wcscmp(token+1, extension)==0) {
				return encoders[i].MimeType;
			}
			token = wcstok(NULL, L";");
		}
	}

	return NULL;
}

// Gets list of avaliable encoders
Gdiplus::ImageCodecInfo * GetEncoderList(UINT * num) {

	UINT size = 0; // size of the image encoder array in bytes

	Gdiplus::GetImageEncodersSize(num, &size);
	if (size == 0) {
		fprintf(logFile, "No encoders!\n");
		exit(1);
	}

	Gdiplus::ImageCodecInfo * imageCodecInfoArray = new Gdiplus::ImageCodecInfo[size];

	Gdiplus::Status status = Gdiplus::GetImageEncoders(*num, size, imageCodecInfoArray);
	if ( status != Gdiplus::Ok) {
		fprintf(logFile, "Gdiplus::GetImageEncoders returns: %d\n", status);
		fprintf(logFile, "Failed to get image encoders!\n");
		exit(1);
	}

	return imageCodecInfoArray;

}

// Shows list of encoders
void ShowEncoderList() {

	UINT num;
	Gdiplus::ImageCodecInfo * encoders = GetEncoderList(&num);

	for (UINT i = 0; i < num; ++i) {
		wprintf(L"%s\n", encoders[i].MimeType);
	}

}

void Screenshot(HDC * hMemoryDC, HBITMAP * hBitmap) {

	fprintf(logFile, "- Taking a screenshot.\n");

	// Gets screen image
	HDC hScreenDC = GetDC(NULL);

	// Creates temporary memory DC
	*hMemoryDC = CreateCompatibleDC(hScreenDC);

	// Get size of screen
	int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	fprintf(logFile, "Size of screen: %dx%d\n", width, height);

	// Creates a bitmap for the screen
	*hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);

	// Puts the bitmap in the memory, or something like that
	HBITMAP oldHBitmap = (HBITMAP) SelectObject(*hMemoryDC, *hBitmap);

	// Copies from the screen to the memory (consequently, the bitmap)
	BitBlt(*hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

	ReleaseDC(NULL, hScreenDC);

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
		DrawIcon(*hMemoryDC, iconPosX, iconPosY, cursorInfo.hCursor);
	}

	SelectObject(*hMemoryDC, oldHBitmap);
	
}

void GetHDCAndHBitmapBitmapInfoAndData(HDC * hdc, HBITMAP * hBitmap, BITMAPINFO * bitmapInfo, BYTE ** bitmapData) {

	// Get bitmap object
	BITMAP bitmap;
	GetObject(*hBitmap, sizeof(bitmap), &bitmap);

	// Make a bitmap info object
	(*bitmapInfo).bmiHeader.biSize = sizeof((*bitmapInfo).bmiHeader);
	(*bitmapInfo).bmiHeader.biWidth = bitmap.bmWidth;
	(*bitmapInfo).bmiHeader.biHeight = -bitmap.bmHeight;
	(*bitmapInfo).bmiHeader.biPlanes = bitmap.bmPlanes;
	(*bitmapInfo).bmiHeader.biBitCount = bitmap.bmBitsPixel;
	(*bitmapInfo).bmiHeader.biCompression = BI_RGB;

	// Size, with scanline size aligned to 4 bytes
	size_t bitmapSize = ((((bitmap.bmBitsPixel / 8) * bitmap.bmWidth) + 3) & ~3) * bitmap.bmHeight;
	fprintf(logFile, "Size of bitmap: %lld\n", bitmapSize);

	// Allocate bitmap data
	*bitmapData = new BYTE[bitmapSize];

	if (GetDIBits(*hdc, *hBitmap, 0, bitmap.bmHeight, *bitmapData, &(*bitmapInfo), DIB_RGB_COLORS) == 0) {
		fprintf(logFile, "Failed to GetDIBits!\n");
	};

}

// Converts the image into a format and puts in a file.
void ImageToFile(BITMAPINFO * bitmapInfo, BYTE ** bitmapData) {

	fprintf(logFile, "- Saving image to file.\n");

	// Make gdiplus object and save it
	Gdiplus::Bitmap * gdiBitmap = Gdiplus::Bitmap::FromBITMAPINFO(&(*bitmapInfo), *bitmapData);

	if (encoderMime == NULL) {
		fprintf(logFile, "Deducing format from file name extension\n");
		encoderMime = GetExtensionMIMEFormat(outputFileName);
		if (encoderMime == NULL) {
			fprintf(logFile, "Failed to deduce format, defaulting to image/bmp\n");
			encoderMime = L"image/bmp";
		}
	}

	fwprintf(logFile, L"encoderMime: %s\n", encoderMime);

	int result = GetEncoderClsid(encoderMime, &encoderCLSID);

	fprintf(logFile, "GetEncoderClsid returns: %d\n", result);
	if (result != 0) {
		if (result == 2) {
			fwprintf(logFile, L"No mime format called %s! Use --list-file-formats for a list of formats.\n", encoderMime);
			fwprintf(stderr, L"No mime format called %s! Use --list-file-formats for a list of formats.\n", encoderMime);
		} else {
			fprintf(logFile, "Failed to get encoder CLSID!\n");
		}
		exit(1);
	}

	Gdiplus::Status status = gdiBitmap -> Save(outputFileName, &encoderCLSID, NULL);

	fprintf(logFile, "Gdiplus::Save returns: %d\n", status);
	if (status != Gdiplus::Ok) {
		fprintf(logFile, "Failed to save file!\n");
	}

}

//
void ImageToClipboard(HBITMAP * outHBitmap) {

	fprintf(logFile, "- Copying image to clipboard.\n");

	if (OpenClipboard(NULL) == 0) {
		fprintf(logFile, "Failed to OpenClipboard!\n");
		exit(1);
	}

	if (EmptyClipboard() == 0) {
		fprintf(logFile, "Failed to EmptyClipboard!\n");
		exit(1);
	}

	if (SetClipboardData(CF_BITMAP, *outHBitmap) == NULL) {
		fprintf(logFile, "Failed to SetClipboardData!\n");
		exit(1);
	}

	if (CloseClipboard() == 0) {
		fprintf(logFile, "Failed to CloseClipboard!\n");
		exit(1);
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
   return 2;  // Failure
}