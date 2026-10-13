#pragma once
#include <Arduino.h>
#include "config.h"

// Devices are only ever remembered as a salted 64-bit hash of their address;
// the raw address is discarded as soon as it has been hashed. The salt is random
// per device and never leaves it, so the stored bits can't be matched against
// a list of known MAC addresses.

void     seenSaltInit();                       // load or create the salt
uint64_t hashAddress(const uint8_t addr[6]);   // MurmurHash64A(salt || addr)

class SeenFilter {
public:
  void load();
  void save();
  // Marks the hash as eaten. Returns true if it was new.
  bool eat(uint64_t hash);
  // Clears the filter once it is too full to be accurate. Returns true if cleared.
  bool digestIfFull();
  uint32_t count() const { return _count; }

private:
  uint8_t  _bits[SEEN_FILTER_BYTES];
  uint32_t _count = 0;
};
