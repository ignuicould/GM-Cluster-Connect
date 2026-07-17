#include <iostream>
#include <string>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <cmath>

// Helper function to strip commas, spaces, and other non-numeric formatting characters
std::string cleanInput(const std::string& input) {
    std::string cleaned = "";
    bool decimalFound = false;
    
    for (char c : input) {
        if (std::isdigit(c)) {
            cleaned += c;
        } else if (c == '.' && !decimalFound) {
            // Keep the first decimal point in case they pass a float like "100500.5"
            cleaned += c;
            decimalFound = true;
        }
    }
    return cleaned;
}

void calculateMileageHex(double miles) {
    // 1. Calculate BCM VSS (Vehicle Speed Sensor) pulses (4000 pulses per mile)
    double rawPulses = miles * 4000.0;
    
    // Round to the nearest whole integer pulse
    uint64_t pulses = static_cast<uint64_t>(std::round(rawPulses));
    
    // 2. Perform 32-bit bounds checking
    // The BCM stores pulses in a 32-bit register (Max value: 4,294,967,295)
    const uint64_t MAX_PULSES = 0xFFFFFFFFULL;
    if (pulses > MAX_PULSES) {
        std::cerr << "Error: Mileage exceeds maximum supported 32-bit limit (" 
                  << (MAX_PULSES / 4000.0) << " miles)." << std::endl;
        return;
    }
    
    // 3. Extract the 4 big-endian bytes
    uint8_t byte1 = (pulses >> 24) & 0xFF;
    uint8_t byte2 = (pulses >> 16) & 0xFF;
    uint8_t byte3 = (pulses >> 8) & 0xFF;
    uint8_t byte4 = pulses & 0xFF;
    
    // 4. Output the results
    std::cout << std::uppercase << std::hex << std::setfill('0');
    
    // Print the raw target bytes
    std::cout << "Target Odometer : " << std::dec << miles << " miles" << std::endl;
    std::cout << "Calculated VSS  : " << std::dec << pulses << " pulses (0x" 
              << std::hex << pulses << ")" << std::endl;
    std::cout << "Encoded Hex     : " 
              << std::setw(2) << (int)byte1 << " " 
              << std::setw(2) << (int)byte2 << " " 
              << std::setw(2) << (int)byte3 << " " 
              << std::setw(2) << (int)byte4 << std::endl;
              
    // Print the redundant string ready for BCM address 0x0080
    std::cout << "BCM 0x0080 Write: ";
    for (int i = 0; i < 3; i++) {
        std::cout << std::setw(2) << (int)byte1 << " " 
                  << std::setw(2) << (int)byte2 << " " 
                  << std::setw(2) << (int)byte3 << " " 
                  << std::setw(2) << (int)byte4;
        if (i < 2) std::cout << " ";
    }
    std::cout << std::dec << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "GM/Hummer H3 BCM Mileage Encoder" << std::endl;
        std::cout << "Usage: " << argv[0] << " <mileage>" << std::endl;
        std::cout << "Example: " << argv[0] << " 100,000" << std::endl;
        std::cout << "Example: " << argv[0] << " 180,705" << std::endl;
        return 1;
    }
    
    std::string rawInput = argv[1];
    std::string cleanedInput = cleanInput(rawInput);
    
    if (cleanedInput.empty()) {
        std::cerr << "Error: Invalid mileage input. Please enter a valid number." << std::endl;
        return 1;
    }
    
    try {
        double miles = std::stod(cleanedInput);
        if (miles < 0) {
            std::cerr << "Error: Mileage cannot be negative." << std::endl;
            return 1;
        }
        calculateMileageHex(miles);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing input: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}