#!/usr/bin/env python3
"""
Convert video files to custom RGB565 format with RLE compression for Teensy display
Usage: python video_converter.py input.mp4 output.vid
"""

import cv2
import numpy as np
import struct
import sys
from pathlib import Path

def rgb888_to_rgb565(r, g, b):
    """Convert 8-bit RGB to 16-bit RGB565"""
    # Convert numpy types to Python int to avoid overflow issues
    r = int(r)
    g = int(g)
    b = int(b)
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def compress_frame_rle(frame_data):
    """
    Compress frame data using RLE (Run-Length Encoding)
    
    Format:
    - Header byte: bit 7 = run flag (1=run, 0=literal), bits 6-0 = count (1-128)
    - Run: header byte followed by 2 bytes (RGB565 value)
    - Literal: header byte followed by count * 2 bytes (RGB565 values)
    
    Returns: compressed bytes
    """
    compressed = bytearray()
    i = 0
    
    while i < len(frame_data):
        # Check for runs
        run_start = i
        run_value = frame_data[i]
        run_length = 1
        
        # Count consecutive identical values (max 128)
        while i + run_length < len(frame_data) and run_length < 128:
            if frame_data[i + run_length] == run_value:
                run_length += 1
            else:
                break
        
        # Decide whether to encode as run or literal
        if run_length >= 3:  # Use run encoding for 3+ identical values
            # Run encoding: set bit 7, store count-1 in bits 6-0
            header = 0x80 | (run_length - 1)
            compressed.append(header)
            compressed.extend(struct.pack('<H', run_value))
            i += run_length
        else:
            # Find length of literal sequence (different consecutive values)
            literal_start = i
            literal_length = 0
            
            while i < len(frame_data) and literal_length < 128:
                # Look ahead to see if a run is coming
                if i + 2 < len(frame_data) and frame_data[i] == frame_data[i + 1] == frame_data[i + 2]:
                    break
                literal_length += 1
                i += 1
            
            # Literal encoding: clear bit 7, store count-1 in bits 6-0
            header = (literal_length - 1) & 0x7F
            compressed.append(header)
            for j in range(literal_start, literal_start + literal_length):
                compressed.extend(struct.pack('<H', frame_data[j]))
    
    return bytes(compressed)

def convert_video(input_path, output_path, target_width=240):
    # Open video
    cap = cv2.VideoCapture(input_path)
    fps = int(cap.get(cv2.CAP_PROP_FPS))
    frame_count = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    original_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    original_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    
    # Uncomment to test with limited frames
    # frame_count = min(10, frame_count)
    # print("TESTING MODE: Only converting first 10 frames")
    
    # Calculate target height to maintain aspect ratio
    aspect_ratio = original_height / original_width
    target_height = int(target_width * aspect_ratio)
    
    print(f"Input video: {frame_count} frames at {fps} FPS")
    print(f"Original resolution: {original_width}x{original_height}")
    print(f"Output format: {target_width}x{target_height} RGB565 (aspect ratio preserved)")
    print(f"Compression: RLE")
    
    with open(output_path, 'wb') as f:
        # Write header (will update index offset later)
        # Header with RLE compression always enabled
        header = struct.pack('<4sIHHBBBBI', 
                           b'VID0',           # magic
                           frame_count,       # frame count
                           target_width,      # width
                           target_height,     # height
                           fps,              # fps
                           1,                # reserved[0] = compression type (1=RLE)
                           0, 0,             # reserved[1], reserved[2]
                           0)                # index offset (placeholder)
        f.write(header)
        
        # Write frame index table (placeholders)
        index_offset = f.tell()
        frame_offsets = []
        frame_sizes = []  # Track compressed frame sizes
        f.write(b'\x00' * (frame_count * 8))  # 4 bytes offset + 4 bytes size per frame
        
        # Process each frame
        total_uncompressed = 0
        total_compressed = 0
        
        for i in range(frame_count):
            ret, frame = cap.read()
            if not ret:
                break
                
            # Resize frame
            frame = cv2.resize(frame, (target_width, target_height))
            
            # Convert BGR to RGB
            frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            
            # Convert to RGB565
            frame_data = []
            for y in range(target_height):
                for x in range(target_width):
                    r, g, b = frame[y, x]
                    rgb565 = rgb888_to_rgb565(r, g, b)
                    frame_data.append(rgb565)
            
            # Track uncompressed size
            uncompressed_size = len(frame_data) * 2
            total_uncompressed += uncompressed_size
            
            # Record frame offset BEFORE writing
            frame_offsets.append(f.tell())
            
            # Compress and write
            compressed_data = compress_frame_rle(frame_data)
            f.write(compressed_data)
            frame_sizes.append(len(compressed_data))
            total_compressed += len(compressed_data)
            compression_ratio = (1 - len(compressed_data) / uncompressed_size) * 100
            print(f"Frame {i+1}/{frame_count}: {uncompressed_size} -> {len(compressed_data)} bytes ({compression_ratio:.1f}% reduction)", end='\r')
        
        # Update header with index offset
        current_pos = f.tell()
        f.seek(16)  # Seek to index offset field (at byte 16)
        f.write(struct.pack('<I', index_offset))
        f.seek(current_pos)  # Return to end of file
        
        # Write actual frame offsets and sizes
        f.seek(index_offset)
        for offset, size in zip(frame_offsets, frame_sizes):
            f.write(struct.pack('<II', offset, size))
    
    cap.release()
    print(f"\n\nConverted {len(frame_offsets)} frames to {output_path}")
    print(f"Output file size: {Path(output_path).stat().st_size / 1024 / 1024:.1f} MB")
    
    compression_ratio = (1 - total_compressed / total_uncompressed) * 100
    print(f"Total compression ratio: {compression_ratio:.1f}%")
    print(f"Uncompressed size would be: {total_uncompressed / 1024 / 1024:.1f} MB")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python video_converter.py input.mp4 output.vid")
        sys.exit(1)
    
    convert_video(sys.argv[1], sys.argv[2])