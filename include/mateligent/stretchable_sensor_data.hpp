#include <cstdint>
#include <variant>
#include "etl/string.h"

namespace Mateligent::StretchableSensor
{

// Single event commands
// Defined in the "Commands" sectin
// Citations to the specific command referenced are shown below
enum class Command : uint8_t
{
    kCalibrationBurst,    // 3aii
    kConfigurationRecord, // 3aviii
    kSensorSleep,         // 3bvii
};

// Settings available to be read
// Defined in the "Commands:" section
// Citations to the specific command referenced are shown below
enum class Setting : uint8_t
{
    kUnknown,                // To handle cases where the sensor outputs a log message
    kConfigurePwm,           // 3ai
    kCalibrationTempCoeff,   // 3aiii
    kCharacterData,          // 3aiv
    kConfigreLed,            // 3av
    kCalibrationCurrent,     // 3avi
    kConfigurePollingRate,   // 3avii
    kCalibrationTemperature, // 3ax
    kCalibrationZero,        // 3axi
    kSerialNumberOne, // 3bi
    kSerialNumberTwo, // 3bii
    kSensorAveraging, // 3biii
    kSensorFirmware,  // 3biv
    kSensorLength,    // 3bvi
    kSensorNumber,    // 3bvii
};

// Settings can only be defined as string or 2 byte integer responses
struct StringSetting
{
    Setting setting;
    char value[16];
};

struct LogMessage
{
    char value[16];
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
                             StringSetting,
                             LogMessage,
                             IntegerSetting>;

} // namespace Mateligent::StetchableSensor