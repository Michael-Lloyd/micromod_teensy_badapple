#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include <Arduino.h>
#include "SDFileReader.h"
#include "DisplayManager.h"
#include "RLEDecoder.h"

// Video header structure matching the .vid format
#pragma pack(push, 1)
struct VideoHeader {
    char magic[4];          // "VID0"
    uint32_t frameCount;
    uint16_t frameWidth;
    uint16_t frameHeight;
    uint8_t fps;
    uint8_t compression;    // Compression type: 1=RLE
    uint8_t reserved[2];    
    uint32_t indexOffset;   // Offset to frame index table
};

// Frame index entry structure
struct FrameIndexEntry {
    uint32_t offset;        // File offset to frame data
    uint32_t size;          // Size of compressed frame data
};
#pragma pack(pop)

class VideoPlayer {
public:
    // Timing metrics
    struct SegmentMetrics {
        uint32_t readTime;
        uint32_t swapTime;
        uint32_t drawTime;
    };
    
private:
    SDFileReader* sdReader;
    DisplayManager* displayManager;
    const char* videoPath;
    VideoHeader header;
    bool isValid;
    
    // Buffers for RLE decompression
    uint8_t* compressedBuffer;     // Buffer for compressed frame data (100KB)
    uint16_t* segmentBuffer;       // Buffer for decompressed segment (100KB worth of pixels)
    uint32_t segmentSize;          // Size of segment buffer in pixels
    uint32_t rowsPerSegment;       // Number of rows that fit in segment
    
    // Frame index cache (now stores FrameIndexEntry)
    FrameIndexEntry* frameIndexCache;
    uint32_t indexCacheStart;
    uint32_t indexCacheSize;
    static const uint32_t INDEX_CACHE_FRAMES = 50;  // Reduced since entries are larger
    
    bool loadIndexCache(uint32_t startFrame);
    void cleanupBuffers();
    
public:
    VideoPlayer(SDFileReader* reader, DisplayManager* display, const char* path);
    ~VideoPlayer();
    
    bool begin();
    void end();
    
    // Play a single frame in segments
    bool playFrameSegmented(uint32_t frameNumber, uint16_t x, uint16_t y, SegmentMetrics* metrics = nullptr);
    
    // Getters
    uint16_t getWidth() const { return header.frameWidth; }
    uint16_t getHeight() const { return header.frameHeight; }
    uint16_t getFPS() const { return header.fps; }
    uint32_t getFrameCount() const { return header.frameCount; }
    bool isReady() const { return isValid; }
    uint32_t getSegmentRows() const { return rowsPerSegment; }
};

#endif // VIDEO_PLAYER_H