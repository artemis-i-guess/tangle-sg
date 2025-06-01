#!/bin/bash

echo "[INFO] Starting setup for Tangle-SG system..."

# Enabling SPI
CONFIG_FILE="/boot/config.txt"
if ! grep -q "^dtparam=spi=on" "$CONFIG_FILE"; then
    echo "[INFO] Enabling SPI in /boot/config.txt"
    echo "dtparam=spi=on" | sudo tee -a "$CONFIG_FILE"
    sudo raspi-config nonint do_spi 0
    SPI_ENABLED_NOW=true
else
    echo "[INFO] SPI already enabled."
fi

# Rebooting...
if [ "$SPI_ENABLED_NOW" = true ]; then
    echo "[INFO] SPI was just enabled. Rebooting..."
    sudo reboot
fi

# 3. Update system and install dependencies
echo "[INFO] Updating packages..."
sudo apt-get update && sudo apt-get upgrade -y

echo "[INFO] Installing required packages..."
sudo apt-get install -y \
    python3 \
    python3-pip \
    python3-dev \
    libpython3-dev \
    build-essential \
    git \
    cmake \
    libsqlite3-dev \
    wiringpi \
    pigpio \
    python3-spidev

# 4. Install required Python packages
echo "[INFO] Installing Python libraries..."
pip3 install --upgrade pip
pip3 install spidev RPi.GPIO

# 5. Check if SPI device nodes exist
if [ ! -e /dev/spidev0.0 ]; then
    echo "[ERROR] SPI not enabled or not detected. Check wiring and reboot manually."
    exit 1
fi

# 6. Start pigpiod (if used by your Python driver)
if ! pgrep -x "pigpiod" > /dev/null; then
    echo "[INFO] Starting pigpio daemon..."
    sudo pigpiod
fi

# 7. Build the C++ system
echo "[INFO] Building the C++ system..."
make clean && make

# 8. Start the C++ program
echo "[INFO] Starting the system..."
./build/tangle_main
