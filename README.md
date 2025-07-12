This project plays the Bad Apple video on a SparkFun MicroMod display board using a
Teensy MicroMod processor. The video is stored on an SD card in a custom RLE-compressed 
format and played back at 30 FPS.

## Hardware Requirements

- SparkFun MicroMod Teensy Processor
- SparkFun MicroMod Input and Display Carrier Board 
- MicroSD card

## Software Dependencies

This project uses PlatformIO with the following libraries:

- **Hyperdisplay** - Base display library
- **HyperDisplay_4DLCD-320240** - Driver for the 320x240 display (provided in `lib/`)
- **SdFat** - SD card library

## Setup Instructions

1. **Install PlatformIO**

2. **Clone the repository, and libraries**
   - The HyperDisplay library can be found on SparkFun's github page 
   - The specific display library is in `lib/` 
   - SdFat can be found on github (use 2.0.0 or later) 

3. **Prepare the video file**
   - Convert your (mp4) video to the custom RLE format using the provided Python script:
   ```bash
   cd vid
   python video_converter.py input_video.mp4 bad_apple_rle.vid
   ```
   - Copy `bad_apple_rle.vid` to the root of your SD card

4. **Build and upload**
   - Connect your MicroMod board via USB
   - Build and upload using PlatformIO

5. **Run the video**
   - Insert the SD card with the video file
   - Power on or reset the board
   - The video will start playing automatically

## Video Format

The project uses a custom VID0 format with RLE compression optimized for embedded systems:
- RGB565 color format
- Run-length encoding compression
- Frame index for fast seeking
- See `vid/SPECIFICATION.md` for detailed format documentation

## Performance

The video player achieves stable 30 FPS playback with:
- Segmented frame loading to manage memory
- Optimized SPI switching between SD card and display
- Hardware-accelerated display updates
- Real-time performance metrics via serial output

## Serial Debug Output

Connect to the serial port at 115200 baud to see:
- Video properties (resolution, FPS, frame count)
- Playback progress
- Performance metrics and timing information
- Any errors during playback
