#include "DisplayManager.h"
#include <SPI.h>

DisplayManager::DisplayManager(uint8_t csPin, uint8_t dcPin, uint8_t backlightPin) 
    : pinCS(csPin), pinDC(dcPin), pinBacklight(backlightPin), 
      display(nullptr), displayInitialized(false) {
}

DisplayManager::~DisplayManager() {
    if (displayInitialized) {
        end();
    }
    if (display) {
        delete display;
    }
}

void DisplayManager::ensurePinsConfigured() {
    pinMode(pinCS, OUTPUT);
    pinMode(pinDC, OUTPUT);
    pinMode(pinBacklight, OUTPUT);
    
    digitalWrite(pinCS, HIGH);
}

bool DisplayManager::begin(uint8_t brightness) {
    if (displayInitialized) {
        return true;
    }
    
    ensurePinsConfigured();
    
    SPI.begin();
    
    analogWrite(pinBacklight, brightness);
    
    if (!display) {
        display = new LCD320240_4WSPI();
    }
    
    display->begin(pinDC, pinCS, pinBacklight);
    
    display->clearDisplay();
    
    displayInitialized = true;
    
    return true;
}

void DisplayManager::end() {
    if (!displayInitialized) {
        return;
    }
    
    analogWrite(pinBacklight, 0);
    
    digitalWrite(pinCS, HIGH);
    
    delay(10);
    
    displayInitialized = false;
}

void DisplayManager::releaseSPI() {
    digitalWrite(pinCS, HIGH);
    
    SPI.endTransaction();
}

uint16_t DisplayManager::getWidth() const {
    if (display && displayInitialized) {
        return display->xExt;
    }
    return 0;
}

uint16_t DisplayManager::getHeight() const {
    if (display && displayInitialized) {
        return display->yExt;
    }
    return 0;
}

void DisplayManager::clear() {
    if (!display || !displayInitialized) {
        return;
    }
    
    display->clearDisplay();
}

void DisplayManager::drawImage(uint16_t x, uint16_t y, uint16_t* imageBuffer, 
                               uint16_t imageWidth, uint16_t imageHeight) {
    if (!display || !displayInitialized || !imageBuffer) {
        return;
    }
    
    uint16_t displayWidth = display->xExt;
    uint16_t displayHeight = display->yExt;
    
    for (uint16_t row = 0; row < imageHeight; row++) {
        for (uint16_t col = 0; col < imageWidth; col++) {
            if ((x + col < displayWidth) && (y + row < displayHeight)) {
                uint16_t pixel = imageBuffer[row * imageWidth + col];
                display->pixel(x + col, y + row, &pixel);
            }
        }
    }
}

void DisplayManager::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (!display || !displayInitialized) {
        return;
    }
    
    if (x < display->xExt && y < display->yExt) {
        display->pixel(x, y, &color);
    }
}

void DisplayManager::drawRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, 
                                   uint16_t color, bool filled) {
    if (!display || !displayInitialized) {
        return;
    }
    
    display->rectangle(x0, y0, x1, y1, filled, &color);
}

void DisplayManager::drawFrameBuffer(uint16_t* frameBuffer, uint16_t width, uint16_t height,
                                    uint16_t x, uint16_t y) {
    if (!display || !displayInitialized || !frameBuffer) {
        return;
    }
    
    display->hwfillFromArray(x, y, x + width - 1, y + height - 1, 
                            frameBuffer, width * height, false);
}