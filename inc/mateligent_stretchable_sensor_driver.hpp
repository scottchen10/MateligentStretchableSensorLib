/**
 * filename:    mateligent_stretchable_sensor_driver.h
 * description: Implements the Mateligent Flex Sensor communication interface for the AX-008-05 Firmware.
 *              See folder /docs/ for more details.
 * author:      Scott Chen  
 * date:        07/06/2026
 */

#include <cstdint>
#include <variant>

namespace Mateligent
{

enum class Result: uint8_t
{
    kSuccess,
    kParseError,
    kUartError,
    kTimeout,
    kChecksumError,
    kMeasurementFailed,
    kCalibrationNotComplete,
};

struct PlatformUart
{
    Result (*write)(const uint8_t *data, size_t len);
    Result (*read)(uint8_t *out_buffer, size_t buf_len, size_t& out_bytes_read, uint32_t timeout_ms);
};

class StretchSensor 
{
public:
    enum class LedConfig : uint8_t
    {
        kNone = 0,
        kBlinkOnMeasurement,
        kOther,
    };

    enum class OutputMode : uint8_t
    {
        kBinary = 0,
        kAscii,
    };
    
    struct CalibratedMeasurement
    {
        float percentage_stretch;
        float temperature_k;
        OutputMode mode; 
        uint8_t status_flags;
    };

    struct RawMeasurement
    {
        uint16_t raw_count;
        float temperature_k;
    };

    explicit StretchSensor(const PlatformUart& uart_interface): uart_(uart_interface) {}

    // Measurements
    static Result parseMeasurement(const uint8_t *data, size_t len, std::variant<CalibratedMeasurement, RawMeasurement>& out_measurement);

    // Calibration
    Result performZeroCalibration();
    Result performSpanCalibration();

    Result setZeroCalibrationValue(uint16_t value);
    Result getZeroCalibrationValue(uint16_t& out_value);

    Result setSpanCalibrationValue(uint16_t value);
    Result getSpanCalibrationValue(uint16_t& out_value);

    Result setCalibrationTemperature(uint16_t temperature_tenths_k);
    Result getCalibrationTemperature(uint16_t& out_temperature_tenths_k);

    Result setCalibrationTemperatureCoefficient(uint16_t coefficient);
    Result getCalibrationTemperatureCoefficient(uint16_t& out_coefficient);

    Result calibrationBurst(uint8_t exponent);
    Result calibrationErase();

    // Configuration
    Result setPollingPeriod(uint16_t period_ms);
    Result getPollingPeriod(uint16_t& out_period_ms);

    Result setOversampling(uint8_t exponent);
    Result getOversampling(uint8_t& out_exponent);

    Result setLedConfig(LedConfig config);
    Result getLedConfig(LedConfig& out_config);

    Result setOutputDataType(OutputMode type);
    Result getOutputDataType(OutputMode& out_type);

    Result setCalibrationCurrent(uint16_t current_nA);
    Result getCalibrationCurrent(uint16_t& out_current_nA);

    Result setPwmOutput(bool enabled);

    Result writeConfigToFlash();

    // Sensor Information
    Result getFirmwareVersion(char* buffer, size_t buffer_len);

    Result setSerialNumber(uint32_t serial_number);
    Result getSerialNumber(uint32_t& out_serial_number);

    Result setSensorLength(uint16_t length_mm);
    Result getSensorLength(uint16_t& out_length_mm);

    Result sensorSleep();
    Result enterBootloader();
private:
    Result sendCommandAndGetResponse(const char* cmd, size_t cmd_len, char* out_buf, size_t buf_len, uint32_t timeout_ms=10);
    Result setUintCommand(const char* command, uint32_t value);
    Result queryUintCommand(const char* command, uint16_t& value);
    Result setBoolCommand(const char* command, bool value);
    Result sendSimpleCommand(const char* command);
   
    PlatformUart uart_;
};

} // namespace Mateligent