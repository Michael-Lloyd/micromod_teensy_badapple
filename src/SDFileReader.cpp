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
    
    digitalWrite(chipSelectPin, HIGH);
    delay(10);
    
    SPI.begin();
    
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
    
    digitalWrite(chipSelectPin, HIGH);
    
    sdInitialized = false;
}

uint8_t* SDFileReader::readPartialFile(const char* filename, size_t& bytesRead, size_t bytesToRead, size_t offset) {
    bytesRead = 0;
    
    if (!sdInitialized) {
        return nullptr;
    }
    
    FsFile file = sd.open(filename, O_RDONLY);
    if (!file) {
        return nullptr;
    }
    
    size_t totalFileSize = file.fileSize();
    
    if (offset >= totalFileSize) {
        file.close();
        return nullptr;
    }
    
    size_t actualBytesToRead = min(bytesToRead, totalFileSize - offset);
    
    uint8_t* buffer = new uint8_t[actualBytesToRead];
    if (!buffer) {
        file.close();
        return nullptr;
    }
    
    if (!file.seekSet(offset)) {
        delete[] buffer;
        file.close();
        return nullptr;
    }
    
    bytesRead = file.read(buffer, actualBytesToRead);
    
    if (bytesRead != actualBytesToRead) {
        delete[] buffer;
        file.close();
        return nullptr;
    }
    
    file.close();
    
    return buffer;
}

bool SDFileReader::openFile(const char* filename) {
    if (!sdInitialized) {
        return false;
    }
    
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
    
    uint8_t* buffer = new uint8_t[bytesToRead];
    if (!buffer) {
        return nullptr;
    }
    
    if (!currentFile.seekSet(offset)) {
        delete[] buffer;
        return nullptr;
    }
    
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
    
    if (!currentFile.seekSet(offset)) {
        return false;
    }
    
    bytesRead = currentFile.read(buffer, bytesToRead);
    
    if (bytesRead != bytesToRead) {
        return false;
    }
    
    return true;
}

void SDFileReader::printTimingSummary() {
}