# 1. Update your package manager
sudo apt update && sudo apt install -y python3-pip python3-spidev

# 2. Enable the SPI interface (if not already enabled)
# This command adds the necessary lines to your config and requires a reboot.
sudo raspi-config nonint do_spi 0

echo "Prerequisites installed. Please reboot your Raspberry Pi for SPI changes to take effect: 'sudo reboot'"