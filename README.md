# zigbee airstation
compact CO2 + temperature + barometer zigbee dongle, based on nrf52840

# Description
This project aims to provide a compact and efficient solution for monitoring environmental conditions. The device integrates CO2 sensing, temperature measurement, and barometric pressure monitoring into a single Zigbee-enabled dongle. Utilizing the nrf52840 chipset, it ensures reliable wireless communication and low power consumption, making it ideal for various applications such as smart homes, industrial monitoring, and environmental data collection.
This project is based on the Nordic Connect SDK (NCS) and Zephyr RTOS, and adapted for Adafruit bootloader ([github|https://github.com/adafruit/Adafruit_nRF52_Bootloader/tree/master]
Structure of the project is based on the example [github|https://github.com/zephyrproject-rtos/example-application]

## Structure Description
The project is organized into several key directories and files to facilitate development and maintenance:

- **src/**: Contains the source code for the application, including main logic and sensor integration.
- **include/**: Header files defining interfaces and structures used across the project.
- **boards/**: Board-specific configurations and definitions, including pin mappings and hardware settings.
- **zephyr/**: Zephyr RTOS configuration files and settings.
- **CMakeLists.txt**: Build configuration file for CMake, specifying how the project should be compiled.
- **prj.conf**: Project-specific configuration settings for Zephyr RTOS.
- **README.md**: Documentation and instructions for building and using the project.

# How to build
1. Initialize correct nordic env:
 - [https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/installation/install_ncs.html
 - [https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/app_dev/config_and_build/building.html#building]
2. Copy board in correct path nrf/boards/promicro
- [https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/applications/nrf5340_audio/doc/adapting_application.html#nrf53-audio-app-adapting]
3. Build
west build --pristine -b supermini_nrf52840 app

# How to flash
Connect board to the USB port and double press the reset button to enter the bootloader mode. Then flash the board with the following command:
```sh
# Enter the following commands to flash the firmware:
sudo mount /dev/sdb /mnt -o user,rw
sudo cp zephyr.uf2 /mnt/
sudo umount /mnt
```
