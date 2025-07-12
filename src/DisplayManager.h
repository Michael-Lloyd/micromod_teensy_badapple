#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>
#include <HyperDisplay_4DLCD-320240_4WSPI.h>

class DisplayManager {
private:
    uint8_t pinCS;
    uint8_t pinDC;
    uint8_t pinBacklight;
    LCD320240_4WSPI* display;
    bool displayInitialized;
    
public:
    DisplayManager(uint8_t csPin, uint8_t dcPin, uint8_t backlightPin);
    ~DisplayManager();
    
    // Initialize display (opens communication)
    bool begin(uint8_t brightness = 255);
    
    // Shutdown display (closes communication, releases SPI)
    void end();
    
    // Check if display is initialized
    bool isInitialized() const { return displayInitialized; }
    
    // Get display dimensions
    uint16_t getWidth() const;
    uint16_t getHeight() const;
    
    // Clear the display
    void clear();
    
    // Draw an image from memory buffer
    void drawImage(uint16_t x, uint16_t y, uint16_t* imageBuffer, 
                   uint16_t imageWidth, uint16_t imageHeight);
    
    // Draw a frame buffer using fast bulk transfer (optimized for video)
    void drawFrameBuffer(uint16_t* frameBuffer, uint16_t width, uint16_t height,
                        uint16_t x = 0, uint16_t y = 0);
    
    // Draw a single pixel
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    
    // Draw a filled rectangle
    void drawRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, 
                       uint16_t color, bool filled = true);
    
    // SPI management
    void releaseSPI();
    
private:
    void ensurePinsConfigured();
};

#endif // DISPLAYMANAGER_H