/**
 * filename:    mateligent_stretchable_sensor_driver.cpp
 * description: Implements the Mateligent Flex Sensor communication interface.
 * author:      Scott Chen
 * date:        07/06/2026
 */

#include "mateligent_stretchable_sensor_driver.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Mateligent
{

Result StretchSensor::parseMeasurement(const uint8_t *data, size_t len, std::variant<CalibratedMeasurement, RawMeasurement>& out_measurement)
{
    constexpr size_t kBinaryFormatLen = 6;
    constexpr size_t kAsciiCalibratedLen = 14;
    constexpr size_t kAsciiRawLen = 12;
    constexpr size_t kMeasurementErrorLen = 9;

    if (data == nullptr)
    {
        return Result::kParseError;
    }

    if (len == kMeasurementErrorLen && 
            memcmp(data, "EAP error", kMeasurementErrorLen) == 0)
    {
        return Result::kMeasurementFailed;
    }
    /**
     * Binary Format:
     * a. 2 bytes temperature in degrees (K) in tenths. 273.1 °K = 0.0 °C
     * b. 2 bytes primary reading in percentage in hundredths.
     *     i. (0x0000 = 000.00% to 0x7FFF = 327.67%)
     *     ii. Expected maximum value approx. 120.00%
     *     iii. Maximum value saturated to 200.00%
     *     iv. Expected minimum value – saturated to 0.00%
     *     v. Error message for disconnected sensor, overflow, etc (0x8000/-32768?)
     * c. 1 byte status flag – bit 0x01 set for sensor error, bit 0x20 set for saturated (calibrated)
     * measurement.
     * d. 1 byte checksum (sum of previous bytes in message)
     */
    else if (len == kBinaryFormatLen)
    {
        const uint8_t checksum = data[0] + data[1] + data[2] + data[3] + data[4];

        if (checksum != data[5])
        {
            return Result::kChecksumError;
        }

        const uint16_t raw_temp = static_cast<uint16_t>(data[0] << 8) | static_cast<uint16_t>(data[1]);
        const uint16_t raw_stretch = static_cast<uint16_t>(data[2] << 8) | static_cast<uint16_t>(data[3]);

        if (raw_stretch == 0x8000)
        {
            return Result::kMeasurementFailed;
        }

        CalibratedMeasurement measurement {
            .percentage_stretch = static_cast<float>(raw_stretch) / 100.0f,
            .temperature_k = static_cast<float>(raw_temp) / 10.0f,
            .mode = OutputMode::kBinary,
            .status_flags = data[4]
        };

        out_measurement = measurement;
        
        return Result::kSuccess;
    }
    /**
     * Calibrated Ascii Format:
     * Character (ASCII) data format, calibrated sensor. Communication settings: 9600 baud, 1 start
     * bit, 8 data bits, 1 stop bit. NOTE: baud rate for AX-008-55 (high speed) version is 115,200 baud.
     * a. Message string format: “000.00%,000.0K”
     * b. 6 bytes primary reading in percentage in hundredths with decimal point.
     *     i. (“000.00” = 000.00% to “327.67” = 327.67%)
     *     ii. Expected maximum value approx. 120.00%
     * c. 1 byte “%”, indicating calibrated percentage reading.
     *     d. 1 byte comma
     *     e. 5 bytes temperature in Kelvin degrees in tenths with decimal point. (“273.1” = 273.1K)
     *     f. 1 byte “K”, indicating Kelvin temperature units.
     *     g. Message string format if sensor measurement error present (measurement overflow or
     *     open circuit): “EAP error”
     */
    else if (len == kAsciiCalibratedLen && 
            data[6] == '%' &&
            data[7] == ',' &&
            data[13] == 'K')
    {
        char ascii_data[kAsciiCalibratedLen + 1];
        memcpy(ascii_data, data, kAsciiCalibratedLen);
        ascii_data[kAsciiCalibratedLen] = '\0';

        CalibratedMeasurement measurement {
            .percentage_stretch = strtof(ascii_data, nullptr),
            .temperature_k = strtof(ascii_data + 8, nullptr),
            .mode = OutputMode::kAscii,
            .status_flags = 0x00
        };

        out_measurement = measurement;
        return Result::kSuccess;
    }
    /**
     * Raw Measurement Format:
     * Character (ASCII) data format, uncalibrated sensor. Communication settings: 9600 baud, 1 start
     * bit, 8 data bits, 1 stop bit. NOTE: baud rate for AX-008-55 (high speed) version is 115,200 baud.
     * a. Message string format: “00000,000.0K”
     * b. 5 bytes primary reading – raw measurement counts
     *     i. Minimum value 0
     *     ii. Maximum value 65535
     * c. 1 byte comma
     * d. 5 bytes temperature in Kelvin degrees in tenths with decimal point. (“273.1” = 273.1K)
     * e. 1 byte “K”, indicating Kelvin temperature units.
     * f. Message string format if sensor measurement error present (measurement overflow or
     * open circuit): “EAP error
     */
    else if (len == kAsciiRawLen &&
            data[5] == ',' &&
            data[11] == 'K')
    {
        char ascii_data[kAsciiRawLen + 1];
        memcpy(ascii_data, data, kAsciiRawLen);
        ascii_data[kAsciiRawLen] = '\0';

        RawMeasurement measurement{
            .raw_count = static_cast<uint16_t>(strtol(ascii_data, nullptr, 10)),
            .temperature_k = strtof(ascii_data + 6, nullptr),
        };

        out_measurement = measurement;
        return Result::kSuccess;
    }

    return Result::kParseError;
}

Result StretchSensor::sendCommandAndGetResponse(const char *cmd, size_t cmd_len, char *out_buf, size_t buf_len, uint32_t timeout_ms)
{
    Result res = uart_.write((uint8_t *)cmd, cmd_len);
    if (res != Result::kSuccess)
    {
        return res;
    }

    res = uart_.read((uint8_t *)out_buf, buf_len, buf_len, timeout_ms);
    return res;
}

Result StretchSensor::setUintCommand(const char *command, uint32_t value)
{
    char tx[32];
    int len = snprintf(tx, sizeof(tx), "%s=%lu",
                        command,
                        static_cast<unsigned long>(value));

    char rx[64];
    return sendCommandAndGetResponse(
        tx,
        static_cast<size_t>(len),
        rx,
        sizeof(rx));
}

Result StretchSensor::queryUintCommand(const char *command, uint16_t &value)
{
    char tx[8];
    int len = snprintf(tx, sizeof(tx), "%s?", command);

    char rx[64];

    Result result = sendCommandAndGetResponse(
        tx,
        static_cast<size_t>(len),
        rx,
        sizeof(rx));

    if (result != Result::kSuccess)
    {
        return result;
    }

    value = static_cast<uint16_t>(strtoul(rx, nullptr, 10));
    return Result::kSuccess;
}

Result StretchSensor::setBoolCommand(const char *command, bool value)
{
    return setUintCommand(command, value ? 1 : 0);
}

Result StretchSensor::sendSimpleCommand(const char *command)
{
    char rx[64];

    return sendCommandAndGetResponse(
        command,
        strlen(command),
        rx,
        sizeof(rx));
}

//
// Calibration
//

Result StretchSensor::performZeroCalibration()
{
    return sendSimpleCommand("CZ=1");
}

Result StretchSensor::performSpanCalibration()
{
    return sendSimpleCommand("CS=1");
}

Result StretchSensor::setZeroCalibrationValue(uint16_t value)
{
    return setUintCommand("CZ", value);
}

Result StretchSensor::getZeroCalibrationValue(uint16_t &out_value)
{
    return queryUintCommand("CZ", out_value);
}

Result StretchSensor::setSpanCalibrationValue(uint16_t value)
{
    return setUintCommand("CS", value);
}

Result StretchSensor::getSpanCalibrationValue(uint16_t &out_value)
{
    return queryUintCommand("CS", out_value);
}

Result StretchSensor::setCalibrationTemperature(uint16_t temperature_tenths_k)
{
    return setUintCommand("CT", temperature_tenths_k);
}

Result StretchSensor::getCalibrationTemperature(uint16_t &out_temperature_tenths_k)
{
    return queryUintCommand("CT", out_temperature_tenths_k);
}

Result StretchSensor::setCalibrationTemperatureCoefficient(uint16_t coefficient)
{
    return setUintCommand("CC", coefficient);
}

Result StretchSensor::getCalibrationTemperatureCoefficient(uint16_t &out_coefficient)
{
    return queryUintCommand("CC", out_coefficient);
}

Result StretchSensor::calibrationBurst(uint8_t exponent)
{
    return setUintCommand("CB", exponent);
}

Result StretchSensor::calibrationErase()
{
    return sendSimpleCommand("CE=1");
}

//
// Configuration
//

Result StretchSensor::setPollingPeriod(uint16_t period)
{
    return setUintCommand("CP", period);
}

Result StretchSensor::getPollingPeriod(uint16_t &out_period)
{
    return queryUintCommand("CP", out_period);
}

Result StretchSensor::setOversampling(uint8_t exponent)
{
    return setUintCommand("SA", exponent);
}

Result StretchSensor::getOversampling(uint8_t &out_exponent)
{
    uint16_t value;
    Result result = queryUintCommand("SA", value);

    if (result != Result::kSuccess)
        return result;

    out_exponent = static_cast<uint8_t>(value);
    return Result::kSuccess;
}

Result StretchSensor::setLedConfig(LedConfig config)
{
    return setUintCommand(
        "CL",
        static_cast<uint8_t>(config));
}

Result StretchSensor::getLedConfig(LedConfig &out_config)
{
    uint16_t value;

    Result result = queryUintCommand("CL", value);

    if (result != Result::kSuccess)
        return result;

    out_config = static_cast<LedConfig>(value);
    return Result::kSuccess;
}

Result StretchSensor::setOutputDataType(OutputMode type)
{
    return setUintCommand(
        "CD",
        static_cast<uint8_t>(type));
}

Result StretchSensor::getOutputDataType(OutputMode &out_type)
{
    uint16_t value;

    Result result = queryUintCommand("CD", value);

    if (result != Result::kSuccess)
        return result;

    out_type = static_cast<OutputMode>(value);
    return Result::kSuccess;
}

Result StretchSensor::setCalibrationCurrent(uint16_t current_nA)
{
    return setUintCommand("CI", current_nA);
}

Result StretchSensor::getCalibrationCurrent(uint16_t &out_current_nA)
{
    return queryUintCommand("CI", out_current_nA);
}

Result StretchSensor::setPwmOutput(bool enabled)
{
    return setBoolCommand("CA", enabled);
}

Result StretchSensor::writeConfigToFlash()
{
    return sendSimpleCommand("CR=1");
}

//
// Sensor information
//

Result StretchSensor::getFirmwareVersion(char *buffer, size_t buffer_len)
{
    return sendCommandAndGetResponse(
        "SF?",
        3,
        buffer,
        buffer_len);
}

Result StretchSensor::setSerialNumber(uint32_t serial_number)
{
    Result result = setUintCommand(
        "S1",
        static_cast<uint16_t>(serial_number >> 16));

    if (result != Result::kSuccess)
        return result;

    return setUintCommand(
        "S2",
        static_cast<uint16_t>(serial_number & 0xFFFF));
}

Result StretchSensor::getSerialNumber(uint32_t &out_serial_number)
{
    char response[32];

    Result result = sendCommandAndGetResponse(
        "SN?",
        3,
        response,
        sizeof(response));

    if (result != Result::kSuccess)
        return result;

    out_serial_number = static_cast<uint32_t>(
        strtoul(response, nullptr, 16));

    return Result::kSuccess;
}

Result StretchSensor::setSensorLength(uint16_t length_mm)
{
    return setUintCommand("SL", length_mm);
}

Result StretchSensor::getSensorLength(uint16_t &out_length_mm)
{
    return queryUintCommand("SL", out_length_mm);
}

Result StretchSensor::sensorSleep()
{
    return sendSimpleCommand("SS=1");
}

Result StretchSensor::enterBootloader()
{
    return sendSimpleCommand("SF=1");
}

} // namespace Mateligent