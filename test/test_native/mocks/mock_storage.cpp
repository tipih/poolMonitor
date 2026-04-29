// Storage for the mock-class static members.
#include "ModbusMaster.h"
#include "MQTTManager.h"

uint8_t  ModbusMaster::nextResult = ModbusMaster::ku8MBSuccess;
uint16_t ModbusMaster::responseBuffer[8] = {0};

std::string MQTTManager::lastSub;
std::string MQTTManager::lastVal;
int         MQTTManager::publishCount = 0;
