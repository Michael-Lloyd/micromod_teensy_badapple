#ifndef SDFILEREADER_H
#define SDFILEREADER_H

#include <Arduino.h>
#include <SdFat.h>

class SDFileReader {
public:
    SdFs sd;  // Public for optimized access
    
private:
    uint8_t chipSelectPin;
    bool sdInitialized;
    FsFile currentFile;  // Keep file open for sequential reads
    
public:
    SDFileReader(uint8_t csPin);
    ~SDFileReader();
    
    // Initialize SD card once
    bool begin();
    
    // End SD card access
    void end();
    
    // Read partial file without reinitializing SD
    uint8_t* readPartialFile(const char* filename, size_t& bytesRead, size_t bytesToRead, size_t offset = 0);
    
    // Optimized sequential file reading
    bool openFile(const char* filename);
    void closeFile();
    uint8_t* readSequential(size_t& bytesRead, size_t bytesToRead, size_t offset);
    bool readSequentialInto(uint8_t* buffer, size_t& bytesRead, size_t bytesToRead, size_t offset);
    
    // Print timing summary
    void printTimingSummary();
};

#endif // SDFILEREADER_H
