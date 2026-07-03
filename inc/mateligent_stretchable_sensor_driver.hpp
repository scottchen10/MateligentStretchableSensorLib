/**
 * filename:    mateligent_stretchable_sensor_driver.h
 * description: Implements the Mateligent Flex Sensor communication interface. See folder /docs/ for more details.
 * author:      Scott Chen
 * date:        07/03/2026
 */

#include <cstdint>

namespace MateligentSensors
{

struct PlatformUart
{
    // write(const uint8_t * data, size_t len)
    // read(uint8_t * buffer, size_t buf_len, uint32_t read_len)
    uint32_t (*write)(const uint8_t*, size_t);
    uint32_t (*read)(uint8_t*, size_t, uint32_t);
};

}