#pragma once
#include <Arduino.h>
#include "game_state.h"

String base64Encode(const uint8_t* data, size_t len);

// Builds data.json for the dashboard.
String buildStatePayload(const GameState& s, uint8_t face, uint8_t battery);

// Joins home Wi-Fi, sets the RTC from NTP, and PUTs data.json to the repo.
// Returns true on success; `error` describes any failure.
bool syncToGitHub(const GameState& s, uint8_t face, uint8_t battery, String& error);
