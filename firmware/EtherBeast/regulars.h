#pragma once
#include <Arduino.h>
#include "forage.h"

// Regulars: devices heard in every forage (at most REGULAR_MAX_MISSED in a row
// missed) for REGULAR_AFTER_SEC. Home and office devices, and the ones you carry,
// end up here and are never eaten again - not even after the seen-device filter
// is cleared. Like the filter, only salted hashes are kept. Stored in LittleFS
// on the spare "spiffs" partition, since the streaks change every forage.

bool     regularsBegin();               // mount + load; false if the filesystem failed
bool     isRegular(uint64_t hash);
// Updates the streaks with this forage's sightings. `cycle` counts forages.
// Returns how many devices became regulars.
uint16_t regularsTrack(const ForageResult& r, uint32_t cycle, uint32_t epoch);
void     regularsSave();
void     regularsClear();               // forget everything (fresh egg)
uint16_t regularsCount();
uint16_t regularsCandidates();
