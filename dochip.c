#include <iostream>
#include <vector>
#include <iomanip>
#include <wiringPi.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdexcept>

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
        pullUpDnControl(PIN_DO, PUD_OFF); // Ensure DO is floating
        
        digitalWrite(PIN_SK, LOW);
        digitalWrite(PIN_CS, LOW); // CS idle
    }

    void pulseClock() {
        digitalWrite(PIN_SK, HIGH);
        usleep(100); // 100 microseconds for stability
        digitalWrite(PIN_SK, LOW);
        usleep(100);
    }

    void sendBit(int bit) {
        digitalWrite(PIN_DI, bit ? HIGH : LOW);
        pulseClock();
    }

    int readBit() {
        digitalWrite(PIN_SK, HIGH);
        usleep(100); // Wait for DO to stabilize
        int bit = digitalRead(PIN_DO);
        digitalWrite(PIN_SK, LOW);
        usleep(100);
        return bit;
    }

    uint8_t readByte(int address) {
        digitalWrite(PIN_CS, HIGH); 
        usleep(100); // CS setup time
        
        sendBit(1); // Start bit
        sendBit(1); // Opcode 1
        sendBit(0); // Opcode 0 (Read)

        // CRITICAL: M93C56 in 8-bit mode requires a 9-bit address (A8 down to A0).
        for (int i = 8; i >= 0; i--) {
            sendBit((address >> i) & 1);
        }

        int dummy = readBit(); // Consume the dummy '0' bit that Microwire outputs here
        
        uint8_t data = 0;
        // Data (8 bits)
        for (int i = 7; i >= 0; i--) {
            data = (data << 1) | readBit();
        }
        
        digitalWrite(PIN_CS, LOW); 
        usleep(100); // CS hold time
        return data;
    }

    void writeByte(int address, uint8_t data) {
        // 1. EWEN (Erase/Write Enable)
        digitalWrite(PIN_CS, HIGH);
        usleep(100);
        sendBit(1); sendBit(0); sendBit(0); // Start(1) Op(00)
        sendBit(1); sendBit(1);             // EWEN prefix
        for(int i = 0; i < 7; i++) sendBit(0); // 7 bits padding to complete 9-bit address length
        digitalWrite(PIN_CS, LOW);
        usleep(1000);

        // 2. WRITE
        digitalWrite(PIN_CS, HIGH);
        usleep(100);
        sendBit(1); // Start
        sendBit(0); sendBit(1); // Op (01)
        
        // CRITICAL: 9-bit Address for M93C56
        for (int i = 8; i >= 0; i--) {
            sendBit((address >> i) & 1);
        }
        
        for (int i = 7; i >= 0; i--) {
            sendBit((data >> i) & 1);
        }
        digitalWrite(PIN_CS, LOW);
        
        usleep(15000); // Write cycle time (15ms max for this chip)

        // 3. EWDS (Erase/Write Disable)
        digitalWrite(PIN_CS, HIGH);
        usleep(100);
        sendBit(1); sendBit(0); sendBit(0); // Start(1) Op(00)
        sendBit(0); sendBit(0);             // EWDS prefix
        for(int i = 0; i < 7; i++) sendBit(0); // 7 bits padding to complete 9-bit address length
        digitalWrite(PIN_CS, LOW);
        usleep(1000);
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
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <read|write> [data_string]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    try {
        M93C56_GPIO eeprom;

        if (mode == "read") {
            eeprom.printDump();
        } else if (mode == "write" && argc > 2) {
            std::string data = argv[2];
            std::cout << "Writing: " << data << std::endl;
            for (size_t i = 0; i < data.length(); ++i) {
                eeprom.writeByte(i, (uint8_t)data[i]);
            }
            std::cout << "Write complete. You can run 'read' to verify." << std::endl;
        } else if (mode == "write") {
            std::cerr << "Error: Write command requires a data string." << std::endl;
            return 1;
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