#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include <Arduino.h>
#include "SDFileReader.h"
#include "DisplayManager.h"
#include "RLEDecoder.h"

#pragma pack(push, 1)
struct VideoHeader {
    char magic[4];
    uint32_t frameCount;
    uint16_t frameWidth;
    uint16_t frameHeight;
    uint8_t fps;
    uint8_t compression;
    uint8_t reserved[2];    
    uint32_t indexOffset;
};

struct FrameIndexEntry {
    uint32_t offset;
    uint32_t size;
};
#pragma pack(pop)

class VideoPlayer {
private:
    SDFileReader* sdReader;
    DisplayManager* displayManager;
    const char* videoPath;
    VideoHeader header;
    bool isValid;
    
    uint8_t* compressedBuffer;
    uint16_t* segmentBuffer;
    uint32_t segmentSize;
    uint32_t rowsPerSegment;
    
    FrameIndexEntry* frameIndexCache;
    uint32_t indexCacheStart;
    uint32_t indexCacheSize;
    static const uint32_t INDEX_CACHE_FRAMES = 50;
    
    bool loadIndexCache(uint32_t startFrame);
    void cleanupBuffers();
    
public:
    VideoPlayer(SDFileReader* reader, DisplayManager* display, const char* path);
    ~VideoPlayer();
    
    bool begin();
    void end();
    
    bool playFrameSegmented(uint32_t frameNumber, uint16_t x, uint16_t y);
    
    uint16_t getWidth() const { return header.frameWidth; }
    uint16_t getHeight() const { return header.frameHeight; }
    uint16_t getFPS() const { return header.fps; }
    uint32_t getFrameCount() const { return header.frameCount; }
    bool isReady() const { return isValid; }
    uint32_t getSegmentRows() const { return rowsPerSegment; }
};

#endif