#pragma once
#include <Arduino.h>

// Forms, in sprite order (sprites/body_<n>_*.txt). Two extra sprites after the
// last form hold the Mesh Hydra with two and three heads.
enum Form : uint8_t {
  FORM_EGG, FORM_STATIC_BLOB,
  FORM_BEACON_IMP, FORM_BLUE_GREMLIN, FORM_PULSE_LEECH,           // tier 2: diet
  FORM_PACKET_HOUND, FORM_ROUTER_GOLEM, FORM_STATIC_MOTH,         // tier 3: lifestyle
  FORM_MESH_HYDRA, FORM_FIREWALL_DRAKE, FORM_HOLLOW_WRAITH,       // tier 4: temperament
  FORM_ETHER_WYRM, FORM_PROMISCUOUS_ONE,                          // tier 5: final
  FORM_NULL_BYTE, FORM_FARADAY_GHOST,                             // temporary conditions
  FORM_COUNT
};
static const uint8_t SPRITE_HYDRA_2 = FORM_COUNT;
static const uint8_t SPRITE_HYDRA_3 = FORM_COUNT + 1;
static const uint8_t TIER_COUNT = 6;
static const uint8_t NO_SPRITE = 0xFF;

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
  uint8_t  digestions;         // times the seen-device filter was cleared (scars)
  uint8_t  lastSyncOk;         // 0 none yet, 1 ok, 2 failed

  // Evolution
  uint8_t  form;               // current Form (not counting Null Byte / Faraday Ghost)
  uint8_t  path[TIER_COUNT];   // form taken at each tier
  uint8_t  secretUnlocked;     // ate all four kinds of device in one frenzy
  uint16_t silentStreak;       // forages in a row that heard nothing at all
  uint16_t frenzies;           // lifetime x3.0 combos
  uint32_t eatenByKind[4];     // lifetime diet, by SightingKind
  // What it ate in its current form; decides the next branch, then resets.
  uint32_t tierEatenByKind[4];
  uint32_t tierEatenAway;
  uint32_t tierEatenAtNight;
  uint16_t tierFrenzies;
  uint8_t  tierRecoveries;     // times it was starving and then fed
};

enum Face : uint8_t { FACE_NORMAL, FACE_HAPPY, FACE_SAD, FACE_SUGAR, FACE_SLEEP };

void        stateDefaults(GameState& s);
bool        stateLoad(GameState& s);          // false = fresh egg
void        stateSave(const GameState& s);

uint32_t    xpForLevel(uint16_t level);       // total XP needed to reach `level`
uint8_t     tierForLevel(uint16_t level);
uint8_t     chooseForm(const GameState& s, uint8_t tier);  // form it would take on entering `tier`
void        enterLevel(GameState& s);         // after s.level++: changes form at a new tier
uint8_t     displayForm(const GameState& s);  // form, or Null Byte / Faraday Ghost
uint8_t     spriteFor(const GameState& s);    // body sprite for displayForm()
uint8_t     teaserSprite(const GameState& s); // next form if the next level changes it, else NO_SPRITE
const char* formName(uint8_t form);
const char* formKey(uint8_t form);            // snake_case id for data.json
bool        isStarving(const GameState& s);
bool        isSugarRush(const GameState& s);
uint8_t     faceFor(const GameState& s, bool sleeping);
const char* moodName(uint8_t face);
void        checkEvolveReady(GameState& s);

uint32_t    nowEpoch();                       // 0 if the RTC/system clock isn't set
int         localHour();                      // UK local hour, -1 if the clock isn't set
