#include "mateligent/stretchable_sensor_parser.hpp"
#include "gtest/gtest.h"
#include <optional>
#include <variant>

using namespace Mateligent::StretchableSensor;

class ParserTest : public ::testing::Test
{
protected:
    Parser parser_;

    std::optional<Message> feed(std::string_view str)
    {
        std::optional<Message> result;

        for (uint8_t byte : str)
        {
            auto msg = parser_.feed(byte);
            if (msg)
            {
                result = std::move(msg);
            }
        }

        return result;
    }

    std::optional<Message> feed(std::initializer_list<uint8_t> bytes)
    {
        std::optional<Message> result;

        for (uint8_t byte : bytes)
        {
            auto msg = parser_.feed(byte);
            if (msg)
            {
                result = std::move(msg);
            }
        }

        return result;
    }
};


TEST_F(ParserTest, ParsesCalibratedAsciiMeasurement)
{
    std::optional<Message> msg = feed("123.45%,295.5K\r");

    ASSERT_TRUE(msg.has_value());
    auto* measurement = std::get_if<CalibratedMeasurement>(&*msg);
    ASSERT_NE(measurement, nullptr);
    EXPECT_FLOAT_EQ(measurement->percentage_stretch, 123.45f);
    EXPECT_FLOAT_EQ(measurement->temperature_k, 295.5f);
    EXPECT_EQ(measurement->mode, MeasurementFormat::kAscii);
}

TEST_F(ParserTest, ParsesCalibratedAsciiMeasurementWithSpaces)
{
    std::optional<Message> msg = feed("    .45%, 295.5K\r");

    ASSERT_TRUE(msg.has_value());
    auto* measurement = std::get_if<CalibratedMeasurement>(&*msg);
    ASSERT_NE(measurement, nullptr);
    EXPECT_FLOAT_EQ(measurement->percentage_stretch, 0.45f);
    EXPECT_FLOAT_EQ(measurement->temperature_k, 295.5f);
    EXPECT_EQ(measurement->mode, MeasurementFormat::kAscii);
}

TEST_F(ParserTest, ParsesNoisyCalibratedAsciiMeasurementWithSpaces)
{
    std::optional<Message> msg = feed("AXInitializing..    .45%, 295.5K\r\r\r");

    ASSERT_TRUE(msg.has_value());
    auto* measurement = std::get_if<CalibratedMeasurement>(&*msg);
    ASSERT_NE(measurement, nullptr);
    EXPECT_FLOAT_EQ(measurement->percentage_stretch, 0.45f);
    EXPECT_FLOAT_EQ(measurement->temperature_k, 295.5f);
    EXPECT_EQ(measurement->mode, MeasurementFormat::kAscii);
}

TEST_F(ParserTest, ParsesRawAsciiMeasurement)
{
    std::optional<Message> msg = feed("10211, 295.5K\r");

    ASSERT_TRUE(msg.has_value());
    auto* measurement = std::get_if<RawMeasurement>(&*msg);
    ASSERT_NE(measurement, nullptr);
    EXPECT_EQ(measurement->raw_count, 10211);
    EXPECT_FLOAT_EQ(measurement->temperature_k, 295.5f);
}

TEST_F(ParserTest, ParsesBinaryMeasurement)
{
    std::optional<Message> msg = feed({0x0b, 0xb9, 0x00, 0x41, 0x00, 0x05});

    ASSERT_TRUE(msg.has_value());
    auto* measurement = std::get_if<CalibratedMeasurement>(&*msg);
    ASSERT_NE(measurement, nullptr);
    EXPECT_FLOAT_EQ(measurement->percentage_stretch, 0.65f);
    EXPECT_FLOAT_EQ(measurement->temperature_k, 300.1f);
    EXPECT_EQ(measurement->mode, MeasurementFormat::kBinary);
}

TEST_F(ParserTest, ParsesNoisyBinaryMeasurement)
{
    std::optional<Message> msg = feed({0x12, 0x21, 0x0b, 0xb9, 0x00, 0x41, 0x00, 0x05});

    ASSERT_TRUE(msg.has_value());
    auto* measurement = std::get_if<CalibratedMeasurement>(&*msg);
    ASSERT_NE(measurement, nullptr);
    EXPECT_FLOAT_EQ(measurement->percentage_stretch, 0.65f);
    EXPECT_FLOAT_EQ(measurement->temperature_k, 300.1f);
    EXPECT_EQ(measurement->mode, MeasurementFormat::kBinary);
}

TEST_F(ParserTest, ParsesCommand)
{
    std::optional<Message> msg = feed("CP=1000\r\r");
    auto* settings = std::get_if<IntegerSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kConfigurePollingRate);
    EXPECT_EQ(settings->value, 1000);
}

TEST_F(ParserTest, ParsesConfigurePwm)
{
    std::optional<Message> msg = feed("CA=100\r\r");

    auto* settings = std::get_if<IntegerSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kConfigurePwm);
    EXPECT_EQ(settings->value, 100);
}

TEST_F(ParserTest, ParsesCalibrationTempCoeff)
{
    std::optional<Message> msg = feed("CC=200\r\r");

    auto* settings = std::get_if<IntegerSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kCalibrationTempCoeff);
    EXPECT_EQ(settings->value, 200);
}

TEST_F(ParserTest, ParsesCharacterData)
{
    std::optional<Message> msg = feed("CD=1\r\r");

    auto* settings = std::get_if<IntegerSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kCharacterData);
    EXPECT_EQ(settings->value, 1);
}

TEST_F(ParserTest, ParsesConfigureLed)
{
    std::optional<Message> msg = feed("CL=2\r\r");

    auto* settings = std::get_if<IntegerSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kConfigreLed);
    EXPECT_EQ(settings->value, 2);
}

TEST_F(ParserTest, ParsesCalibrationCurrent)
{
    std::optional<Message> msg = feed("CI=500\r\r");

    auto* settings = std::get_if<IntegerSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kCalibrationCurrent);
    EXPECT_EQ(settings->value, 500);
}

TEST_F(ParserTest, ParsesConfigurePollingRate)
{
    std::optional<Message> msg = feed("CP=1000\r\r");

    auto* settings = std::get_if<IntegerSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kConfigurePollingRate);
    EXPECT_EQ(settings->value, 1000);
}

TEST_F(ParserTest, ParsesCalibrationSpan)
{
    std::optional<Message> msg = feed("CS=5000\r\r");

    auto* settings = std::get_if<IntegerSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kCalibrationSpan);
    EXPECT_EQ(settings->value, 5000);
}

TEST_F(ParserTest, ParsesCalibrationTemperature)
{
    std::optional<Message> msg = feed("CT=295\r\r");

    auto* settings = std::get_if<IntegerSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kCalibrationTemperature);
    EXPECT_EQ(settings->value, 295);
}

TEST_F(ParserTest, ParsesCalibrationZero)
{
    std::optional<Message> msg = feed("CZ=0\r\r");

    auto* settings = std::get_if<IntegerSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kCalibrationZero);
    EXPECT_EQ(settings->value, 0);
}

TEST_F(ParserTest, ParsesSerialNumberOne)
{
    std::optional<Message> msg = feed("S1=123456\r\r");

    auto* settings = std::get_if<StringSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kSerialNumberOne);
    EXPECT_STREQ(settings->value, "123456");
}

TEST_F(ParserTest, ParsesSerialNumberTwo)
{
    std::optional<Message> msg = feed("S2=789012\r\r");

    auto* settings = std::get_if<StringSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kSerialNumberTwo);
    EXPECT_STREQ(settings->value, "789012");
}

TEST_F(ParserTest, ParsesSensorAveraging)
{
    std::optional<Message> msg = feed("SA=16\r\r");

    auto* settings = std::get_if<IntegerSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kSensorAveraging);
    EXPECT_EQ(settings->value, 16);
}

TEST_F(ParserTest, ParsesSensorFirmware)
{
    std::optional<Message> msg = feed("SF=1.2.3\r\r");

    auto* settings = std::get_if<StringSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kSensorFirmware);
    EXPECT_STREQ(settings->value, "1.2.3");
}

TEST_F(ParserTest, ParsesSensorLength)
{
    std::optional<Message> msg = feed("SL=50\r\r");

    auto* settings = std::get_if<IntegerSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kSensorLength);
    EXPECT_EQ(settings->value, 50);
}

TEST_F(ParserTest, ParsesSerialNumber)
{
    std::optional<Message> msg = feed("SN=ABC123\r\r");

    auto* settings = std::get_if<StringSetting>(&*msg);
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->setting, Setting::kSerialNumber);
    EXPECT_STREQ(settings->value, "ABC123");
}