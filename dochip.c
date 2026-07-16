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
        digitalWrite(PIN_SK, HIGH);
        usleep(100); // Wait for DO to stabilize
        int bit = digitalRead(PIN_DO);
        digitalWrite(PIN_SK, LOW);
        usleep(100);
        return bit;
    }

    uint8_t readByte(int address) {
        digitalWrite(PIN_CS, HIGH); 
        usleep(10); // CS setup time
        
        sendBit(1); // Start bit
        sendBit(1); // Opcode 1
        sendBit(0); // Opcode 0 (Read)

        // CRITICAL FIX: Address must be exactly 9 bits for M93C56 in 8-bit mode (A8 down to A0).
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
        usleep(10); // CS hold time
        return data;
    }

    void writeByte(int address, uint8_t data) {
        // 1. EWEN (Erase/Write Enable)
        digitalWrite(PIN_CS, HIGH);
        usleep(10);
        sendBit(1); sendBit(0); sendBit(0); // Start(1) Op(00)
        sendBit(1); sendBit(1);             // EWEN prefix
        for(int i=0; i<7; i++) sendBit(0);  // 7 bits padding to complete 9-bit address length
        digitalWrite(PIN_CS, LOW);
        usleep(1000);

        // 2. WRITE
        digitalWrite(PIN_CS, HIGH);
        usleep(10);
        sendBit(1); // Start
        sendBit(0); sendBit(1); // Op (01)
        
        // CRITICAL FIX: 9-bit Address for M93C56
        for (int i = 8; i >= 0; i--) {
            sendBit((address >> i) & 1);
        }
        
        for (int i = 7; i >= 0; i--) {
            sendBit((data >> i) & 1);
        }
        digitalWrite(PIN_CS, LOW);
        
        usleep(15000); // Write cycle time (15ms)

        // 3. EWDS (Erase/Write Disable)
        digitalWrite(PIN_CS, HIGH);
        usleep(10);
        sendBit(1); sendBit(0); sendBit(0); // Start(1) Op(00)
        sendBit(0); sendBit(0);             // EWDS prefix
        for(int i=0; i<7; i++) sendBit(0);  // 7 bits padding to complete 9-bit address length
        digitalWrite(PIN_CS, LOW);
        usleep(1000);
    }

    void printDump(int size = 256) {
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