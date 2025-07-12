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
    
    bool begin(uint8_t brightness = 255);
    
    void end();
    
    bool isInitialized() const { return displayInitialized; }
    
    uint16_t getWidth() const;
    uint16_t getHeight() const;
    
    void clear();
    
    void drawImage(uint16_t x, uint16_t y, uint16_t* imageBuffer, 
                   uint16_t imageWidth, uint16_t imageHeight);
    
    void drawFrameBuffer(uint16_t* frameBuffer, uint16_t width, uint16_t height,
                        uint16_t x = 0, uint16_t y = 0);
    
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    
    void drawRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, 
                       uint16_t color, bool filled = true);
    
    void releaseSPI();
    
private:
    void ensurePinsConfigured();
};

#endif