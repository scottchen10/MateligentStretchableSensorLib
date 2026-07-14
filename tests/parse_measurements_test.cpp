#include "mateligent_stretchable_sensor_driver.hpp"

#include <gtest/gtest.h>
#include <numeric>
#include <memory>

using namespace Mateligent;

class ParseMeasurementsTest : public ::testing::Test
{
protected:
    std::variant<StretchSensor::CalibratedMeasurement, StretchSensor::RawMeasurement> measurement;
};

std::array<uint8_t, 2> createRawTemperature(float temperature)
{
    std::array<uint8_t, 2> bytes{};

    uint16_t raw_temperature = static_cast<uint16_t>(temperature * 10.0f);

    bytes[0] = static_cast<uint8_t>(raw_temperature >> 8);
    bytes[1] = static_cast<uint8_t>(raw_temperature);

    return bytes;
}

std::array<uint8_t, 2> createRawStretch(float stretch_percent)
{
    std::array<uint8_t, 2> bytes{};

    uint16_t raw_stretch = static_cast<uint16_t>(stretch_percent * 100.0f);

    bytes[0] = static_cast<uint8_t>(raw_stretch >> 8);
    bytes[1] = static_cast<uint8_t>(raw_stretch);

    return bytes;
}

std::array<uint8_t, 6> createBinaryMeasurement(float stretch_percent, float temperature_k, uint8_t status)
{
    auto raw_stretch = createRawStretch(stretch_percent);
    auto raw_temp = createRawTemperature(temperature_k);

    std::array<uint8_t, 6> measurement = {
        raw_temp[0], raw_temp[1],
        raw_stretch[0], raw_stretch[1],
        status,
    };

    measurement[5] = std::accumulate(measurement.begin(), measurement.begin() + 5, 0);

    return measurement;
}

std::array<uint8_t, 6> createBinaryMeasurement(uint16_t raw_stretch, uint16_t raw_temp, uint8_t status)
{
    std::array<uint8_t, 6> measurement = {
        static_cast<uint8_t>(raw_temp >> 8), static_cast<uint8_t>(raw_temp),
        static_cast<uint8_t>(raw_stretch >> 8), static_cast<uint8_t>(raw_stretch),
        status,
    };

    measurement[5] = std::accumulate(measurement.begin(), measurement.begin() + 5, 0);

    return measurement;
}


TEST_F(ParseMeasurementsTest, ParsesBinaryCalibratedMeasurement)
{
    constexpr float temperature_k = 300.0f;
    constexpr float stretch_percent = 100.0f;

    auto packet = createBinaryMeasurement(stretch_percent, temperature_k, 0x00);
    std::variant<StretchSensor::CalibratedMeasurement, StretchSensor::RawMeasurement> measurement{};

    Result result = StretchSensor::parseMeasurement(
            packet.data(),
            packet.size(),
            measurement);

    EXPECT_EQ(result, Result::kSuccess);
    ASSERT_TRUE(std::holds_alternative<StretchSensor::CalibratedMeasurement>(measurement));

    auto value = std::get<StretchSensor::CalibratedMeasurement>(measurement);
    EXPECT_FLOAT_EQ(value.temperature_k, temperature_k);
    EXPECT_FLOAT_EQ(value.percentage_stretch, stretch_percent);
    EXPECT_EQ(value.mode, StretchSensor::OutputMode::kBinary);
    EXPECT_EQ(value.status_flags, 0x00);
}


TEST_F(ParseMeasurementsTest, RejectsBinaryChecksumFailure)
{
    constexpr float temperature_k = 300.0f;
    constexpr float stretch_percent = 100.0f;

    auto packet = createBinaryMeasurement(stretch_percent, temperature_k, 0x00);
    packet[5] = 0xEE;

    std::variant<StretchSensor::CalibratedMeasurement, StretchSensor::RawMeasurement> measurement{};
    Result result = StretchSensor::parseMeasurement(
            packet.data(),
            packet.size(),
            measurement);

    EXPECT_EQ(result, Result::kChecksumError);
}


TEST_F(ParseMeasurementsTest, RejectsBinarySensorErrorValue)
{
    constexpr uint16_t raw_temp = 0xBEEF;
    constexpr uint16_t raw_stretch = 0x8000;

    auto packet = createBinaryMeasurement(raw_stretch, raw_temp, 0x00);

    Result result = StretchSensor::parseMeasurement(
            packet.data(),
            packet.size(),
            measurement);

    EXPECT_EQ(result, Result::kMeasurementFailed);
}


// ASCII calibrated tests

TEST_F(ParseMeasurementsTest, ParsesAsciiCalibratedMeasurement)
{
    uint8_t packet[] =
        "012.34%,300.0K";

    Result result = StretchSensor::parseMeasurement(
            packet,
            14,
            measurement);

    EXPECT_EQ(result, Result::kSuccess);

    ASSERT_TRUE(std::holds_alternative<StretchSensor::CalibratedMeasurement>(measurement));

    auto value = std::get<StretchSensor::CalibratedMeasurement>(measurement);

    EXPECT_FLOAT_EQ(value.percentage_stretch, 12.34f);
    EXPECT_FLOAT_EQ(value.temperature_k, 300.0f);
    EXPECT_EQ(value.mode, StretchSensor::OutputMode::kAscii);
    EXPECT_EQ(value.status_flags, 0);
}


// ASCII raw tests

TEST_F(ParseMeasurementsTest, ParsesAsciiRawMeasurement)
{
    uint8_t packet[] = "12345,300.0K";

    Result result = StretchSensor::parseMeasurement(
            packet,
            12,
            measurement);

    EXPECT_EQ(result, Result::kSuccess);

    ASSERT_TRUE(std::holds_alternative<StretchSensor::RawMeasurement>(measurement));

    auto value = std::get<StretchSensor::RawMeasurement>(measurement);

    EXPECT_EQ(value.raw_count, 12345);
    EXPECT_FLOAT_EQ(value.temperature_k, 300.0f);
}


// Error cases

TEST_F(ParseMeasurementsTest, ParsesSensorErrorMessage)
{
    uint8_t packet[] = "EAP error";

    Result result = StretchSensor::parseMeasurement(
            packet,
            sizeof(packet) - 1,
            measurement);

    EXPECT_EQ(result, Result::kMeasurementFailed);
}


TEST_F(ParseMeasurementsTest, RejectsNullBuffer)
{
    Result result = StretchSensor::parseMeasurement(
            nullptr,
            10,
            measurement);

    EXPECT_EQ(result, Result::kParseError);
}


TEST_F(ParseMeasurementsTest, RejectsUnknownPacketFormat)
{
    uint8_t packet[] = "INVALID";

    Result result = StretchSensor::parseMeasurement(
            packet,
            sizeof(packet) - 1,
            measurement);

    EXPECT_EQ(result, Result::kParseError);
}