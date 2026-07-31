#include <cstdint>
#include "etl/queue.h"
#include "etl/circular_buffer.h"

#include "stretchable_sensor_data.hpp"

namespace Mateligent::StretchableSensor
{

class Parser
{

enum class State
{
    
};

public:
    void feed(const uint8_t byte);

private:
    etl::circular_buffer<int, 8> parse_events_;
    
};

} // namespace Mateligent::StretchableSensor