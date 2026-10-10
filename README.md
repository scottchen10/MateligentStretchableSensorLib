# Mateligent Stretchable Sensor Library

A platform-independent C++17 library for reading and configuring Mateligent EAP Stretchable Sensors. This library parses the sensor's incoming serial data into structured, discrete messages, supporting ASCII and binary measurements, log messages, and configuration requests and responses.

The library is not dependent on any specific hardware platform, it only expects the data to be streamed in byte by byte with multibyte fields using big-endian ordering.

## Quick Start

### Option 1: Using CMake

Clone the repository and initialize its submodules:
```bash
git clone --recurse-submodules https://github.com/scottchen10/MateligentStretchableSensorLib.git
cd MateligentStretchableSensorLib
git submodule update --init --recursive
```

Add the library as a subdirectory and link it to your executable:

```CMake
cmake_minimum_required(VERSION 3.16)
project(my_app LANGUAGES CXX)

add_subdirectory(path/to/MateligentStretchableSensorLib)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE mateligent_stretch_sensor_lib)
```

### Option 2: Using PlatformIO
Clone the repository into your projects ``/lib/`` directory and PlatformIO will automatically discover the libary

```bash
cd my_app/lib
git clone --recurse-submodules https://github.com/scottchen10/MateligentStretchableSensorLib.git
```

Alternatively, add the repository as a Git submodule:

```bash
cd my_app/lib
git clone --recurse-submodules https://github.com/scottchen10/MateligentStretchableSensorLib.git
git submodule update --init --recursive
```

## Basic Usage

```cpp
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <variant>

#include <mateligent/stretchable_sensor_data.hpp>
#include <mateligent/stretchable_sensor_parser.hpp>

int main()
{
    using namespace Mateligent::StretchableSensor;

    // Assume Serial allows us to read and write bytes to UART.
    // Example sending a command
    char command[32];
    uint16_t value = 10;
    buildSettingCommand(Setting::kConfigurePollingRate , value, command, sizeof(command));
    Serial.write(command, strlen(command));

    // Example parsing incoming stream
    Parser parser{};
    while (Serial.has_value())
    {
        const uint8_t byte = Serial.consume_byte();
        parser.feed(byte);

        const std::optional<Message> msg = parser.consume();

        if (msg.has_value() &&
            std::holds_alternative<CalibratedMeasurement>(*msg))
        {
            const auto& meas = std::get<CalibratedMeasurement>(*msg);

            std::printf(
                "Calibrated Measurement:\n"
                "  Stretch: %f\n"
                "  Temperature: %f K\n",
                meas.percentage_stretch,
                meas.temperature_k
            );
        }
    }

    return 0;
}
```

## Building and Testing

This project uses CMake and includes GoogleTest-based unit tests.

```bash
cmake -S . -B build -DBUILD_STRETCHABLE_SENSOR_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Dependencies

- C++17 compiler
- CMake
- ETL library (under `external/etl`)
- GoogleTest (fetched automatically when `BUILD_STRETCHABLE_SENSOR_TESTS=ON`)
