#include "capture/ScreenCapture.h"
#include <iostream>

int main() {
    try {
        ScreenCapture capture;

        if (capture.captureScreen()) {
            if (capture.saveToFile(L"screenshot.bmp")) {
                std::cout << "Screenshot saved successfully!\n";
                if (capture.saveToPNG(L"screenshot.png", false)) {
                    std::cout << "PNG screenshot saved successfully!\n";
                }
                else {
                    std::cerr << "Failed to save PNG screenshot\n";
                }
            }
            else {
                std::cerr << "Failed to save screenshot\n";
            }
        }
        else {
            std::cerr << "Failed to capture screen\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}