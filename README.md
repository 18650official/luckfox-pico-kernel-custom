# 🦊 LuckFox Pico Custom Kernel & Toolchain

This repository contains a customized Linux kernel build specifically tailored for the **LuckFox Pico** development board. It integrates several key features and tools beyond the standard configuration to expand its capabilities for embedded projects.

## ✨ Key Features & Enhancements

This kernel branch (`custom_kernel_mods`) is built with the following main features enabled:

| Feature | Description | Benefit |
| :--- | :--- | :--- |
| **`uinput` Support** | The kernel module for creating virtual input devices is enabled. | Essential for simulating keyboard, mouse, or joystick input from user space applications. |
| **ST7796 Display Driver** | Built-in support for the popular **ST7796** TFT LCD controller. | Enables direct, optimized screen output for integrated display projects. |
| **ZRAM Compression** | Enabled kernel feature for compressed swap/RAM block devices. | Improves system responsiveness and memory efficiency on devices with limited RAM. |
| **Extended Toolchain & `httpd`** | Includes a robust toolchain and key networking utilities like the **`httpd`** web server. | Provides a full environment for development and running web-based applications directly on the device. |

## 📦 Getting Started (Pre-compiled Images)

For immediate deployment, you can skip the compilation process and flash a pre-built image directly.

1.  **Download:** Navigate to the **[Releases]** tab of this repository.
2.  **Select:** Download the latest pre-compiled kernel image archive (e.g., `.img` or `.tar.gz`).
3.  **Flash:** Use the standard LuckFox flashing tools (such as `rkdeveloptool` or the proprietary flash tool) to write the downloaded image onto your LuckFox Pico's storage (SD card/NAND).

## 💻 Building from Source

If you need to make further customizations or changes, follow these general steps to rebuild the kernel:

1.  **Clone the Repository:** Ensure you clone this repository using the SSH URL to avoid authentication issues.
    ```bash
    git clone git@github.com:18650official/luckfox-pico-kernel-custom.git
    cd luckfox-pico-kernel-custom
    git checkout custom_kernel_mods
    ```

2.  **Toolchain Location:** This kernel is designed to be built using the toolchain located at:
    ```
    /home/miku/luckfox-pico/sysdrv/source/kernel/
    ```
    Ensure your build environment is correctly set up to use this path for cross-compilation.

3.  **Build:** *[Optional: Add your specific build commands here, e.g., `make luckfox_pico_defconfig && make -j$(nproc)`]*

## 📄 License

This kernel repository is based on the original LuckFox SDK and is distributed under the [Insert License Here, e.g., GPL-2.0].

