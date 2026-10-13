#include "game_state.h"
#include "config.h"
#include <Preferences.h>
#include <time.h>

static const uint32_t STATE_MAGIC = 0xE7BE0001;
static const char* NVS_NS = "beast";

void stateDefaults(GameState& s) {
  memset(&s, 0, sizeof(s));
  s.magic = STATE_MAGIC;
  s.level = 1;
  s.lastComboX10 = 10;
  s.cyclesSinceSync = UINT32_MAX / 2;  // sync on the first visit home
}

bool stateLoad(GameState& s) {
  Preferences p;
  p.begin(NVS_NS, true);
  size_t n = p.getBytes("state", &s, sizeof(s));
  p.end();
  if (n != sizeof(s) || s.magic != STATE_MAGIC) {
    stateDefaults(s);
    return false;
  }
  return true;
}

void stateSave(const GameState& s) {
  Preferences p;
  p.begin(NVS_NS, false);
  p.putBytes("state", &s, sizeof(s));
  p.end();
}

uint32_t xpForLevel(uint16_t level) {
  return (uint32_t)LEVEL_XP_STEP * level * (level - 1);
}

uint8_t stageForLevel(uint16_t level) {
  if (level <= 1) return 0;
  if (level <= 3) return 1;
  if (level <= 6) return 2;
  if (level <= 9) return 3;
  return 4;
}

const char* stageName(uint8_t stage) {
  static const char* names[] = { "Egg", "Blob", "Imp", "Brute", "Ether Wyrm" };
  return stage < 5 ? names[stage] : "???";
}

bool isStarving(const GameState& s) {
  return s.cycles > 0 &&
         (uint64_t)s.cyclesSinceFed * FORAGE_INTERVAL_SEC >= STARVING_AFTER_SEC;
}

bool isSugarRush(const GameState& s) {
  return s.sugarCyclesLeft > 0;
}

uint8_t faceFor(const GameState& s, bool sleeping) {
  if (isSugarRush(s)) return FACE_SUGAR;
  if (isStarving(s)) return FACE_SAD;
  if (s.lastNewDevices >= FEEDING_MIN_NEW) return FACE_HAPPY;
  if (sleeping) return FACE_SLEEP;
  return FACE_NORMAL;
}

const char* moodName(uint8_t face) {
  static const char* names[] = { "content", "feeding", "starving", "sugar_rush", "sleeping" };
  return face < 5 ? names[face] : "unknown";
}

void checkEvolveReady(GameState& s) {
  // The beast never evolves on its own - it waits for the top button.
  s.readyToEvolve = s.totalXp >= xpForLevel(s.level + 1);
}

uint32_t nowEpoch() {
  time_t t = time(nullptr);
  return t > 1700000000 ? (uint32_t)t : 0;
}
