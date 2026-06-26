import spidev
import time
import argparse
import sys

"""
HARDWARE CONFIGURATION NOTES:
- ORG Pin: Must be tied to GND (Default) for 8-bit mode operation.
- CS (Chip Select) Pin: Must be in the "ENABLED" (HIGH) state via your 
  physical board jumper to perform any Read or Write operations.
"""

class M93C56:
    def __init__(self, is_16bit=False):
        self.spi = spidev.SpiDev()
        self.spi.open(0, 0)
        self.spi.max_speed_hz = 1000000
        self.spi.mode = 0b00
        self.spi.cshigh = True  # Microwire requirement: CS is active-HIGH
        self.is_16bit = is_16bit

    def read(self, address):
        """Reads a single byte/word from the EEPROM."""
        if self.is_16bit:
            byte1 = 0x06
            byte2 = (address << 1) & 0xFE
            tx = [byte1, byte2, 0x00, 0x00]
        else:
            byte1 = 0x06
            byte2 = address & 0xFF
            tx = [byte1, byte2, 0x00, 0x00]
            
        rx = self.spi.xfer2(tx)
        if self.is_16bit:
            return (((rx[2] << 8) | rx[3]) >> 6) & 0xFFFF
        else:
            return (((rx[2] << 8) | rx[3]) >> 7) & 0xFF

    def write_byte(self, address, data):
        """Enables write, writes data, and disables write."""
        # 1. EWEN (Erase/Write Enable)
        self.spi.xfer2([0x00, 0x90])
        
        # 2. WRITE
        if self.is_16bit:
            cmd = [0x05, (address << 1) & 0xFE, (data >> 8) & 0xFF, data & 0xFF]
        else:
            cmd = [0x05, address & 0xFF, data & 0xFF]
        
        self.spi.xfer2(cmd)
        time.sleep(0.01) # Write cycle time (typ. 5ms)
        
        # 3. EWDS (Erase/Write Disable)
        self.spi.xfer2([0x00, 0x80])

    def read_all(self):
        """Reads the full address space."""
        size = 128 if self.is_16bit else 256
        return bytes([self.read(i) for i in range(size)])

    def dump_to_file(self, filename):
        print(f"Dumping EEPROM to {filename}...")
        data = self.read_all()
        with open(filename, "wb") as f:
            f.write(data)
        print("Done.")

    def write_from_file(self, filename):
        print(f"Writing {filename} to EEPROM...")
        with open(filename, "rb") as f:
            data = f.read()
            for addr, byte in enumerate(data):
                self.write_byte(addr, byte)
        print("Done.")

    def close(self):
        self.spi.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="M93C56 EEPROM Programmer for Automotive Clusters.")
    parser.add_argument("--mode", choices=['8', '16'], default='8', help="EEPROM organization mode (default: 8-bit)")
    parser.add_argument("--dump", metavar="FILE", help="Dump entire EEPROM content to the specified file")
    parser.add_argument("--write", metavar="FILE", help="Write binary file content to the EEPROM")
    
    args = parser.parse_args()

    if not (args.dump or args.write):
        parser.print_help()
        sys.exit(1)

    eeprom = M93C56(is_16bit=(args.mode == '16'))
    
    try:
        if args.dump:
            eeprom.dump_to_file(args.dump)
        if args.write:
            eeprom.write_from_file(args.write)
    finally:
        eeprom.close()