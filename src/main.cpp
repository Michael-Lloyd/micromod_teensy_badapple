#include <Arduino.h>
#include <SPI.h>
#include "DisplayManager.h"
#include "SDFileReader.h"
#include "VideoPlayer.h"

#define PIN_SPI_CS    4
#define PIN_SPI_DC    5
#define PIN_BACKLIGHT 3
#define PIN_SD_CS     10

DisplayManager displayManager(PIN_SPI_CS, PIN_SPI_DC, PIN_BACKLIGHT);
SDFileReader sdReader(PIN_SD_CS);

void playVideo() {
    if (!sdReader.begin()) {
        return;
    }
    if (!displayManager.begin()) {
        sdReader.end();
        return;
    }
    displayManager.clear();
    VideoPlayer video(&sdReader, &displayManager, "/bad_apple_rle.vid");
    if (!video.begin()) {
        displayManager.end();
        sdReader.end();
        return;
    }
    uint16_t frameCount = video.getFrameCount();
    uint16_t fps = video.getFPS();
    uint16_t videoWidth = video.getWidth();
    uint16_t videoHeight = video.getHeight();
    const uint32_t frameDelay = 1000 / fps;
    uint16_t x = (240 - videoWidth) / 2;
    uint16_t y = (320 - videoHeight) / 2;
    uint32_t startTime = millis();
    uint32_t nextFrameTime = startTime;
    
    uint32_t frameNum = 0;
    while (frameNum < frameCount) {
        nextFrameTime = startTime + (frameNum * frameDelay);
        displayManager.releaseSPI();
        bool success = video.playFrameSegmented(frameNum, x, y);
        
        if (!success) {
            break;
        }
        uint32_t now = millis();
        
        if (now < nextFrameTime) {
            delay(nextFrameTime - now);
        }
        frameNum++;
    }
    displayManager.end();
    video.end();
    sdReader.end();
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);
    playVideo();
}

void loop() {
    delay(1000);
}