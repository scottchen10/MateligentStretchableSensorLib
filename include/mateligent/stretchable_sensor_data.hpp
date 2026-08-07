#include <cstdint>
#include <variant>
#include <stdio.h>
#include "etl/string_view.h"

namespace Mateligent::StretchableSensor
{

// Settings available to be read
// Defined in the "Commands:" section
// Citations to the specific command referenced are shown below
enum class Setting : uint8_t
{
    kUnknown,                // To handle cases where the sensor outputs a log message
    kCalibrationBurst,    // 3aii
    kConfigurationRecord, // 3aviii
    kSensorSleep,         // 3bvii
    kConfigurePwm,           // 3ai
    kCalibrationTempCoeff,   // 3aiii
    kCharacterData,          // 3aiv
    kConfigureLed,            // 3av
    kCalibrationCurrent,     // 3avi
    kConfigurePollingRate,   // 3avii
    kCalibrationSpan,        // 3aix
    kCalibrationTemperature, // 3ax
    kCalibrationZero,        // 3axi
    kSerialNumberOne, // 3bi
    kSerialNumberTwo, // 3bii
    kSensorAveraging, // 3biii
    kSensorFirmware,  // 3biv
    kSensorLength,    // 3bvi
    kSerialNumber,    // 3bvii
};

struct CommandMapping
{
    char command[3];
    Setting setting;
};

constexpr CommandMapping kCommandMappings[] = {
    {"CA", Setting::kConfigurePwm},
    {"CC", Setting::kCalibrationTempCoeff},
    {"CB", Setting::kCalibrationBurst},
    {"CR", Setting::kConfigurationRecord},
    {"SS", Setting::kSensorSleep},
    {"CD", Setting::kCharacterData},
    {"CL", Setting::kConfigureLed},
    {"CI", Setting::kCalibrationCurrent},
    {"CP", Setting::kConfigurePollingRate},
    {"CS", Setting::kCalibrationSpan},
    {"CT", Setting::kCalibrationTemperature},
    {"CZ", Setting::kCalibrationZero},
    {"S1", Setting::kSerialNumberOne},
    {"S2", Setting::kSerialNumberTwo},
    {"SA", Setting::kSensorAveraging},
    {"SF", Setting::kSensorFirmware},
    {"SL", Setting::kSensorLength},
    {"SN", Setting::kSerialNumber},
};

inline const char* settingToCommand(Setting setting)
{
    for (const auto& mapping : kCommandMappings)
    {
        if (mapping.setting == setting)
        {
            return mapping.command;
        }
    }

    return "";    
}

inline Setting commandToSetting(etl::string_view command)
{
    for (const auto& mapping : kCommandMappings)
    {
        if (mapping.command == command)
        {
            return mapping.setting;
        }
    }

    return Setting::kUnknown;
}

inline void buildSettingCommand(Setting setting, uint16_t count, char *buffer, size_t len)
{
    
    snprintf(buffer, len, "%s=%d\r", settingToCommand(setting), count);
}

inline void buildQueryCommand(Setting setting, char *buffer, size_t len)
{
    
    snprintf(buffer, len, "%s?\r", settingToCommand(setting));
}

// Settings can only be defined as string or 2 byte integer responses
struct StringSetting
{
    Setting setting;
    char value[32];
};

struct LogMessage
{
    char value[32];
};

struct IntegerSetting
{
    uint16_t value;
    Setting setting;
};

enum class MeasurementFormat : uint8_t
{
    kBinary = 0,
    kAscii
};

struct CalibratedMeasurement
{
    float percentage_stretch;
    float temperature_k;
    MeasurementFormat mode; 
    uint8_t status_flags;
};

struct RawMeasurement
{
    uint16_t raw_count;
    float temperature_k;
};

using Message = std::variant<CalibratedMeasurement,
                             RawMeasurement,
                             LogMessage,
                             StringSetting,
                             IntegerSetting>;

} // namespace Mateligent::StetchableSensor