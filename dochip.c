#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <wiringPi.h>
#include <string>
#include <cstring>
#include <stdexcept>

/* 
 * PHYSICAL PIN MAPPING (Banana Pi M2 Zero)
 * Physical Pin 15 -> wPi 3  (CS Jumper)
 * Physical Pin 19 -> wPi 12 (MOSI / DI)
 * Physical Pin 21 -> wPi 13 (MISO / DO)
 * Physical Pin 23 -> wPi 14 (CLK / SK)
 * 
 * Note: If you patched wiringPi, run `gpio readall` to verify 
 * these wPi numbers haven't changed!
 */
const int PIN_SK  = 14; 
const int PIN_DI  = 12; 
const int PIN_DO  = 13; 
const int PIN_CS  = 3;

class M93C56_GPIO {
public:
    M93C56_GPIO() {
        if (wiringPiSetup() == -1) {
            throw std::runtime_error("Failed to initialize wiringPi.");
        }
        pinMode(PIN_SK, OUTPUT);
        pinMode(PIN_DI, OUTPUT);
        pinMode(PIN_CS, OUTPUT);
        
        pinMode(PIN_DO, INPUT);
        // DIAGNOSTIC CHANGE: We enable the pull-up. If the chip is dead/disconnected, 
        // readByte will return 0xFF instead of 0x00.
        pullUpDnControl(PIN_DO, PUD_UP); 
        
        digitalWrite(PIN_SK, LOW);
        digitalWrite(PIN_CS, LOW); // CS idle
        delayMicroseconds(1000);   // Give the chip time to stabilize
    }

    void pulseClock() {
        digitalWrite(PIN_SK, HIGH);
        delayMicroseconds(2); // Reduced for much faster clock
        digitalWrite(PIN_SK, LOW);
        delayMicroseconds(2);
    }

    void sendBit(int bit) {
        digitalWrite(PIN_DI, bit ? HIGH : LOW);
        pulseClock();
    }

    int readBit() {
        digitalWrite(PIN_SK, HIGH);
        delayMicroseconds(2); // Wait for DO to stabilize on rising edge
        int bit = digitalRead(PIN_DO);
        digitalWrite(PIN_SK, LOW);
        delayMicroseconds(2);
        return bit;
    }

    uint8_t readByte(int address) {
        digitalWrite(PIN_CS, HIGH); 
        delayMicroseconds(2); // CS setup time
        
        sendBit(1); // Start bit
        sendBit(1); // Opcode 1
        sendBit(0); // Opcode 0 (Read)

        // M93C56 8-bit mode requires a 9-bit address (A8 down to A0).
        for (int i = 8; i >= 0; i--) {
            sendBit((address >> i) & 1);
        }
        
        uint8_t data = 0;
        // Read 8 bits of Data
        for (int i = 7; i >= 0; i--) {
            data = (data << 1) | readBit();
        }
        
        digitalWrite(PIN_CS, LOW); 
        delayMicroseconds(2); // CS hold time
        return data;
    }

    void writeByte(int address, uint8_t data) {
        // 1. EWEN (Erase/Write Enable)
        digitalWrite(PIN_CS, HIGH);
        delayMicroseconds(2);
        sendBit(1); sendBit(0); sendBit(0); // Start(1) Op(00)
        sendBit(1); sendBit(1);             // EWEN prefix
        for(int i = 0; i < 7; i++) sendBit(0); // 7 bits padding (9-bit total addr length)
        digitalWrite(PIN_CS, LOW);
        delayMicroseconds(1000);

        // 2. WRITE
        digitalWrite(PIN_CS, HIGH);
        delayMicroseconds(2);
        sendBit(1); // Start
        sendBit(0); sendBit(1); // Op (01)
        
        // 9-bit Address
        for (int i = 8; i >= 0; i--) {
            sendBit((address >> i) & 1);
        }
        
        // 8-bit Data
        for (int i = 7; i >= 0; i--) {
            sendBit((data >> i) & 1);
        }
        digitalWrite(PIN_CS, LOW);
        
        delay(15); // Write cycle time (15ms delay is safer than delayMicroseconds here)

        // 3. EWDS (Erase/Write Disable)
        digitalWrite(PIN_CS, HIGH);
        delayMicroseconds(2);
        sendBit(1); sendBit(0); sendBit(0); // Start(1) Op(00)
        sendBit(0); sendBit(0);             // EWDS prefix
        for(int i = 0; i < 7; i++) sendBit(0); // 7 bits padding
        digitalWrite(PIN_CS, LOW);
        delayMicroseconds(1000);
    }

    void printDump(int size = 256) {
        std::cout << "Dumping " << size << " bytes from M93C56..." << std::endl;
        for (int i = 0; i < size; i++) {
            if (i % 16 == 0) {
                std::cout << std::endl << std::setw(4) << std::setfill('0') << std::hex << i << " : ";
            }
            uint8_t byte = readByte(i);
            std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)byte << " ";
        }
        std::cout << std::dec << std::endl;
    }

    void backupToFile(const std::string& filename, int size = 256) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Could not open file for writing: " + filename);
        }
        
        std::cout << "Backing up " << size << " bytes to " << filename << "..." << std::endl;
        for (int i = 0; i < size; i++) {
            uint8_t byte = readByte(i);
            file.write(reinterpret_cast<const char*>(&byte), 1);
        }
        file.close();
        std::cout << "Backup complete!" << std::endl;
    }

    void writeFromFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Could not open file for reading: " + filename);
        }
        
        // Determine file size
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        if (size > 256) {
            std::cout << "Warning: File is larger than 256 bytes. Only writing the first 256 bytes." << std::endl;
            size = 256;
        } else if (size == 0) {
            throw std::runtime_error("File is empty.");
        }
        
        std::cout << "Writing " << size << " bytes from " << filename << " to EEPROM..." << std::endl;
        for (int i = 0; i < size; i++) {
            uint8_t byte;
            if (file.read(reinterpret_cast<char*>(&byte), 1)) {
                writeByte(i, byte);
            }
        }
        file.close();
        std::cout << "Write complete!" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <read|backup|write> [filename]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    try {
        M93C56_GPIO eeprom;

        if (mode == "read") {
            eeprom.printDump();
        } else if (mode == "backup") {
            if (argc < 3) {
                std::cerr << "Error: Backup command requires a filename. (e.g., ./program backup cluster.bin)" << std::endl;
                return 1;
            }
            eeprom.backupToFile(argv[2]);
        } else if (mode == "write") {
            if (argc < 3) {
                std::cerr << "Error: Write command requires a filename. (e.g., ./program write mod_cluster.bin)" << std::endl;
                return 1;
            }
            eeprom.writeFromFile(argv[2]);
        } else {
            std::cerr << "Unknown mode: " << mode << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}