#pragma once
#include <Arduino.h>
#include "config.h"

enum SightingKind : uint8_t { KIND_WIFI_CLIENT, KIND_WIFI_AP, KIND_BLE_OTHER, KIND_BLE_WEARABLE };

struct Sighting {
  uint64_t hash;      // salted hash of the device address - never the address itself
  uint8_t  kind;
};

struct ForageResult {
  Sighting items[MAX_SIGHTINGS];
  uint16_t count;
  bool     homeSeen;
};

uint8_t xpForKind(uint8_t kind);

// Passive listening only: nothing is transmitted during either scan.
void forageBle(ForageResult& r);
void forageWifi(ForageResult& r, const char* homeSsid);
