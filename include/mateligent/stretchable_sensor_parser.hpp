#include <cstdint>
#include <optional>

#include "etl/circular_buffer.h"
#include "etl/string.h"

#include "stretchable_sensor_data.hpp"

namespace Mateligent::StretchableSensor
{

class Parser
{
// A message can either be a pure ASCII response 
// ASCII responses are either terminated by one or two carriage returns
// One carriage return indicates an ECHO
// Two carriage returns indicate a RESP

// Or a binary data message that is 5 bytes long. THis is guranteed to have one byte 
public:
    bool feed(const uint8_t byte);
    std::optional<Message> consume();
    void reset();
private:
    uint8_t checksum_value_ = 0x00;
    etl::circular_buffer<uint8_t, 6> prev_bytes_;
    CalibratedMeasurement binary_measurement{};

    etl::string<16> cmd_str_;

    etl::circular_buffer<Message, 3> queued_messages_;
    std::optional<Message> feedAsciiParser(const uint8_t byte);
    std::optional<Message> feedBinaryParser(const uint8_t byte);
};

} // namespace Mateligent::StretchableSensor