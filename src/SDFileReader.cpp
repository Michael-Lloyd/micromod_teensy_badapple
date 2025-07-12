#include "SDFileReader.h"

SDFileReader::SDFileReader(uint8_t csPin) : chipSelectPin(csPin), sdInitialized(false) {
    pinMode(chipSelectPin, OUTPUT);
    digitalWrite(chipSelectPin, HIGH);
}

SDFileReader::~SDFileReader() {
    if (sdInitialized) {
        end();
    }
}

bool SDFileReader::begin() {
    if (sdInitialized) {
        return true;
    }
    
    // Ensure CS pin is high before init
    digitalWrite(chipSelectPin, HIGH);
    delay(10);
    
    // Initialize SPI
    SPI.begin();
    
    // Configure SPI for SD card - use 60MHz for balance of speed and reliability
    // Some SD cards can't handle 80MHz reliably
    SdSpiConfig spiConfig(chipSelectPin, SHARED_SPI, SD_SCK_MHZ(60));
    
    if (!sd.begin(spiConfig)) {
        return false;
    }
    
    sdInitialized = true;
    return true;
}

void SDFileReader::end() {
    if (!sdInitialized) {
        return;
    }
    
    // Ensure CS pin is high
    digitalWrite(chipSelectPin, HIGH);
    
    sdInitialized = false;
}

uint8_t* SDFileReader::readPartialFile(const char* filename, size_t& bytesRead, size_t bytesToRead, size_t offset) {
    bytesRead = 0;
    
    if (!sdInitialized) {
        return nullptr;
    }
    
    // Open file
    FsFile file = sd.open(filename, O_RDONLY);
    if (!file) {
        return nullptr;
    }
    
    // Get file size
    size_t totalFileSize = file.fileSize();
    
    // Validate offset
    if (offset >= totalFileSize) {
        file.close();
        return nullptr;
    }
    
    // Calculate actual bytes to read
    size_t actualBytesToRead = min(bytesToRead, totalFileSize - offset);
    
    // Allocate buffer
    uint8_t* buffer = new uint8_t[actualBytesToRead];
    if (!buffer) {
        file.close();
        return nullptr;
    }
    
    // Seek to offset
    if (!file.seekSet(offset)) {
        delete[] buffer;
        file.close();
        return nullptr;
    }
    
    // Read data
    bytesRead = file.read(buffer, actualBytesToRead);
    
    if (bytesRead != actualBytesToRead) {
        delete[] buffer;
        file.close();
        return nullptr;
    }
    
    // Close file (but keep SD initialized)
    file.close();
    
    return buffer;
}

bool SDFileReader::openFile(const char* filename) {
    if (!sdInitialized) {
        return false;
    }
    
    // Close any previously open file
    if (currentFile) {
        currentFile.close();
    }
    
    currentFile = sd.open(filename, O_RDONLY);
    
    if (!currentFile) {
        return false;
    }
    
    return true;
}

void SDFileReader::closeFile() {
    if (currentFile) {
        currentFile.close();
    }
}

uint8_t* SDFileReader::readSequential(size_t& bytesRead, size_t bytesToRead, size_t offset) {
    bytesRead = 0;
    
    if (!currentFile) {
        return nullptr;
    }
    
    // Allocate buffer
    uint8_t* buffer = new uint8_t[bytesToRead];
    if (!buffer) {
        return nullptr;
    }
    
    // Seek to offset
    if (!currentFile.seekSet(offset)) {
        delete[] buffer;
        return nullptr;
    }
    
    // Read data
    bytesRead = currentFile.read(buffer, bytesToRead);
    
    if (bytesRead != bytesToRead) {
        delete[] buffer;
        return nullptr;
    }
    
    return buffer;
}
bool SDFileReader::readSequentialInto(uint8_t* buffer, size_t& bytesRead, size_t bytesToRead, size_t offset) {
    bytesRead = 0;
    
    if (!currentFile) {
        return false;
    }
    
    if (!buffer) {
        return false;
    }
    
    // Seek to offset
    if (!currentFile.seekSet(offset)) {
        return false;
    }
    
    // Read data directly into provided buffer
    bytesRead = currentFile.read(buffer, bytesToRead);
    
    if (bytesRead != bytesToRead) {
        return false;
    }
    
    return true;
}

// Function to print timing summary
void SDFileReader::printTimingSummary() {
    // Empty implementation - no longer needed
}
