#include <iostream>
#include <vector>
#include <iomanip>
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

        int dummy = readBit(); // Consume the dummy '0' bit
        
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