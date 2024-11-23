# Esp32 Christmas Light

This project implements advanced LED control using the ESP32 microcontroller and the TLC59116 constant current driver. While it is designed for creating beautiful and customizable Christmas light displays, it can also be adapted for other LED effects and creative lighting solutions.

## Features

- ESP32 Microcontroller: The heart of the project for processing and control.
- TLC59116: Handles constant current control for LEDs.

## Installation

### Clone the repository:

- Set up the [ESP-IDF](https://github.com/espressif/esp-idf) environment: Follow the official ESP-IDF Getting Started guide to set up the development environment.

- Copy code
git clone [Esp32ChristmasLight](https://github.com/dscabello/Esp32ChristmasLight).git
cd Esp32ChristmasLight

### Build the project:
```
bash
Copy code
idf.py build
```

### Flash the firmware to your ESP32:
```
bash
Copy code
idf.py -p <PORT> flash
```

### Monitor the output:
```
bash
Copy code
idf.py monitor
```

### Usage

TODO.

### Contributing
We welcome contributions! To contribute, please fork the repository, create a new branch, and submit a pull request with your changes.

```
Fork the project.
Create your feature branch:
bash
```
Copy code
```
git checkout -b feature/AmazingFeature
Commit your changes:
bash
```
Copy code
```
git commit -m 'Add some amazing feature'
Push to the branch:
```
bash
Copy code
```
git push origin feature/AmazingFeature
Open a pull request.
```


## Example folder contents


```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   ├── include
│   │   ├── lightCtrl.h
│   │   ├── spiHandler.h
│   │   ├── tasksHandler.h
│   │   └── wifiHandler.h
│   ├── Kconfig.projbuild
│   ├── main.c
│   └── src
│       ├── lightCtrl.c
│       ├── spiHandler.c
│       ├── tasksHandler.c
│       └── wifiHandler.c
├── README.md
└── sdkconfig

```

## License

- Attribution-NonCommercial-ShareAlike 4.0 International [LICENSE](https://github.com/dscabello/Esp32ChristmasLight/blob/main/LICENSE)