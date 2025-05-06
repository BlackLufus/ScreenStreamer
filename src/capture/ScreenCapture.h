#pragma once

#include <windows.h>
#include <string>

class ScreenCapture {
public:
    // Constructor
    ScreenCapture();

    // Deconstructor
    ~ScreenCapture();

    // Capture entire screen
    bool captureScreen();

    // Save captured bitmap to file (BMP format)
    bool saveToFile(const std::wstring& filename) const;

    bool saveToPNG(const std::wstring& filename, bool transformTo24BitDeep = true) const;

    // Get raw bitmap data (caller must free with delete[])
    BYTE* getBitmapData(int& width, int& height, int& bitsPerPixel) const;

private:
    HBITMAP hBitmap = nullptr;
    HDC hdcMemDC = nullptr;
    int screenWidth = 0;
    int screenHeight= 0;

    void cleanup();
};