#include <iostream>
#include <vector>
#include <iomanip>
#include <wiringPi.h>
#include <string>
#include <cstring>
#include <unistd.h>

// Physical Pin 8  -> wPi 14
// Physical Pin 12 -> wPi 12
// Physical Pin 13 -> wPi 13
// Physical Pin 15 -> wPi 3 (CS Jumper)
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
        pinMode(PIN_DO, INPUT);
        pinMode(PIN_CS, OUTPUT);
        
        digitalWrite(PIN_SK, LOW);
        digitalWrite(PIN_CS, LOW); // CS idle
    }

    void pulseClock() {
        digitalWrite(PIN_SK, HIGH);
        usleep(50); // Slower clock for reliability
        digitalWrite(PIN_SK, LOW);
        usleep(50);
    }

    void sendBit(int bit) {
        digitalWrite(PIN_DI, bit ? HIGH : LOW);
        pulseClock();
    }

    int readBit() {
        pulseClock();
        return digitalRead(PIN_DO);
    }

    uint8_t readByte(int address) {
        digitalWrite(PIN_CS, HIGH); // Start command
        
        sendBit(1); // Start bit
        sendBit(1); // Opcode 1
        sendBit(0); // Opcode 0 (Read)

        // Address (8 bits)
        for (int i = 7; i >= 0; i--) {
            sendBit((address >> i) & 1);
        }

        uint8_t data = 0;
        // Data (8 bits)
        for (int i = 7; i >= 0; i--) {
            data = (data << 1) | readBit();
        }
        
        digitalWrite(PIN_CS, LOW); // End command
        return data;
    }

    void writeByte(int address, uint8_t data) {
        // 1. EWEN (Erase/Write Enable)
        digitalWrite(PIN_CS, HIGH);
        sendBit(1); sendBit(0); sendBit(0); // Start(1) Op(00)
        sendBit(1); sendBit(1);             // Addr (11xxxxxx)
        for(int i=0; i<6; i++) sendBit(0);  // Padding
        digitalWrite(PIN_CS, LOW);
        usleep(1000); // Small delay between commands

        // 2. WRITE
        digitalWrite(PIN_CS, HIGH);
        sendBit(1); // Start
        sendBit(0); sendBit(1); // Op (01)
        for (int i = 7; i >= 0; i--) sendBit((address >> i) & 1); // Address
        for (int i = 7; i >= 0; i--) sendBit((data >> i) & 1);    // Data
        digitalWrite(PIN_CS, LOW);
        
        usleep(10000); // Write cycle time (10ms)

        // 3. EWDS (Erase/Write Disable)
        digitalWrite(PIN_CS, HIGH);
        sendBit(1); sendBit(0); sendBit(0); // Start(1) Op(00)
        sendBit(0); sendBit(0);             // Addr (00xxxxxx)
        for(int i=0; i<6; i++) sendBit(0);  // Padding
        digitalWrite(PIN_CS, LOW);
    }

    void printDump(int size = 128) {
        std::cout << "Reading M93C56 Memory:" << std::endl;
        for (int i = 0; i < size; i++) {
            if (i % 16 == 0) std::cout << std::endl << std::setw(4) << std::setfill('0') << std::hex << i << ": ";
            std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)readByte(i) << " ";
        }
        std::cout << std::endl;
    }
};

int main(int argc, char* argv[]) {
    try {
        M93C56_GPIO eeprom;
        if (argc < 2) {
            std::cout << "Usage: sudo ./m93c56_gpio [read|write \"text\"]" << std::endl;
            return 1;
        }

        std::string mode = argv[1];
        if (mode == "read") {
            eeprom.printDump();
        } else if (mode == "write" && argc > 2) {
            std::string data = argv[2];
            std::cout << "Writing: " << data << std::endl;
            for (size_t i = 0; i < data.length(); ++i) {
                eeprom.writeByte(i, (uint8_t)data[i]);
            }
            std::cout << "Write complete." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}