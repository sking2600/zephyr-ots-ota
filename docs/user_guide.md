# Zephyr OTS OTA User Guide

This guide provides step-by-step instructions for setting up the ESP32-C3 device with the initial firmware and ensuring it is ready for Over-The-Air (OTA) updates via Bluetooth LE Object Transfer Service (OTS). It also covers how to perform the actual update using the Rust client.

## Prerequisites

*   **Hardware:** ESP32-C3 DevKitM.
*   **Software:**
    *   Zephyr SDK & West workspace active.
    *   Python environment with `esptool` (for flashing).
    *   Rust toolchain (for the `ots-client` CLI).
*   **Assumptions:**
    *   You have a Zephyr workspace initialized.
    *   `ZEPHYR_BASE` is set or you are inside a west workspace.

---

## Part 1: Initial Device Setup (Wired Flash)

The project uses **Zephyr Sysbuild** to automatically build and flash both **MCUboot** and the **Application** in a single step.

### 1. Setup Environment
Ensure your terminal is in the project root directory and your Zephyr environment is sourced.

```bash
source <path-to-zephyr-workspace>/zephyr/zephyr-env.sh
```

### 2. Build the Project
Build both MCUboot and the application with a single command:

```bash
west build -b esp32c3_devkitm --sysbuild -p always
```

### 3. Flash the Device
Flash the entire system image (Bootloader + App) using the standard Zephyr flash tool:

```bash
west flash
```

> [!TIP]
> **Verification:** Toggle the serial monitor (`west flash --open-monitor`). You should see the MCUboot banner followed by `*** Starting OTS OTA App ***`.

---

## Part 2: Performing Bluetooth DFU (Wireless Update)

Now that the device is running a valid image, you can update it wirelessly.

### 1. Prepare the Update (v2)
Modify your firmware source code (e.g., change the version string in `src/main.c` to "v2.0.0") to confirm the update works.

Rebuild the application to generate the new signed binary:
```bash
west build -b esp32c3_devkitm
```
*   **Target File:** `build/zephyr/zephyr.signed.bin`

### 2. Get the Rust Client
Clone the [bluetooth-ots-rs](https://github.com/sking2600/bluetooth-ots-rs) repository and build the CLI tool.

```bash
git clone https://github.com/sking2600/bluetooth-ots-rs
cd bluetooth-ots-rs
cargo build --release
```

### 3. Run the OTA Update
Use the client to write the signed binary to the device.

```bash
# Replace with your device's BLE Address if different
# The --disco 10 flag scans for 10 seconds to find the device
# Assuming you are inside the bluetooth-ots-rs directory
./target/release/ots-client --disco 10 --device 3C:0F:02:29:5D:88 write --name firmware.bin --file <path-to-zephyr_ots_ota>/build/zephyr/zephyr.signed.bin
```

### 4. What to Expect
1.  **Scanning:** Client finds "Zephyr OTS OTA".
2.  **Connecting:** Client connects and resolves OTS characteristics.
3.  **Writing:** The progress bar will show the upload (using efficient L2CAP chunks).
4.  **Reboot:** Upon completion (100%), the ESP32-C3 will automatically disconnect and reboot.
5.  **Swap:** MCUboot will verify the new image in the secondary slot, swap it into the primary slot, and boot it.
