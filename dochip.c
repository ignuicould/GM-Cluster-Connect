#include <iostream>
#include <vector>
#include <iomanip>
#include <wiringPi.h>

/**
 * PIN CONFIGURATION:
 * IMPORTANT: WiringPi uses its own internal pin numbering. 
 * You must map the physical pins on your header to WiringPi pins.
 * 
 * Physical Header (from your image):
 * - Pin 19 (MOSI) -> Map to your WiringPi DI pin
 * - Pin 21 (MISO) -> Map to your WiringPi DO pin
 * - Pin 23 (CLK)  -> Map to your WiringPi SK pin
 * 
 * Run 'gpio readall' in your terminal to see the WiringPi pin mapping 
 * for your specific board if these values don't match.
 */
const int PIN_SK  = 14; // Update with correct WiringPi Pin # for Header Pin 23
const int PIN_DI  = 12; // Update with correct WiringPi Pin # for Header Pin 19
const int PIN_DO  = 13; // Update with correct WiringPi Pin # for Header Pin 21

class M93C56_GPIO {
public:
    M93C56_GPIO() {
        if (wiringPiSetup() == -1) {
            throw std::runtime_error("Failed to initialize wiringPi.");
        }
        pinMode(PIN_SK, OUTPUT);
        pinMode(PIN_DI, OUTPUT);
        pinMode(PIN_DO, INPUT);
        
        digitalWrite(PIN_SK, LOW);
    }

    void pulseClock() {
        digitalWrite(PIN_SK, HIGH);
        delayMicroseconds(10);
        digitalWrite(PIN_SK, LOW);
        delayMicroseconds(10);
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
        // CS is handled by hardware jumper, so we proceed directly
        
        // Start bit (1) + Read Opcode (10)
        sendBit(1);
        sendBit(1);
        sendBit(0);

        // Send 8-bit address
        for (int i = 7; i >= 0; i--) {
            sendBit((address >> i) & 1);
        }

        uint8_t data = 0;
        for (int i = 7; i >= 0; i--) {
            data = (data << 1) | readBit();
        }

        return data;
    }

    void printDump(int size = 256) {
        std::cout << "Reading M93C56 Memory (CS/ORG jumpered):" << std::endl;
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
        if (argc > 1 && std::string(argv[1]) == "read") {
            eeprom.printDump();
        } else {
            std::cout << "Usage: ./m93c56_gpio read" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}