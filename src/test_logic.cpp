#include "hud/E2EProtection.hpp"
#include <iostream>
int main() {
    uint8_t data[] = {1, 2, 3};
    uint8_t crc = adas::hud::E2EChecker::compute_crc8(data, 3);
    std::cout << "CRC: " << (int)crc << std::endl;
    return 0;
}
