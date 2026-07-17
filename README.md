# GM-Cluster-Connect
GM Cluster Connect is a dedicated Raspberry Pi HAT designed for interfacing with and programming automotive EEPROM chips (specifically the M93 series) commonly found in GM instrument clusters.

![GM Cluster Connect HAT Raspberry Pi](./img/gm-cluster-connect-hat-raspberry-pi.png)

This project provides a hardware platform and a Python-based utility to read full-chip binary dumps and write backups back to new EEPROM chips, allowing for cluster restoration and maintenance.

## Features
* **Dedicated Hardware HAT:** Custom PCB layout for Raspberry Pi featuring:
  * Direct SPI interfacing via GPIO.
  * Configurable jumpers for ORG (mode selection) and CS (Enable/Disable).
  * Decoupling capacitor footprint for signal stability.

* **Python EEPROM Controller:** Command-line utility to:
  * Perform full-chip reads/dumps to .bin files.
  * Flash binary files back to the chip.
  * Support for 8-bit and 16-bit Microwire modes.

## Replacement Chip

https://www.mouser.com/en/ProductDetail/511-M93C56-WMN6TP

https://www.mouser.com/en/ProductDetail/STMicroelectronics/M93C56-RMN3TP-K (automotive grade)

## Hardware Configuration
**WARNING:** This tool interacts with critical vehicle hardware. Ensure all connections are secure before powering the Pi.
* **ORG Jumper:** Set to **GND** for 8-bit operation (default).

## Setup & Installation

### Instrument Cluster
Run the following command to compile cluster.cpp:

```cmd
g++ -o cluster cluster.cpp -lwiringPi -lpthread -lm -lcrypt
```

### Reading Cluster Data
Run the following command to compile cluster.cpp:

```cmd
sudo ./cluster read
```

### Creating Backup of Cluster Data
Run the following command to make a backup of your current cluster data:

```cmd
sudo ./cluster backup "mycluster.bin"
```

### Writing Backup of Cluster Data to a New Chip
Run the following command to copy your cluster data onto a new chip:

```cmd
sudo ./cluster write "mycluster.bin"
```

### BCM Mileage Calculator
Run the following command to compile mileage.cpp:

```cmd
g++ -O2 -o mileage mileage.cpp
```

### Generating Mileage Hex
The following example assumes 100,000 miles (use your own mileage):

```cmd
./mileage 100,000
```

### Mileage Output (expected)

```cmd
Target Odometer : 100000 miles
Calculated VSS  : 400000000 pulses (0x17d78400)
Encoded Hex     : 17 D7 84 00
BCM 0x0080 Write: 17 D7 84 00 17 D7 84 00 17 D7 84 00
```





### Python approach (not verified working)
Run the following commands on your Raspberry Pi to prepare the environment and enable SPI:

```cmd
sudo apt update && sudo apt install -y python3-pip python3-spidev
sudo raspi-config nonint do_spi 0
sudo reboot
```

## Usage

The eeprom_controller.py script provides a simple CLI interface.

### Dump EEPROM to file

```cmd
python3 eeprom_controller.py --dump backup.bin
```

### Write binary file to EEPROM

```cmd
python3 eeprom_controller.py --write new_data.bin
```

### Help

```cmd
python3 eeprom_controller.py -h
```

## Automotive Safety & Disclaimer
**This project is for educational and maintenance use only.**
* **New Chip Requirement:** This hardware and software are designed for use with **BRAND NEW** EEPROM chips. Attempting to flash data onto a used (worn-out) chip is likely to fail again due to the end-of-life cycle of the original flash memory.
* **Legal:** Modifying odometer data is strictly regulated. Ensure compliance with all local laws and regulations regarding vehicle documentation.
* **Checksums:** Automotive clusters often use checksums or CRCs. Writing raw binary data without calculating or patching the appropriate checksums will likely render the cluster non-functional.
* **Backups:** Always read and save a "Golden Image" (full dump) of your original chip before attempting any write operations.
