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

Result StretchSensor::sendCommandAndGetResponse(const char *cmd, size_t cmd_len, char *out_buf, size_t buf_len, uint32_t timeout_ms)
{
    Result res = uart_.write((uint8_t *)cmd, cmd_len);
    if (res != Result::kOkay)
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

    if (result != Result::kOkay)
    {
        return result;
    }

    value = static_cast<uint16_t>(strtoul(rx, nullptr, 10));
    return Result::kOkay;
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

    if (result != Result::kOkay)
        return result;

    out_exponent = static_cast<uint8_t>(value);
    return Result::kOkay;
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

    if (result != Result::kOkay)
        return result;

    out_config = static_cast<LedConfig>(value);
    return Result::kOkay;
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

    if (result != Result::kOkay)
        return result;

    out_type = static_cast<OutputMode>(value);
    return Result::kOkay;
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

    if (result != Result::kOkay)
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

    if (result != Result::kOkay)
        return result;

    out_serial_number = static_cast<uint32_t>(
        strtoul(response, nullptr, 16));

    return Result::kOkay;
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