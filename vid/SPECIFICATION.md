# VID0 Video Format Specification

## Overview

The VID0 format is a compressed video format designed for embedded systems with limited processing 
power and direct RGB565 display interfaces. It uses RLE (Run-Length Encoding) compression to reduce 
file sizes while maintaining fast frame access and simple decompression.

## File Structure

The file consists of three sections:
1. **Header** (24 bytes)
2. **Frame Index Table** (variable size)
3. **Frame Data** (RLE compressed RGB565 pixel data)

## Header Format (24 bytes)

```c
struct VideoHeader {
    char magic[4];          // "VID0" - Format identifier
    uint32_t frameCount;    // Total number of frames in the video
    uint16_t frameWidth;    // Width of each frame in pixels
    uint16_t frameHeight;   // Height of each frame in pixels
    uint8_t fps;            // Frames per second
    uint8_t compression;    // Compression type: 1=RLE (reserved for future formats)
    uint8_t reserved[2];    // Reserved for future use (set to 0)
    uint32_t indexOffset;   // File offset to the frame index table
};
```

All multi-byte values are stored in **little-endian** format.

## Frame Index Table

Located at `indexOffset` bytes from the start of the file. Contains an array of frame entries:

```c
struct FrameIndexEntry {
    uint32_t offset;  // File offset to the start of frame data
    uint32_t size;    // Size of frame data in bytes (compressed size if RLE)
};

FrameIndexEntry frameIndex[frameCount];
```

Each entry provides both the offset and size of the frame data, allowing efficient reading of compressed frames.

## Frame Data

Frame data is compressed using Run-Length Encoding (RLE):

**RLE Format**:
- Each compressed sequence starts with a header byte
- **Header byte format**: bit 7 = run flag, bits 6-0 = count
  - If bit 7 = 1: Run of identical pixels (count+1 repetitions)
  - If bit 7 = 0: Literal sequence (count+1 different pixels)
- **Run encoding**: header byte + 2 bytes (RGB565 value to repeat)
- **Literal encoding**: header byte + (count+1) × 2 bytes (RGB565 values)

Example:
```
0x82 0x00 0xF8  // Run: repeat RGB565 value 0xF800 three times (count=2+1)
0x03 0x00 0xF8 0xE0 0x07 0x1F 0x00 0xFF 0xFF  // Literal: 4 different RGB565 values
```

### RGB565 Format
```
Bit:  15 14 13 12 11 | 10 9 8 7 6 5 | 4 3 2 1 0
      R  R  R  R  R  | G  G G G G G | B B B B B
```

## File Size Calculation

### RLE Compressed
```
File Size = Header (24 bytes) 
          + Index Table (frameCount × 8 bytes)
          + Compressed Frame Data (varies by content)
```

Example for 240x180 @ 30fps:
- Uncompressed size per frame: 86,400 bytes
- RLE compressed (typical animation): 10,000-30,000 bytes  
- Compression ratio: 65-88% reduction
- Per second: ~0.3-0.9 MB/s (content dependent)
