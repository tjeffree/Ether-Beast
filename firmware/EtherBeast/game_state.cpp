#include "game_state.h"
#include "config.h"
#include "forage.h"
#include <Preferences.h>
#include <time.h>

static const uint32_t STATE_MAGIC = 0xE7BE0002;
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

uint8_t tierForLevel(uint16_t level) {
  if (level >= TIER5_LEVEL) return 5;
  if (level >= TIER4_LEVEL) return 4;
  if (level >= TIER3_LEVEL) return 3;
  if (level >= TIER2_LEVEL) return 2;
  if (level >= 2) return 1;
  return 0;
}

static bool atLeastPct(uint32_t part, uint32_t whole, uint32_t pct) {
  return whole > 0 && (uint64_t)part * 100 >= (uint64_t)whole * pct;
}

uint8_t chooseForm(const GameState& s, uint8_t tier) {
  const uint32_t* k = s.tierEatenByKind;
  uint32_t total = k[KIND_WIFI_CLIENT] + k[KIND_WIFI_AP] + k[KIND_BLE_OTHER] + k[KIND_BLE_WEARABLE];
  switch (tier) {
    case 0: return FORM_EGG;
    case 1: return FORM_STATIC_BLOB;
    case 2:  // diet
      if (atLeastPct(k[KIND_BLE_WEARABLE], total, DIET_WEARABLE_PCT)) return FORM_PULSE_LEECH;
      if (k[KIND_BLE_OTHER] + k[KIND_BLE_WEARABLE] > k[KIND_WIFI_AP] + k[KIND_WIFI_CLIENT])
        return FORM_BLUE_GREMLIN;
      return FORM_BEACON_IMP;
    case 3:  // lifestyle
      if (atLeastPct(s.tierEatenAtNight, total, NIGHT_FEEDER_PCT)) return FORM_STATIC_MOTH;
      if (atLeastPct(s.tierEatenAway, total, ROAMER_PCT)) return FORM_PACKET_HOUND;
      return FORM_ROUTER_GOLEM;
    case 4:  // temperament
      if (s.tierRecoveries) return FORM_HOLLOW_WRAITH;
      if (s.tierFrenzies >= HYDRA_FRENZIES) return FORM_MESH_HYDRA;
      return FORM_FIREWALL_DRAKE;
    default:
      return s.secretUnlocked ? FORM_PROMISCUOUS_ONE : FORM_ETHER_WYRM;
  }
}

void enterLevel(GameState& s) {
  uint8_t tier = tierForLevel(s.level);
  if (tier == tierForLevel(s.level - 1)) return;
  s.form = chooseForm(s, tier);
  s.path[tier] = s.form;
  memset(s.tierEatenByKind, 0, sizeof(s.tierEatenByKind));
  s.tierEatenAway = 0;
  s.tierEatenAtNight = 0;
  s.tierFrenzies = 0;
  s.tierRecoveries = 0;
}

uint8_t displayForm(const GameState& s) {
  if (s.cycles > 0 && (uint64_t)s.cyclesSinceFed * FORAGE_INTERVAL_SEC >= NULL_BYTE_AFTER_SEC)
    return FORM_NULL_BYTE;
  if (s.silentStreak >= FARADAY_SILENT_FORAGES) return FORM_FARADAY_GHOST;
  return s.form;
}

uint8_t spriteFor(const GameState& s) {
  uint8_t form = displayForm(s);
  if (form != FORM_MESH_HYDRA) return form;
  uint16_t extraHeads = s.tierFrenzies / HYDRA_HEAD_FRENZIES;
  if (extraHeads >= 2) return SPRITE_HYDRA_3;
  if (extraHeads == 1) return SPRITE_HYDRA_2;
  return FORM_MESH_HYDRA;
}

uint8_t teaserSprite(const GameState& s) {
  if (!s.readyToEvolve) return NO_SPRITE;
  uint8_t next = tierForLevel(s.level + 1);
  if (next == tierForLevel(s.level)) return NO_SPRITE;
  return chooseForm(s, next);
}

const char* formName(uint8_t form) {
  static const char* names[FORM_COUNT] = {
    "Egg", "Static Blob", "Beacon Imp", "Blue Gremlin", "Pulse Leech", "Packet Hound",
    "Router Golem", "Static Moth", "Mesh Hydra", "Firewall Drake", "Hollow Wraith",
    "Ether Wyrm", "Promiscuous One", "Null Byte", "Faraday Ghost",
  };
  return form < FORM_COUNT ? names[form] : "???";
}

const char* formKey(uint8_t form) {
  static const char* keys[FORM_COUNT] = {
    "egg", "static_blob", "beacon_imp", "blue_gremlin", "pulse_leech", "packet_hound",
    "router_golem", "static_moth", "mesh_hydra", "firewall_drake", "hollow_wraith",
    "ether_wyrm", "promiscuous_one", "null_byte", "faraday_ghost",
  };
  return form < FORM_COUNT ? keys[form] : "unknown";
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

int localHour() {
  time_t t = nowEpoch();
  if (!t) return -1;
  setenv("TZ", LOCAL_TZ, 1);
  tzset();
  struct tm tm;
  localtime_r(&t, &tm);
  return tm.tm_hour;
}
