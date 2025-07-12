#include <Arduino.h>
#include <SPI.h>
#include "DisplayManager.h"
#include "SDFileReader.h"
#include "VideoPlayer.h"

// Pin definitions for MicroMod Input and Display Carrier Board
#define PIN_SPI_CS    4   // Display Chip Select
#define PIN_SPI_DC    5   // Display Data/Command
#define PIN_BACKLIGHT 3   // Display Backlight
#define PIN_SD_CS     10  // SD Card Chip Select

// Create managers
DisplayManager displayManager(PIN_SPI_CS, PIN_SPI_DC, PIN_BACKLIGHT);
SDFileReader sdReader(PIN_SD_CS);

void playVideo() {
    
    // Initialize SD card
    if (!sdReader.begin()) {
        return;
    }
    
    // Initialize display
    if (!displayManager.begin()) {
        sdReader.end();
        return;
    }
    displayManager.clear();
    
    // Create video player
    VideoPlayer video(&sdReader, &displayManager, "/bad_apple_rle.vid");
    
    // Initialize video
    if (!video.begin()) {
        displayManager.end();
        sdReader.end();
        return;
    }
    
    // Get video properties
    uint16_t frameCount = video.getFrameCount();
    uint16_t fps = video.getFPS();
    uint16_t videoWidth = video.getWidth();
    uint16_t videoHeight = video.getHeight();
    
    // Frame timing configuration
    const uint32_t frameDelay = 1000 / fps;  // milliseconds per frame
    
    // Calculate center position
    uint16_t x = (240 - videoWidth) / 2;
    uint16_t y = (320 - videoHeight) / 2;
    
    // Play all frames
    uint32_t startTime = millis();
    uint32_t nextFrameTime = startTime;
    
    uint32_t frameNum = 0;
    while (frameNum < frameCount) {
        
        // Calculate when this frame should be displayed
        nextFrameTime = startTime + (frameNum * frameDelay);
        
        // Step 1: Release SPI for SD card access
        displayManager.releaseSPI();
        
        // Step 2: Play frame in segments
        bool success = video.playFrameSegmented(frameNum, x, y);
        
        if (!success) {
            break;
        }
        
        // Step 3: Frame rate maintenance (wait/delay)
        uint32_t now = millis();
        
        if (now < nextFrameTime) {
            delay(nextFrameTime - now);
        }
        
        
        // Advance to next frame
        frameNum++;
    }
    
    // Cleanup
    displayManager.end();
    video.end();
    sdReader.end();
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);
    
    // Play the video
    playVideo();
}

void loop() {
    // Nothing to do in loop
    delay(1000);
}