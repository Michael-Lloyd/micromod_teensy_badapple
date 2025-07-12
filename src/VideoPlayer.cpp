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
    Serial.println("VideoPlayer: Opening video file");
    
    // Read header
    size_t headerSize;
    uint8_t* headerData = sdReader->readPartialFile(videoPath, headerSize, sizeof(VideoHeader), 0);
    
    if (!headerData || headerSize < sizeof(VideoHeader)) {
        Serial.println("VideoPlayer: Failed to read video header");
        if (headerData) delete[] headerData;
        return false;
    }
    
    // Copy header data
    memcpy(&header, headerData, sizeof(VideoHeader));
    delete[] headerData;
    
    // Validate header
    if (memcmp(header.magic, "VID0", 4) != 0) {
        Serial.println("VideoPlayer: Invalid video format (not VID0)");
        return false;
    }
    
    Serial.print("VideoPlayer: Video loaded - ");
    Serial.print(header.frameWidth);
    Serial.print("x");
    Serial.print(header.frameHeight);
    Serial.print(" @ ");
    Serial.print(header.fps);
    Serial.print(" FPS, ");
    Serial.print(header.frameCount);
    Serial.println(" frames");
    
    // Check compression type
    if (header.compression != 1) {
        Serial.print("VideoPlayer: Unsupported compression type: ");
        Serial.println(header.compression);
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
        Serial.println("VideoPlayer: Failed to allocate compressed buffer");
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
    
    Serial.print("Buffer configuration: ");
    Serial.print(COMPRESSED_BUFFER_SIZE / 1024);
    Serial.print("KB compressed, ");
    Serial.print(segmentSize * 2 / 1024);
    Serial.println("KB decompressed segment");
    
    Serial.print("Segment: ");
    Serial.print(rowsPerSegment);
    Serial.print(" rows, ");
    Serial.print(segmentSize);
    Serial.println(" pixels");
    
    // Allocate segment buffer
    segmentBuffer = new uint16_t[segmentSize];
    if (!segmentBuffer) {
        Serial.println("VideoPlayer: Failed to allocate segment buffer");
        cleanupBuffers();
        return false;
    }
    
    // Allocate frame index cache
    frameIndexCache = new FrameIndexEntry[INDEX_CACHE_FRAMES];
    if (!frameIndexCache) {
        Serial.println("VideoPlayer: Failed to allocate index cache");
        cleanupBuffers();
        return false;
    }
    
    // Report memory usage
    uint32_t compressedBufferBytes = COMPRESSED_BUFFER_SIZE;
    uint32_t segmentBufferBytes = segmentSize * sizeof(uint16_t);
    uint32_t indexCacheBytes = INDEX_CACHE_FRAMES * sizeof(FrameIndexEntry);
    uint32_t totalBytes = compressedBufferBytes + segmentBufferBytes + indexCacheBytes;
    
    Serial.println("\n=== MEMORY USAGE REPORT ===");
    Serial.print("Compressed buffer: ");
    Serial.print(compressedBufferBytes);
    Serial.print(" bytes (");
    Serial.print(compressedBufferBytes / 1024.0);
    Serial.println(" KB)");
    
    Serial.print("Segment buffer: ");
    Serial.print(segmentBufferBytes);
    Serial.print(" bytes (");
    Serial.print(segmentBufferBytes / 1024.0);
    Serial.println(" KB)");
    
    Serial.print("Index cache: ");
    Serial.print(indexCacheBytes);
    Serial.print(" bytes (");
    Serial.print(indexCacheBytes / 1024.0);
    Serial.println(" KB)");
    
    Serial.print("Total VideoPlayer memory: ");
    Serial.print(totalBytes);
    Serial.print(" bytes (");
    Serial.print(totalBytes / 1024.0);
    Serial.println(" KB)");
    Serial.println("===========================\n");
    
    // Load first batch of frame indices
    loadIndexCache(0);
    
    // Open the video file for sequential reading
    if (!sdReader->openFile(videoPath)) {
        Serial.println("VideoPlayer: Failed to open file for sequential reading");
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

bool VideoPlayer::playFrameSegmented(uint32_t frameNumber, uint16_t x, uint16_t y, SegmentMetrics* metrics) {
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
        Serial.print("Frame ");
        Serial.print(frameNumber);
        Serial.print(" compressed size (");
        Serial.print(frameEntry.size);
        Serial.println(" bytes) exceeds buffer!");
        return false;
    }
    
    // Read entire compressed frame
    uint32_t readStart = micros();
    size_t readSize;
    bool readSuccess = sdReader->readSequentialInto(
        compressedBuffer, 
        readSize, 
        frameEntry.size, 
        frameEntry.offset
    );
    
    if (!readSuccess || readSize < frameEntry.size) {
        Serial.println("Failed to read compressed frame data");
        return false;
    }
    
    if (metrics) {
        metrics->readTime = micros() - readStart;
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
        uint32_t decompressStart = micros();
        uint32_t decompressedPixels = RLEDecoder::decodeSegment(
            compressedBuffer,
            frameEntry.size,
            segmentBuffer,
            startPixel,
            pixelCount
        );
        
        if (decompressedPixels != pixelCount) {
            Serial.print("Decompression error: expected ");
            Serial.print(pixelCount);
            Serial.print(" pixels, got ");
            Serial.println(decompressedPixels);
            return false;
        }
        
        if (metrics) {
            metrics->swapTime += micros() - decompressStart;  // Using swapTime for decompression time
        }
        
        // Byte swap the segment data in-place
        uint32_t swapStart = micros();
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
        
        if (metrics) {
            metrics->swapTime += micros() - swapStart;
        }
        
        // Draw segment to display
        uint32_t drawStart = micros();
        displayManager->drawFrameBuffer(
            segmentBuffer, 
            header.frameWidth, 
            rowsInSegment,
            x, 
            y + startRow
        );
        
        if (metrics) {
            metrics->drawTime += micros() - drawStart;
        }
    }
    
    return true;
}