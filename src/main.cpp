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
    
    // Performance metrics
    struct {
        uint32_t totalFrames = 0;
        uint32_t droppedFrames = 0;
        // Segment-level timing
        uint32_t totalSegmentReadTime = 0;
        uint32_t minSegmentReadTime = 999999;
        uint32_t maxSegmentReadTime = 0;
        uint32_t totalSegmentSwapTime = 0;
        uint32_t minSegmentSwapTime = 999999;
        uint32_t maxSegmentSwapTime = 0;
        uint32_t totalSegmentDrawTime = 0;
        uint32_t minSegmentDrawTime = 999999;
        uint32_t maxSegmentDrawTime = 0;
        // Frame-level timing
        uint32_t totalFrameReadTime = 0;
        uint32_t minFrameReadTime = 999999;
        uint32_t maxFrameReadTime = 0;
        uint32_t totalFrameSwapTime = 0;
        uint32_t minFrameSwapTime = 999999;
        uint32_t maxFrameSwapTime = 0;
        uint32_t totalFrameDrawTime = 0;
        uint32_t minFrameDrawTime = 999999;
        uint32_t maxFrameDrawTime = 0;
        // SPI release/acquire timing
        uint32_t totalSPIReleaseTime = 0;
        uint32_t minSPIReleaseTime = 999999;
        uint32_t maxSPIReleaseTime = 0;
        // Frame wait/delay timing
        uint32_t totalWaitTime = 0;
        uint32_t minWaitTime = 999999;
        uint32_t maxWaitTime = 0;
        // Total frame processing time
        uint32_t totalFrameTime = 0;
        uint32_t minFrameTime = 999999;
        uint32_t maxFrameTime = 0;
        // Segment count
        uint32_t totalSegments = 0;
    } metrics;
    
    // Initialize SD card
    if (!sdReader.begin()) {
        Serial.println("Failed to initialize SD card!");
        return;
    }
    
    // Initialize display
    Serial.println("Initializing display...");
    if (!displayManager.begin()) {
        Serial.println("Failed to initialize display!");
        sdReader.end();
        return;
    }
    displayManager.clear();
    
    // Create video player
    VideoPlayer video(&sdReader, &displayManager, "/bad_apple_rle.vid");
    
    // Initialize video
    if (!video.begin()) {
        Serial.println("Failed to open video file!");
        displayManager.end();
        sdReader.end();
        return;
    }
    
    // Get video properties
    uint16_t frameCount = video.getFrameCount();
    uint16_t fps = video.getFPS();
    uint16_t videoWidth = video.getWidth();
    uint16_t videoHeight = video.getHeight();
    uint32_t segmentRows = video.getSegmentRows();
    uint32_t segmentsPerFrame = (videoHeight + segmentRows - 1) / segmentRows;
    
    Serial.print("Video: ");
    Serial.print(frameCount);
    Serial.print(" frames @ ");
    Serial.print(fps);
    Serial.print(" FPS, ");
    Serial.print(videoWidth);
    Serial.print("x");
    Serial.println(videoHeight);
    Serial.print("Segments per frame: ");
    Serial.println(segmentsPerFrame);
    
    // Frame timing configuration
    const uint32_t frameDelay = 1000 / fps;  // milliseconds per frame
    
    Serial.print("Frame delay: ");
    Serial.print(frameDelay);
    Serial.println("ms per frame");
    
    // Calculate center position
    uint16_t x = (240 - videoWidth) / 2;
    uint16_t y = (320 - videoHeight) / 2;
    
    // Play all frames
    uint32_t startTime = millis();
    uint32_t nextFrameTime = startTime;
    
    uint32_t frameNum = 0;
    bool summaryPrinted = false; // TIMING_INSTRUMENTATION: Track if we've printed summary
    while (frameNum < frameCount) {
        uint32_t frameStartTime = millis();
        
        // TIMING_INSTRUMENTATION: Print summary after 10 seconds
        if (!summaryPrinted && (millis() - startTime) > 10000) {
            sdReader.printTimingSummary();
            summaryPrinted = true;
        }
        
        // Calculate when this frame should be displayed
        nextFrameTime = startTime + (frameNum * frameDelay);
        
        // Step 1: Release SPI for SD card access
        uint32_t spiReleaseStart = micros();
        displayManager.releaseSPI();
        uint32_t spiReleaseTime = micros() - spiReleaseStart;
        metrics.totalSPIReleaseTime += spiReleaseTime;
        metrics.minSPIReleaseTime = min(metrics.minSPIReleaseTime, spiReleaseTime);
        metrics.maxSPIReleaseTime = max(metrics.maxSPIReleaseTime, spiReleaseTime);
        
        // Step 2: Play frame in segments
        VideoPlayer::SegmentMetrics segMetrics = {0, 0, 0};
        
        uint32_t frameProcessStart = micros();
        bool success = video.playFrameSegmented(frameNum, x, y, &segMetrics);
        uint32_t frameProcessTime = micros() - frameProcessStart;
        
        if (!success) {
            Serial.print("Failed to play frame ");
            Serial.println(frameNum);
            break;
        }
        
        // Update segment-level metrics (these are already in microseconds)
        metrics.totalSegmentReadTime += segMetrics.readTime;
        metrics.minSegmentReadTime = min(metrics.minSegmentReadTime, segMetrics.readTime / segmentsPerFrame);
        metrics.maxSegmentReadTime = max(metrics.maxSegmentReadTime, segMetrics.readTime / segmentsPerFrame);
        
        metrics.totalSegmentSwapTime += segMetrics.swapTime;
        metrics.minSegmentSwapTime = min(metrics.minSegmentSwapTime, segMetrics.swapTime / segmentsPerFrame);
        metrics.maxSegmentSwapTime = max(metrics.maxSegmentSwapTime, segMetrics.swapTime / segmentsPerFrame);
        
        metrics.totalSegmentDrawTime += segMetrics.drawTime;
        metrics.minSegmentDrawTime = min(metrics.minSegmentDrawTime, segMetrics.drawTime / segmentsPerFrame);
        metrics.maxSegmentDrawTime = max(metrics.maxSegmentDrawTime, segMetrics.drawTime / segmentsPerFrame);
        
        // Update frame-level metrics (convert to milliseconds)
        uint32_t frameReadTime = segMetrics.readTime / 1000;
        uint32_t frameSwapTime = segMetrics.swapTime / 1000;
        uint32_t frameDrawTime = segMetrics.drawTime / 1000;
        
        metrics.totalFrameReadTime += frameReadTime;
        metrics.minFrameReadTime = min(metrics.minFrameReadTime, frameReadTime);
        metrics.maxFrameReadTime = max(metrics.maxFrameReadTime, frameReadTime);
        
        metrics.totalFrameSwapTime += frameSwapTime;
        metrics.minFrameSwapTime = min(metrics.minFrameSwapTime, frameSwapTime);
        metrics.maxFrameSwapTime = max(metrics.maxFrameSwapTime, frameSwapTime);
        
        metrics.totalFrameDrawTime += frameDrawTime;
        metrics.minFrameDrawTime = min(metrics.minFrameDrawTime, frameDrawTime);
        metrics.maxFrameDrawTime = max(metrics.maxFrameDrawTime, frameDrawTime);
        
        metrics.totalSegments += segmentsPerFrame;
        
        // Step 3: Frame rate maintenance (wait/delay)
        uint32_t waitStart = millis();
        uint32_t now = millis();
        uint32_t waitTime = 0;
        
        if (now < nextFrameTime) {
            waitTime = nextFrameTime - now;
            delay(waitTime);
        } else if (now > nextFrameTime + frameDelay) {
            metrics.droppedFrames++;
        }
        metrics.totalWaitTime += waitTime;
        if (waitTime > 0) {
            metrics.minWaitTime = min(metrics.minWaitTime, waitTime);
            metrics.maxWaitTime = max(metrics.maxWaitTime, waitTime);
        }
        
        // Calculate total frame time
        uint32_t frameTime = millis() - frameStartTime;
        metrics.totalFrameTime += frameTime;
        metrics.minFrameTime = min(metrics.minFrameTime, frameTime);
        metrics.maxFrameTime = max(metrics.maxFrameTime, frameTime);
        
        metrics.totalFrames++;
        
        // Progress report every second
        if (frameNum % fps == 0 && frameNum > 0) {
            Serial.print("Frame ");
            Serial.print(frameNum);
            Serial.print("/");
            Serial.print(frameCount);
            Serial.print(" @ ");
            Serial.print((millis() - startTime) / 1000.0);
            Serial.print("s - Dropped: ");
            Serial.println(metrics.droppedFrames);
        }
        
        // Advance to next frame
        frameNum++;
        
        // Detailed timing after 5 seconds
        static bool reportPrinted = false;
        if (!reportPrinted && (millis() - startTime) >= 5000) {
            reportPrinted = true;
            Serial.println("\n========= TIMING REPORT ==========");
            
            Serial.println("1. SPI Release (microseconds):");
            Serial.print("   Avg: ");
            Serial.print(metrics.totalSPIReleaseTime / (float)metrics.totalFrames);
            Serial.print("us (min: ");
            Serial.print(metrics.minSPIReleaseTime);
            Serial.print(", max: ");
            Serial.print(metrics.maxSPIReleaseTime);
            Serial.println(")");
            
            Serial.println("\n=== SEGMENT-LEVEL TIMING (microseconds) ===");
            Serial.print("Segments per frame: ");
            Serial.println(segmentsPerFrame);
            
            Serial.println("2a. Segment Read:");
            Serial.print("   Avg: ");
            Serial.print(metrics.totalSegmentReadTime / (float)metrics.totalSegments);
            Serial.print("us (min: ");
            Serial.print(metrics.minSegmentReadTime);
            Serial.print(", max: ");
            Serial.print(metrics.maxSegmentReadTime);
            Serial.println(")");
            
            Serial.println("2b. Segment Byte Swap:");
            Serial.print("   Avg: ");
            Serial.print(metrics.totalSegmentSwapTime / (float)metrics.totalSegments);
            Serial.print("us (min: ");
            Serial.print(metrics.minSegmentSwapTime);
            Serial.print(", max: ");
            Serial.print(metrics.maxSegmentSwapTime);
            Serial.println(")");
            
            Serial.println("2c. Segment Draw:");
            Serial.print("   Avg: ");
            Serial.print(metrics.totalSegmentDrawTime / (float)metrics.totalSegments);
            Serial.print("us (min: ");
            Serial.print(metrics.minSegmentDrawTime);
            Serial.print(", max: ");
            Serial.print(metrics.maxSegmentDrawTime);
            Serial.println(")");
            
            Serial.println("\n=== FRAME-LEVEL TIMING (milliseconds) ===");
            
            Serial.println("3a. Total Frame Read:");
            Serial.print("   Avg: ");
            Serial.print(metrics.totalFrameReadTime / (float)metrics.totalFrames);
            Serial.print("ms (min: ");
            Serial.print(metrics.minFrameReadTime);
            Serial.print(", max: ");
            Serial.print(metrics.maxFrameReadTime);
            Serial.println(")");
            
            Serial.println("3b. Total Frame Swap:");
            Serial.print("   Avg: ");
            Serial.print(metrics.totalFrameSwapTime / (float)metrics.totalFrames);
            Serial.print("ms (min: ");
            Serial.print(metrics.minFrameSwapTime);
            Serial.print(", max: ");
            Serial.print(metrics.maxFrameSwapTime);
            Serial.println(")");
            
            Serial.println("3c. Total Frame Draw:");
            Serial.print("   Avg: ");
            Serial.print(metrics.totalFrameDrawTime / (float)metrics.totalFrames);
            Serial.print("ms (min: ");
            Serial.print(metrics.minFrameDrawTime);
            Serial.print(", max: ");
            Serial.print(metrics.maxFrameDrawTime);
            Serial.println(")");
            
            Serial.println("\n4. Frame Wait/Delay:");
            Serial.print("   Avg: ");
            Serial.print(metrics.totalWaitTime / (float)metrics.totalFrames);
            Serial.print("ms (min: ");
            Serial.print(metrics.minWaitTime == 999999 ? 0 : metrics.minWaitTime);
            Serial.print(", max: ");
            Serial.print(metrics.maxWaitTime == 0 ? 0 : metrics.maxWaitTime);
            Serial.println(")");
            
            Serial.println("\nTOTAL Frame Time:");
            Serial.print("   Avg: ");
            Serial.print(metrics.totalFrameTime / (float)metrics.totalFrames);
            Serial.print("ms (min: ");
            Serial.print(metrics.minFrameTime);
            Serial.print(", max: ");
            Serial.print(metrics.maxFrameTime);
            Serial.println(")");
            Serial.print("   Target: ");
            Serial.print(frameDelay);
            Serial.println("ms");
            
            Serial.println("\nPerformance Summary:");
            Serial.print("   Actual FPS: ");
            Serial.println(1000.0 * metrics.totalFrames / (millis() - startTime));
            Serial.print("   Dropped frames: ");
            Serial.println(metrics.droppedFrames);
            
            // Calculate percentage of time spent in each phase
            uint32_t totalActiveTime = metrics.totalFrameReadTime + metrics.totalFrameSwapTime + metrics.totalFrameDrawTime;
            Serial.println("\nTime Distribution:");
            Serial.print("   SD Read: ");
            Serial.print(100.0 * metrics.totalFrameReadTime / totalActiveTime);
            Serial.println("%");
            Serial.print("   Byte Swap: ");
            Serial.print(100.0 * metrics.totalFrameSwapTime / totalActiveTime);
            Serial.println("%");
            Serial.print("   Display: ");
            Serial.print(100.0 * metrics.totalFrameDrawTime / totalActiveTime);
            Serial.println("%");
            Serial.print("   Waiting: ");
            Serial.print(100.0 * metrics.totalWaitTime / metrics.totalFrameTime);
            Serial.println("%");
            
            Serial.println("=====================================\n");
        }
    }
    
    // Cleanup
    displayManager.end();
    video.end();
    sdReader.end();
    
    // Final report
    uint32_t totalTime = millis() - startTime;
    Serial.println("\n=== FINAL PERFORMANCE ===");
    Serial.print("Playback time: ");
    Serial.print(totalTime / 1000.0);
    Serial.print("s, Avg FPS: ");
    Serial.println(1000.0 * metrics.totalFrames / totalTime);
    Serial.print("Dropped frames: ");
    Serial.println(metrics.droppedFrames);
    Serial.print("Total segments processed: ");
    Serial.println(metrics.totalSegments);
    Serial.print("Avg segments per frame: ");
    Serial.println(metrics.totalSegments / (float)metrics.totalFrames);
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);
    
    #if defined(__IMXRT1062__)
    Serial.print("CPU Clock: ");
    Serial.print(F_CPU / 1000000);
    Serial.println(" MHz");
    #endif
    
    Serial.println("\n=== VIDEO PLAYER ===");
    Serial.println("Using max 200KB segment buffer");
    
    // Play the video
    playVideo();
    
    Serial.println("\nPlayback complete.");
}

void loop() {
    // Nothing to do in loop
    delay(1000);
}