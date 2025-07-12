#include "VideoPlayer.h"
#include <Arduino.h>
#include <cstring>

VideoPlayer::VideoPlayer(SDFileReader* reader, DisplayManager* display, const char* path) 
    : sdReader(reader), displayManager(display), videoPath(path), isValid(false), 
      compressedBuffer(nullptr), segmentBuffer(nullptr), frameIndexCache(nullptr), 
      indexCacheStart(0), indexCacheSize(0) {
}

VideoPlayer::~VideoPlayer() {
    cleanupBuffers();
}

void VideoPlayer::cleanupBuffers() {
    if (compressedBuffer) {
        delete[] compressedBuffer;
        compressedBuffer = nullptr;
    }
    
    if (segmentBuffer) {
        delete[] segmentBuffer;
        segmentBuffer = nullptr;
    }
    
    if (frameIndexCache) {
        delete[] frameIndexCache;
        frameIndexCache = nullptr;
    }
}

bool VideoPlayer::begin() {
    // Read header
    size_t headerSize;
    uint8_t* headerData = sdReader->readPartialFile(videoPath, headerSize, sizeof(VideoHeader), 0);
    
    if (!headerData || headerSize < sizeof(VideoHeader)) {
        if (headerData) delete[] headerData;
        return false;
    }
    
    // Copy header data
    memcpy(&header, headerData, sizeof(VideoHeader));
    delete[] headerData;
    
    // Validate header
    if (memcmp(header.magic, "VID0", 4) != 0) {
        return false;
    }
    
    // Check compression type
    if (header.compression != 1) {
        return false;
    }
    
    // Check if index offset looks reasonable
    if (header.indexOffset == 0) {
        header.indexOffset = 24;  // Default position after header
    }
    
    // Allocate buffers for RLE decompression
    // 100KB for compressed data buffer
    const uint32_t COMPRESSED_BUFFER_SIZE = 100 * 1024;
    compressedBuffer = new uint8_t[COMPRESSED_BUFFER_SIZE];
    if (!compressedBuffer) {
        cleanupBuffers();
        return false;
    }
    
    // Calculate segment buffer size (100KB worth of pixels)
    const uint32_t SEGMENT_BUFFER_BYTES = 100 * 1024;
    segmentSize = SEGMENT_BUFFER_BYTES / sizeof(uint16_t);  // Number of pixels
    
    // Calculate rows per segment for display purposes
    rowsPerSegment = segmentSize / header.frameWidth;
    if (rowsPerSegment > header.frameHeight) {
        rowsPerSegment = header.frameHeight;
        segmentSize = header.frameWidth * rowsPerSegment;
    }
    
    
    // Allocate segment buffer
    segmentBuffer = new uint16_t[segmentSize];
    if (!segmentBuffer) {
        cleanupBuffers();
        return false;
    }
    
    // Allocate frame index cache
    frameIndexCache = new FrameIndexEntry[INDEX_CACHE_FRAMES];
    if (!frameIndexCache) {
        cleanupBuffers();
        return false;
    }
    
    // Load first batch of frame indices
    loadIndexCache(0);
    
    // Open the video file for sequential reading
    if (!sdReader->openFile(videoPath)) {
        cleanupBuffers();
        return false;
    }
    
    isValid = true;
    return true;
}

void VideoPlayer::end() {
    isValid = false;
    sdReader->closeFile();
    cleanupBuffers();
}

bool VideoPlayer::loadIndexCache(uint32_t startFrame) {
    if (startFrame >= header.frameCount) return false;
    
    uint32_t framesToCache = min(INDEX_CACHE_FRAMES, header.frameCount - startFrame);
    uint32_t indexPosition = header.indexOffset + (startFrame * sizeof(FrameIndexEntry));
    size_t bytesRead;
    
    uint8_t* indexData = sdReader->readPartialFile(videoPath, bytesRead, 
                                                   framesToCache * sizeof(FrameIndexEntry), 
                                                   indexPosition);
    
    if (!indexData || bytesRead < framesToCache * sizeof(FrameIndexEntry)) {
        if (indexData) delete[] indexData;
        return false;
    }
    
    memcpy(frameIndexCache, indexData, framesToCache * sizeof(FrameIndexEntry));
    delete[] indexData;
    
    indexCacheStart = startFrame;
    indexCacheSize = framesToCache;
    
    return true;
}

bool VideoPlayer::playFrameSegmented(uint32_t frameNumber, uint16_t x, uint16_t y) {
    if (!isValid || frameNumber >= header.frameCount) {
        return false;
    }
    
    // Get frame info from cache or load new cache if needed
    FrameIndexEntry frameEntry;
    if (frameNumber >= indexCacheStart && frameNumber < indexCacheStart + indexCacheSize) {
        frameEntry = frameIndexCache[frameNumber - indexCacheStart];
    } else {
        if (!loadIndexCache(frameNumber)) {
            return false;
        }
        frameEntry = frameIndexCache[0];
    }
    
    // Check if compressed frame fits in our buffer
    const uint32_t COMPRESSED_BUFFER_SIZE = 100 * 1024;
    if (frameEntry.size > COMPRESSED_BUFFER_SIZE) {
        return false;
    }
    
    // Read entire compressed frame
    size_t readSize;
    bool readSuccess = sdReader->readSequentialInto(
        compressedBuffer, 
        readSize, 
        frameEntry.size, 
        frameEntry.offset
    );
    
    if (!readSuccess || readSize < frameEntry.size) {
        return false;
    }
    
    // Calculate number of segments needed
    uint32_t totalRows = header.frameHeight;
    uint32_t numSegments = (totalRows + rowsPerSegment - 1) / rowsPerSegment;
    
    // Process frame in segments
    for (uint32_t segment = 0; segment < numSegments; segment++) {
        uint32_t startRow = segment * rowsPerSegment;
        uint32_t endRow = min(startRow + rowsPerSegment, totalRows);
        uint32_t rowsInSegment = endRow - startRow;
        
        // Calculate pixel range for this segment
        uint32_t startPixel = startRow * header.frameWidth;
        uint32_t pixelCount = rowsInSegment * header.frameWidth;
        
        // Decompress segment
        uint32_t decompressedPixels = RLEDecoder::decodeSegment(
            compressedBuffer,
            frameEntry.size,
            segmentBuffer,
            startPixel,
            pixelCount
        );
        
        if (decompressedPixels != pixelCount) {
            return false;
        }
        
        // Byte swap the segment data in-place
        uint32_t i = 0;
        
        // Process 8 pixels at a time
        for (; i + 7 < pixelCount; i += 8) {
            segmentBuffer[i]     = __builtin_bswap16(segmentBuffer[i]);
            segmentBuffer[i + 1] = __builtin_bswap16(segmentBuffer[i + 1]);
            segmentBuffer[i + 2] = __builtin_bswap16(segmentBuffer[i + 2]);
            segmentBuffer[i + 3] = __builtin_bswap16(segmentBuffer[i + 3]);
            segmentBuffer[i + 4] = __builtin_bswap16(segmentBuffer[i + 4]);
            segmentBuffer[i + 5] = __builtin_bswap16(segmentBuffer[i + 5]);
            segmentBuffer[i + 6] = __builtin_bswap16(segmentBuffer[i + 6]);
            segmentBuffer[i + 7] = __builtin_bswap16(segmentBuffer[i + 7]);
        }
        
        // Handle remaining pixels
        for (; i < pixelCount; i++) {
            segmentBuffer[i] = __builtin_bswap16(segmentBuffer[i]);
        }
        
        // Draw segment to display
        displayManager->drawFrameBuffer(
            segmentBuffer, 
            header.frameWidth, 
            rowsInSegment,
            x, 
            y + startRow
        );
    }
    
    return true;
}
