#ifndef RLE_DECODER_H
#define RLE_DECODER_H

#include <Arduino.h>

class RLEDecoder {
public:
    // Decode RLE compressed data into output buffer
    // Returns number of pixels decoded
    static uint32_t decode(const uint8_t* compressed, uint32_t compressedSize, uint16_t* output, uint32_t maxPixels);
    
    // Decode a segment of RLE data starting from a specific output position
    // This allows decoding frames in segments when memory is limited
    static uint32_t decodeSegment(
        const uint8_t* compressed, 
        uint32_t compressedSize, 
        uint16_t* output, 
        uint32_t startPixel, 
        uint32_t pixelCount
    );
    
    // Get the size of decompressed data without actually decompressing
    // Useful for memory planning
    static uint32_t getDecompressedSize(const uint8_t* compressed, uint32_t compressedSize);
    
private:
    // Helper to find the compressed data position for a given output pixel
    static uint32_t findCompressedPosition(const uint8_t* compressed, uint32_t compressedSize, uint32_t targetPixel);
};

#endif // RLE_DECODER_H