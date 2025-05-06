#include "ScreenCapture.h"
#include <iostream>
#include <stdexcept>
#include <vector>

// In ScreenCapture.cpp
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb_image_write.h"

ScreenCapture::ScreenCapture() {
    // Retrieve the handle to a display device context for the client 
    // area of the window. 
    HDC hdcScreen = GetWindowDC(NULL);

    // Check if screen is successful
    if (!hdcScreen) {
        throw std::runtime_error("Failed to get screen DC");
    }

    // Get width of screen
    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    std::cout << "width: " << screenWidth << std::endl;

    // Get height of screen
    screenHeight = GetSystemMetrics(SM_CYSCREEN);
    std::cout << "height: " << screenHeight << std::endl;

    // Create a compatible DC, which is used in a BitBlt from the window DC.
    hdcMemDC = CreateCompatibleDC(hdcScreen);
    if (!hdcMemDC) {
        ReleaseDC(NULL, hdcScreen);
        throw std::runtime_error("Failed to create compatible DC");
    }

    // 3. Create bitmap
    hBitmap = CreateCompatibleBitmap(hdcScreen, screenWidth, screenHeight);
    std::cout << hBitmap << std::endl;
    if (!hBitmap) {
        ReleaseDC(NULL, hdcScreen);
        DeleteDC(hdcMemDC);
        throw std::runtime_error("Failed to create bitmap");
    }

    // 4. Bind bitmap to Device Context
    SelectObject(hdcMemDC, hBitmap);

    // 5. Release Screen-HDC
    ReleaseDC(NULL, hdcScreen);
}

ScreenCapture::~ScreenCapture() {
    cleanup();
}

void ScreenCapture::cleanup() {
    // Release bitmap
    if (hBitmap) {
        DeleteObject(hBitmap);
        hBitmap = nullptr;
    }

    // Release Device Context
    if (hdcMemDC) {
        DeleteDC(hdcMemDC);
        hdcMemDC = nullptr;
    }
}

bool ScreenCapture::captureScreen() {
    // 1. Get Screen-HDC (Device Context)
    HDC hdcScreen = GetWindowDC(NULL);
    if (!hdcScreen) return false;

    // 2. Copy content of screen
    if (!BitBlt(
        hdcMemDC,   // Target
        0, 0,       // Target-Position (x, y)
        screenWidth, screenHeight, // Size
        hdcScreen,  // Source (Display)
        0, 0,       // Source-Position (x, y)
        SRCCOPY     // Copymode (easy copy)
    )) {
        // 3. Release HDC
        ReleaseDC(NULL, hdcScreen);
        return false;
    }

    // 3. Release HDC
    ReleaseDC(NULL, hdcScreen);
    return true;
}

bool ScreenCapture::saveToFile(const std::wstring& filename) const {
    if (!hBitmap) return false;

    BITMAPFILEHEADER bmfHeader = { 0 };
    BITMAPINFOHEADER bi = { 0 };

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = screenWidth;
    bi.biHeight = screenHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;

    DWORD dwBmpSize = ((screenWidth * bi.biBitCount + 31) / 32) * 4 * screenHeight;

    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE,
        FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // Set up the bitmap file header
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = dwSizeofDIB;
    bmfHeader.bfType = 0x4D42; // "BM"

    DWORD dwBytesWritten = 0;
    WriteFile(hFile, &bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);

    BYTE* lpbitmap = new BYTE[dwBmpSize];
    GetDIBits(hdcMemDC, hBitmap, 0, screenHeight, lpbitmap,
        (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    WriteFile(hFile, lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

    delete[] lpbitmap;
    CloseHandle(hFile);

    return true;
}

bool ScreenCapture::saveToPNG(const std::wstring& filename, bool transformTo24BitDeep) const {
    if (!hBitmap) return false;

    // 1. Extract bitmap-data (24-Bit RGB)
    int width, height, bpp;
    BYTE* pixels = getBitmapData(width, height, bpp); // Nutze deine existierende Methode

    std::cout << "bpp: " << bpp << std::endl;

    if (!pixels)
        return false;

    if (bpp != 24 && bpp != 32) {
        delete[] pixels;
        std::cerr << "Unsupported bit depth: " << bpp << " bpp\n";
        return false;
    }

    // 2. Konvert BRG to RGB
    int srcChannels = bpp / 8;  // 3 (BGR) oder 4 (BGRA)
    int dstChannels = transformTo24BitDeep ? 3 : srcChannels;
    std::vector<BYTE> pngPixels(width * height * srcChannels);
    for (int i = 0; i < width * height; ++i) {
        pngPixels[i * dstChannels + 0] = pixels[i * srcChannels + 2]; // R (aus BGR)
        pngPixels[i * dstChannels + 1] = pixels[i * srcChannels + 1]; // G
        pngPixels[i * dstChannels + 2] = pixels[i * srcChannels + 0]; // B

        // Copy Alpha-Channel if srcChannel and dstChannel are 4
        if (srcChannels == 4 && dstChannels == 4) {
            pngPixels[i * dstChannels + 3] = pixels[i * srcChannels + 3]; // A
        }
    }

    // 3. Store as PNG
    std::string narrowFilename(filename.begin(), filename.end());
    int success = stbi_write_png(
        narrowFilename.c_str(),
        width,
        height,
        dstChannels,          // RGB = 3 Kanäle
        pngPixels.data(),
        width * dstChannels   // Bytes pro Zeile
    );

    delete[] pixels;
    return success != 0;
}

BYTE* ScreenCapture::getBitmapData(int& width, int& height, int& bitsPerPixel) const {
    if (!hBitmap) return nullptr;

    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    width = bmp.bmWidth;
    height = bmp.bmHeight;
    bitsPerPixel = bmp.bmBitsPixel;

    const DWORD bufferSize = bmp.bmWidthBytes * bmp.bmHeight;
    BYTE* buffer = new BYTE[bufferSize];

    GetBitmapBits(hBitmap, bufferSize, buffer);

    return buffer;
}