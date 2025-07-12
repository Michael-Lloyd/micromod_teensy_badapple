#ifndef SDFILEREADER_H
#define SDFILEREADER_H

#include <Arduino.h>
#include <SdFat.h>

class SDFileReader {
public:
    SdFs sd;
    
private:
    uint8_t chipSelectPin;
    bool sdInitialized;
    FsFile currentFile;
    
public:
    SDFileReader(uint8_t csPin);
    ~SDFileReader();
    
    bool begin();
    
    void end();
    
    uint8_t* readPartialFile(const char* filename, size_t& bytesRead, size_t bytesToRead, size_t offset = 0);
    
    bool openFile(const char* filename);
    void closeFile();
    uint8_t* readSequential(size_t& bytesRead, size_t bytesToRead, size_t offset);
    bool readSequentialInto(uint8_t* buffer, size_t& bytesRead, size_t bytesToRead, size_t offset);
    
    void printTimingSummary();
};

#endif
