// In-memory Preferences stub for native unit tests.
#pragma once

#include <cstdint>
#include <map>
#include <string>

class Preferences {
public:
  bool begin(const char* ns, bool /*readOnly*/ = false) { _ns = ns ? ns : ""; return true; }
  void end() {}

  uint8_t getUChar(const char* key, uint8_t def = 0) {
    auto it = _u8.find(_k(key));
    return it == _u8.end() ? def : it->second;
  }
  size_t putUChar(const char* key, uint8_t v) { _u8[_k(key)] = v; return 1; }

  uint32_t getULong(const char* key, uint32_t def = 0) {
    auto it = _u32.find(_k(key));
    return it == _u32.end() ? def : it->second;
  }
  size_t putULong(const char* key, uint32_t v) { _u32[_k(key)] = v; return 4; }

private:
  std::string _ns;
  std::map<std::string, uint8_t>  _u8;
  std::map<std::string, uint32_t> _u32;
  std::string _k(const char* key) const { return _ns + "/" + (key ? key : ""); }
};
