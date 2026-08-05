#include "mateligent/stretchable_sensor_parser.hpp"
#include <cctype>
#include <etl/circular_buffer.h>
#include <optional>
#include "etl/to_arithmetic.h"

namespace 
{
using namespace Mateligent::StretchableSensor;

bool isValidAsciiCharacter(const char character)
{
    return std::isalnum(character) ||
           character == '%' ||
           character == '.' ||
           character == ' ' ||
           character == '\r';
}

constexpr uint16_t kMinTemperatureDeciKelvin = 1731;
constexpr uint16_t kMaxTemperatureDeciKelvin = 3731;
constexpr uint16_t kMaxStrainHundrethsPercent = 0x7FFF;
constexpr uint16_t kErrorStrainValue = 0x8000;
constexpr uint8_t kValidStatus1 = 0x01;
constexpr uint8_t kValidStatus2 = 0x20;

bool isValidBinaryTemperature(const uint16_t binary_temp)
{
    return binary_temp >= kMinTemperatureDeciKelvin && binary_temp <= kMaxTemperatureDeciKelvin;
}

bool isValidBinaryStrain(const uint16_t binary_strain)
{
    return binary_strain <= kMaxStrainHundrethsPercent || binary_strain == kErrorStrainValue;
}

bool isValidBinaryStatus(const uint8_t status)
{
    return status == kValidStatus1 || status == kValidStatus2;
}

} // namespace

namespace Mateligent::StretchableSensor
{


std::optional<Message> Parser::feedAsciiParser(const uint8_t byte)
{
    constexpr size_t kMinimumValidFrameSize = 3;
    
    const char character = static_cast<const char>(byte);
    const bool string_overflowed = ascii_str_.full();
    const bool frame_completed = character == '\r';
    
    // Framing
    if (!isValidAsciiCharacter(character) || string_overflowed)
    {
        ascii_str_.erase(0, ascii_str_.size());
    }
    else if (character != ' ' && character != '\r') 
    {
        ascii_str_.push_back(character);
    }
    
    if (!frame_completed)
    {
        return std::nullopt;
    }

    // Parsing a complete frame
    if (frame_completed && ascii_str_.size() <= kMinimumValidFrameSize)
    {
        ascii_str_.erase(0, ascii_str_.size());
        return std::nullopt;
    }

    // Identify first the type of frame
    // ex: C#= or S#= is a resp or echo
    //     AX-008##### is the version
    //     000.00%,000.0K is a calibrated measurement
    //     00000,000.0K for uncalibrated measurement
    //     Anything else is an arbitruary log message

    const bool has_comma   = ascii_str_.contains(',');
    const bool has_decimal = ascii_str_.contains('.');
    const bool has_percent = ascii_str_.contains('%');
    const bool ends_with_k = !ascii_str_.empty() && ascii_str_.back() == 'K';

    etl::string_view command_identifier    = etl::string_view(ascii_str_.data(), 2);
    const bool contains_command_prefix     = commandToSetting(command_identifier) != Setting::kUnknown && ascii_str_.at(2) == '=';
    const bool is_firmware_version         = ascii_str_.starts_with("AX-008");
    const bool is_calibrated_measurement   = has_comma && has_decimal && has_percent && ends_with_k;
    const bool is_uncalibrated_measurement = has_comma && has_decimal && !has_percent && ends_with_k;;
    
    std::optional<Message> msg = std::nullopt;

    auto createLogMessage = [](etl::string<16>& str) -> LogMessage {
        LogMessage log{};
        str.copy(log.value, sizeof(log.value));
        return log;
    };

    if (contains_command_prefix)
    {
        etl::string_view postfix_str = etl::string_view(ascii_str_);
        postfix_str.remove_prefix(3);
        etl::to_arithmetic_result result =  etl::to_arithmetic<uint16_t>(postfix_str);
        
        Setting setting = commandToSetting(etl::string_view(ascii_str_.data(), 2));
        
        if (result.has_value())
        {
            IntegerSetting data {
                .value = 0xFFFF,
                .setting = setting
            };

            msg = data;
        }
        else 
        {
            StringSetting data{};
            postfix_str.copy(data.value, sizeof(data.value));
            data.setting = setting;

            msg = data;
        }
    }
    else if (is_firmware_version)
    {
        StringSetting data = {};
        ascii_str_.copy(data.value, sizeof(data.value));
        data.setting = Setting::kSensorFirmware;

        msg = data;
    }
    else if (is_calibrated_measurement)
    {
        size_t strain_end = ascii_str_.find('%');
        size_t temp_end = ascii_str_.find("K");

        etl::string_view strain_str = etl::string_view(ascii_str_.data(), strain_end);
        etl::string_view temp_str = etl::string_view(ascii_str_.data() + strain_end + 2, temp_end - strain_end - 2);

        etl::to_arithmetic_result strain_res =  etl::to_arithmetic<float>(strain_str);
        etl::to_arithmetic_result temp_res =  etl::to_arithmetic<float>(temp_str);

        if (!strain_res.has_value() || !temp_res.has_value())
        {
            msg = createLogMessage(ascii_str_);
        }
        else
        {
            msg = CalibratedMeasurement{
                .percentage_stretch = strain_res.value(),
                .temperature_k = temp_res.value(),
                .mode = MeasurementFormat::kAscii,
                .status_flags = 0x00
            };
        }
    }
    else if (is_uncalibrated_measurement)
    {
        RawMeasurement raw_measurement{};

        size_t strain_end = ascii_str_.find(',');
        size_t temp_end = ascii_str_.find("K");

        etl::string_view raw_strain_str = etl::string_view(ascii_str_.data(), strain_end);
        etl::string_view temp_str = etl::string_view(ascii_str_.data() + strain_end + 1, temp_end - strain_end - 1);

        etl::to_arithmetic_result raw_res =  etl::to_arithmetic<uint16_t>(raw_strain_str);
        etl::to_arithmetic_result temp_res =  etl::to_arithmetic<float>(temp_str);

        if (!raw_res.has_value() || !temp_res.has_value())
        {
            msg = createLogMessage(ascii_str_);
        }
        else
        {
            msg = RawMeasurement{
                .raw_count = raw_res.value(),
                .temperature_k = temp_res.value()
            };    
        }
    }
    else
    {
        msg = createLogMessage(ascii_str_);
    }

    return msg;
}

std::optional<Message> Parser::feedBinaryParser(const uint8_t byte)
{
    if (!prev_bytes_.full())
    {
        prev_bytes_.push(byte);
        checksum_value_ += byte;
        return std::nullopt;
    }

    const uint16_t temperature =(static_cast<uint16_t>(prev_bytes_[0]) << 8) | static_cast<uint16_t>(prev_bytes_[1]);
    const uint16_t strain = (static_cast<uint16_t>(prev_bytes_[2]) << 8) | static_cast<uint16_t>(prev_bytes_[3]);
    const uint8_t status = prev_bytes_[4];
    const uint8_t checksum = prev_bytes_[5];


    if (!isValidBinaryTemperature(temperature) || 
        !isValidBinaryStrain(strain) ||
        !isValidBinaryStatus(status) ||
        checksum != checksum_value_)
    {
        uint8_t value = prev_bytes_.front();
        checksum_value_ -= value;
        prev_bytes_.pop();        
        return std::nullopt;
    }

    return CalibratedMeasurement{
        .percentage_stretch = static_cast<float>(strain) / 100.0f,
        .temperature_k = static_cast<float>(temperature) / 10.0f,
        .mode=MeasurementFormat::kBinary,
        .status_flags = status,
    };
}

std::optional<Message> Parser::feed(const uint8_t byte)
{
    std::optional<Message> ascii_parser_result = feedAsciiParser(byte);
    std::optional<Message> binary_parser_result = feedBinaryParser(byte);

    if (ascii_parser_result.has_value())
    {
        return ascii_parser_result;
    }
    else if (binary_parser_result.has_value())
    {
        return binary_parser_result;
    }

    return std::nullopt;
}


} // namespace Mateligent::StretchableSensor