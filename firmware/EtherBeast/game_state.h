#pragma once
#include <Arduino.h>

// Persistent monster state, saved to NVS as one blob. Timers are counted in
// forage cycles so they keep working before the RTC has been set by NTP.
struct GameState {
  uint32_t magic;
  uint16_t level;
  uint8_t  readyToEvolve;
  uint8_t  sugarCyclesLeft;    // Sugar Rush remaining
  uint32_t totalXp;
  uint32_t devicesEaten;
  uint32_t lastFedEpoch;       // UTC; 0 if the clock wasn't set at the time
  uint32_t cyclesSinceFed;
  uint32_t cyclesSinceSync;
  uint32_t cycles;             // forage cycles since hatching
  uint32_t displayCount;       // screen updates, drives full-refresh cadence
  uint16_t lastNewDevices;     // results of the latest forage
  uint16_t lastXpGain;
  uint8_t  lastComboX10;       // 10 = no combo, 15/20/30 = x1.5/x2.0/x3.0
  uint8_t  forceSync;          // level changed since the last upload
  uint8_t  digestions;         // times the seen-device filter was cleared
  uint8_t  lastSyncOk;         // 0 none yet, 1 ok, 2 failed
};

enum Face : uint8_t { FACE_NORMAL, FACE_HAPPY, FACE_SAD, FACE_SUGAR, FACE_SLEEP };

void        stateDefaults(GameState& s);
bool        stateLoad(GameState& s);          // false = fresh egg
void        stateSave(const GameState& s);

uint32_t    xpForLevel(uint16_t level);       // total XP needed to reach `level`
uint8_t     stageForLevel(uint16_t level);
const char* stageName(uint8_t stage);
bool        isStarving(const GameState& s);
bool        isSugarRush(const GameState& s);
uint8_t     faceFor(const GameState& s, bool sleeping);
const char* moodName(uint8_t face);
void        checkEvolveReady(GameState& s);

uint32_t    nowEpoch();                       // 0 if the RTC/system clock isn't set
