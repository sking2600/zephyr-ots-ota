# Zephyr OTS OTA Demo

This repository demonstrates how to implement Over-The-Air (OTA) firmware updates using the Bluetooth Low Energy (BLE) Object Transfer Service (OTS) on an ESP32-C3 running Zephyr RTOS.

## Overview

The demo consists of:
1.  **Zephyr Application (`ots_ota_app`):** A firmware implementation that includes:
    -   **MCUboot:** Secure bootloader for image verification and swap.
    -   **Object Transfer Service (OTS):** A BLE service used to stream the new firmware binary.
    -   **Stream Flash:** Integration to write the incoming stream directly to the flash secondary slot.
2.  **Rust Client:** A command-line tool used to scan for the device and upload the signed firmware binary using OTS and L2CAP COC (Connection Oriented Channel) for high throughput.

## Getting Started

Follow the [User Guide](docs/user_guide.md) for step-by-step instructions on:
- Setting up the Zephyr environment.
- Building and flashing the initial firmware.
- Using the Rust client to perform a wireless update.

## Repository Structure

- `src/`, `boards/`, `CMakeLists.txt`, `prj.conf`: The Zephyr application source code and build configuration.
- `docs/`: Detailed documentation and user guide.

## External Resources

- [bluetooth-ots-rs](https://github.com/sking2600/bluetooth-ots-rs): The Rust implementation of the BLE OTS client.
