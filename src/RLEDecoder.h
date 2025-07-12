#ifndef RLE_DECODER_H
#define RLE_DECODER_H

#include <Arduino.h>

class RLEDecoder {
public:
    static uint32_t decode(const uint8_t* compressed, uint32_t compressedSize, uint16_t* output, uint32_t maxPixels);
    
    static uint32_t decodeSegment(
        const uint8_t* compressed, 
        uint32_t compressedSize, 
        uint16_t* output, 
        uint32_t startPixel, 
        uint32_t pixelCount
    );
    
    static uint32_t getDecompressedSize(const uint8_t* compressed, uint32_t compressedSize);
    
private:
    static uint32_t findCompressedPosition(const uint8_t* compressed, uint32_t compressedSize, uint32_t targetPixel);
};

#endif