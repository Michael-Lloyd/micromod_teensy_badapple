#include "RLEDecoder.h"

uint32_t RLEDecoder::decode(const uint8_t* compressed, uint32_t compressedSize, uint16_t* output, uint32_t maxPixels) {
    uint32_t inPos = 0;
    uint32_t outPos = 0;
    
    while (inPos < compressedSize && outPos < maxPixels) {
        uint8_t header = compressed[inPos++];
        uint8_t count = (header & 0x7F) + 1;
        
        if (header & 0x80) {
            if (inPos + 1 >= compressedSize) break;
            
            uint16_t value = compressed[inPos] | (compressed[inPos + 1] << 8);
            inPos += 2;
            
            uint32_t pixelsToWrite = min((uint32_t)count, maxPixels - outPos);
            for (uint32_t i = 0; i < pixelsToWrite; i++) {
                output[outPos++] = value;
            }
        } else {
            uint32_t pixelsToWrite = min((uint32_t)count, maxPixels - outPos);
            
            for (uint32_t i = 0; i < pixelsToWrite; i++) {
                if (inPos + 1 >= compressedSize) break;
                output[outPos++] = compressed[inPos] | (compressed[inPos + 1] << 8);
                inPos += 2;
            }
            
            if (pixelsToWrite < count) {
                inPos += (count - pixelsToWrite) * 2;
            }
        }
    }
    
    return outPos;
}

uint32_t RLEDecoder::decodeSegment(
    const uint8_t* compressed, 
    uint32_t compressedSize, 
    uint16_t* output, 
    uint32_t startPixel, 
    uint32_t pixelCount
) {
    uint32_t inPos = findCompressedPosition(compressed, compressedSize, startPixel);
    if (inPos >= compressedSize) return 0;
    
    uint32_t currentPixel = 0;
    uint32_t outPos = 0;
    uint32_t endPixel = startPixel + pixelCount;
    
    uint32_t scanPos = 0;
    while (scanPos < inPos && scanPos < compressedSize) {
        uint8_t header = compressed[scanPos++];
        uint8_t count = (header & 0x7F) + 1;
        
        if (header & 0x80) {
            currentPixel += count;
            scanPos += 2;
        } else {
            currentPixel += count;
            scanPos += count * 2;
        }
    }
    
    while (inPos < compressedSize && outPos < pixelCount) {
        uint8_t header = compressed[inPos];
        uint8_t count = (header & 0x7F) + 1;
        
        if (header & 0x80) {
            inPos++;
            if (inPos + 1 >= compressedSize) break;
            
            uint16_t value = compressed[inPos] | (compressed[inPos + 1] << 8);
            inPos += 2;
            
            uint32_t runStart = currentPixel;
            uint32_t runEnd = currentPixel + count;
            
            if (runEnd > startPixel && runStart < endPixel) {
                uint32_t copyStart = max(runStart, startPixel);
                uint32_t copyEnd = min(runEnd, endPixel);
                uint32_t copyCount = copyEnd - copyStart;
                
                for (uint32_t i = 0; i < copyCount; i++) {
                    output[outPos++] = value;
                }
            }
            
            currentPixel += count;
        } else {
            inPos++;
            
            for (uint8_t i = 0; i < count; i++) {
                if (inPos + 1 >= compressedSize) break;
                
                if (currentPixel >= startPixel && currentPixel < endPixel) {
                    output[outPos++] = compressed[inPos] | (compressed[inPos + 1] << 8);
                }
                
                inPos += 2;
                currentPixel++;
                
                if (currentPixel >= endPixel) break;
            }
        }
        
        if (currentPixel >= endPixel) break;
    }
    
    return outPos;
}

uint32_t RLEDecoder::getDecompressedSize(const uint8_t* compressed, uint32_t compressedSize) {
    uint32_t inPos = 0;
    uint32_t pixelCount = 0;
    
    while (inPos < compressedSize) {
        uint8_t header = compressed[inPos++];
        uint8_t count = (header & 0x7F) + 1;
        
        pixelCount += count;
        
        if (header & 0x80) {
            inPos += 2;
        } else {
            inPos += count * 2;
        }
    }
    
    return pixelCount;
}

uint32_t RLEDecoder::findCompressedPosition(const uint8_t* compressed, uint32_t compressedSize, uint32_t targetPixel) {
    uint32_t inPos = 0;
    uint32_t currentPixel = 0;
    
    while (inPos < compressedSize && currentPixel < targetPixel) {
        uint8_t header = compressed[inPos];
        uint8_t count = (header & 0x7F) + 1;
        
        if (currentPixel + count > targetPixel) {
            return inPos;
        }
        
        inPos++;
        currentPixel += count;
        
        if (header & 0x80) {
            inPos += 2;
        } else {
            inPos += count * 2;
        }
    }
    
    return inPos;
}