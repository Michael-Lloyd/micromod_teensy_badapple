#include "SDFileReader.h"
// TIMING_INSTRUMENTATION: Added for SD card performance analysis
#include <elapsedMillis.h>

// TIMING_INSTRUMENTATION: Track cumulative stats
struct SDTimingStats {
    uint32_t totalReads = 0;
    uint32_t totalBytes = 0;
    uint32_t totalReadTime = 0;
    uint32_t totalSeekTime = 0;
    uint32_t minReadTime = 999999;
    uint32_t maxReadTime = 0;
    uint32_t minSeekTime = 999999;
    uint32_t maxSeekTime = 0;
} sdStats;

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
    
    Serial.println("Initializing SD card...");
    // TIMING_INSTRUMENTATION: Track SD initialization time
    elapsedMicros initTime = 0;
    
    // Ensure CS pin is high before init
    digitalWrite(chipSelectPin, HIGH);
    delay(10);
    
    // Initialize SPI
    SPI.begin();
    
    // Configure SPI for SD card - use 60MHz for balance of speed and reliability
    // Some SD cards can't handle 80MHz reliably
    SdSpiConfig spiConfig(chipSelectPin, SHARED_SPI, SD_SCK_MHZ(60));
    
    if (!sd.begin(spiConfig)) {
        Serial.println("SD card initialization failed!");
        return false;
    }
    // TIMING_INSTRUMENTATION: Report init time
    Serial.print("[TIMING] SD init took: ");
    Serial.print(initTime);
    Serial.println(" us");
    
    Serial.println("SD card initialized successfully!");
    sdInitialized = true;
    return true;
}

void SDFileReader::end() {
    if (!sdInitialized) {
        return;
    }
    
    Serial.println("Ending SD card access...");
    
    // Ensure CS pin is high
    digitalWrite(chipSelectPin, HIGH);
    
    sdInitialized = false;
}

uint8_t* SDFileReader::readPartialFile(const char* filename, size_t& bytesRead, size_t bytesToRead, size_t offset) {
    bytesRead = 0;
    
    if (!sdInitialized) {
        Serial.println("SD card not initialized!");
        return nullptr;
    }
    
    // Open file
    FsFile file = sd.open(filename, O_RDONLY);
    if (!file) {
        Serial.print("Failed to open file: ");
        Serial.println(filename);
        return nullptr;
    }
    
    // Get file size
    size_t totalFileSize = file.fileSize();
    
    // Validate offset
    if (offset >= totalFileSize) {
        Serial.println("Offset beyond file size");
        file.close();
        return nullptr;
    }
    
    // Calculate actual bytes to read
    size_t actualBytesToRead = min(bytesToRead, totalFileSize - offset);
    
    // Allocate buffer
    uint8_t* buffer = new uint8_t[actualBytesToRead];
    if (!buffer) {
        Serial.println("Failed to allocate memory");
        file.close();
        return nullptr;
    }
    
    // Seek to offset
    if (!file.seekSet(offset)) {
        Serial.println("Failed to seek to offset");
        delete[] buffer;
        file.close();
        return nullptr;
    }
    
    // Read data
    bytesRead = file.read(buffer, actualBytesToRead);
    
    if (bytesRead != actualBytesToRead) {
        Serial.println("Failed to read expected bytes");
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
        Serial.println("SD card not initialized!");
        return false;
    }
    
    // TIMING_INSTRUMENTATION: Track file open time
    elapsedMicros openTime = 0;
    
    // Close any previously open file
    if (currentFile) {
        currentFile.close();
    }
    
    currentFile = sd.open(filename, O_RDONLY);
    
    // TIMING_INSTRUMENTATION: Report open time
    Serial.print("[TIMING] Opening file '");
    Serial.print(filename);
    Serial.print("' took: ");
    Serial.print((unsigned long)openTime);
    Serial.println(" us");
    
    if (!currentFile) {
        Serial.print("Failed to open file: ");
        Serial.println(filename);
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
        Serial.println("No file open!");
        return nullptr;
    }
    
    // Allocate buffer
    uint8_t* buffer = new uint8_t[bytesToRead];
    if (!buffer) {
        Serial.println("Failed to allocate memory");
        return nullptr;
    }
    
    // Seek to offset
    if (!currentFile.seekSet(offset)) {
        Serial.println("Failed to seek to offset");
        delete[] buffer;
        return nullptr;
    }
    
    // Read data
    bytesRead = currentFile.read(buffer, bytesToRead);
    
    if (bytesRead != bytesToRead) {
        Serial.println("Failed to read expected bytes");
        delete[] buffer;
        return nullptr;
    }
    
    return buffer;
}
bool SDFileReader::readSequentialInto(uint8_t* buffer, size_t& bytesRead, size_t bytesToRead, size_t offset) {
    bytesRead = 0;
    
    if (!currentFile) {
        Serial.println("No file open!");
        return false;
    }
    
    if (!buffer) {
        Serial.println("Buffer is NULL!");
        return false;
    }
    
    // TIMING_INSTRUMENTATION: Track total function time
    elapsedMicros totalTime = 0;
    elapsedMicros seekTime = 0;
    
    // Seek to offset
    if (!currentFile.seekSet(offset)) {
        Serial.println("Failed to seek to offset");
        return false;
    }
    // TIMING_INSTRUMENTATION: Report seek time
    unsigned long seekElapsed = seekTime;
    sdStats.totalSeekTime += seekElapsed;
    sdStats.minSeekTime = min(sdStats.minSeekTime, seekElapsed);
    sdStats.maxSeekTime = max(sdStats.maxSeekTime, seekElapsed);
    if (seekElapsed > 10) { // Only report significant seeks
        Serial.print("[TIMING] Seek to ");
        Serial.print(offset);
        Serial.print(" took: ");
        Serial.print(seekElapsed);
        Serial.println(" us");
    }
    
    // Read data directly into provided buffer
    // TIMING_INSTRUMENTATION: Track read time
    elapsedMicros readTime = 0;
    bytesRead = currentFile.read(buffer, bytesToRead);
    unsigned long readElapsed = readTime;
    
    // TIMING_INSTRUMENTATION: Update stats
    sdStats.totalReads++;
    sdStats.totalBytes += bytesToRead;
    sdStats.totalReadTime += readElapsed;
    sdStats.minReadTime = min(sdStats.minReadTime, readElapsed);
    sdStats.maxReadTime = max(sdStats.maxReadTime, readElapsed);
    
    // Report every 100 reads for less spam
    if (sdStats.totalReads % 100 == 0) {
        Serial.print("[TIMING] Read #");
        Serial.print(sdStats.totalReads);
        Serial.print(": ");
        Serial.print(bytesToRead);
        Serial.print(" bytes in ");
        Serial.print(readElapsed);
        Serial.print(" us (");
        if (readElapsed > 0) {
            float mbytes_per_sec = (bytesToRead / 1024.0f / 1024.0f) / (readElapsed / 1000000.0f);
            Serial.print(mbytes_per_sec, 2);
            Serial.print(" MB/s");
        }
        Serial.println(")");
    }
    
    if (bytesRead != bytesToRead) {
        Serial.println("Failed to read expected bytes");
        return false;
    }
    
    // TIMING_INSTRUMENTATION: Only report total if it's significantly different from read time
    if ((unsigned long)totalTime > readElapsed + 100) {
        Serial.print("[TIMING] Total readSequentialInto took: ");
        Serial.print((unsigned long)totalTime);
        Serial.print(" us (overhead: ");
        Serial.print((unsigned long)totalTime - readElapsed);
        Serial.println(" us)");
    }
    
    return true;
}

// TIMING_INSTRUMENTATION: Function to print timing summary
void SDFileReader::printTimingSummary() {
    if (sdStats.totalReads == 0) return;
    
    Serial.println("\n\n=== SD CARD TIMING SUMMARY ===");
    Serial.print("Total reads: ");
    Serial.println(sdStats.totalReads);
    Serial.print("Total bytes read: ");
    Serial.print(sdStats.totalBytes);
    Serial.print(" (");
    Serial.print(sdStats.totalBytes / 1024.0f / 1024.0f);
    Serial.println(" MB)");
    
    Serial.println("\nRead Performance:");
    Serial.print("  Total time: ");
    Serial.print(sdStats.totalReadTime / 1000);
    Serial.print(" ms (");
    Serial.print(sdStats.totalReadTime);
    Serial.println(" us)");
    Serial.print("  Average read time: ");
    Serial.print(sdStats.totalReadTime / sdStats.totalReads);
    Serial.println(" us");
    Serial.print("  Min read time: ");
    Serial.print(sdStats.minReadTime);
    Serial.println(" us");
    Serial.print("  Max read time: ");
    Serial.print(sdStats.maxReadTime);
    Serial.println(" us");
    
    Serial.print("  Average throughput: ");
    float avgMBps = (sdStats.totalBytes / 1024.0f / 1024.0f) / (sdStats.totalReadTime / 1000000.0f);
    Serial.print(avgMBps, 2);
    Serial.println(" MB/s");
    
    Serial.println("\nSeek Performance:");
    Serial.print("  Total seek time: ");
    Serial.print(sdStats.totalSeekTime);
    Serial.println(" us");
    Serial.print("  Average seek time: ");
    Serial.print(sdStats.totalSeekTime / sdStats.totalReads);
    Serial.println(" us");
    Serial.print("  Min seek time: ");
    Serial.print(sdStats.minSeekTime);
    Serial.println(" us");
    Serial.print("  Max seek time: ");
    Serial.print(sdStats.maxSeekTime);
    Serial.println(" us");
    
    Serial.println("==============================\n");
}