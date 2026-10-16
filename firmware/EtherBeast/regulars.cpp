#include "regulars.h"
#include "config.h"
#include <LittleFS.h>

static const char* CANDIDATES_FILE = "/candidates.bin";
static const char* REGULARS_FILE = "/regulars.bin";

struct Candidate {
  uint64_t hash;
  uint32_t firstCycle;   // start of the current unbroken streak
  uint32_t lastCycle;    // latest forage it was heard in
  uint32_t firstEpoch;   // 0 if the clock wasn't set
};

static Candidate g_cands[MAX_CANDIDATES];
static uint16_t  g_candCount = 0;
static uint64_t  g_regs[MAX_REGULARS];   // sorted, for binary search
static uint16_t  g_regCount = 0;
static bool      g_mounted = false;
static bool      g_regsDirty = false;

static uint16_t readArray(const char* path, void* dst, size_t itemSize, uint16_t maxItems) {
  File f = LittleFS.open(path, "r");
  if (!f) return 0;
  size_t n = f.read((uint8_t*)dst, itemSize * maxItems) / itemSize;
  f.close();
  return (uint16_t)n;
}

static void writeArray(const char* path, const void* src, size_t bytes) {
  File f = LittleFS.open(path, "w");
  if (!f) return;
  f.write((const uint8_t*)src, bytes);
  f.close();
}

bool regularsBegin() {
  if (!g_mounted) {
    g_mounted = LittleFS.begin(true);  // formats the empty partition on first use
    if (!g_mounted) return false;
    g_candCount = readArray(CANDIDATES_FILE, g_cands, sizeof(Candidate), MAX_CANDIDATES);
    g_regCount = readArray(REGULARS_FILE, g_regs, sizeof(uint64_t), MAX_REGULARS);
  }
  return true;
}

bool isRegular(uint64_t hash) {
  int lo = 0, hi = (int)g_regCount - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    if (g_regs[mid] == hash) return true;
    if (g_regs[mid] < hash) lo = mid + 1;
    else hi = mid - 1;
  }
  return false;
}

static bool addRegular(uint64_t hash) {
  if (g_regCount >= MAX_REGULARS || isRegular(hash)) return false;
  int i = g_regCount;
  while (i > 0 && g_regs[i - 1] > hash) {
    g_regs[i] = g_regs[i - 1];
    i--;
  }
  g_regs[i] = hash;
  g_regCount++;
  g_regsDirty = true;
  return true;
}

uint16_t regularsTrack(const ForageResult& r, uint32_t cycle, uint32_t epoch) {
  // 1. Extend (or restart) the streak of everything heard this time.
  for (uint16_t i = 0; i < r.count; i++) {
    uint64_t h = r.items[i].hash;
    if (isRegular(h)) continue;
    Candidate* c = nullptr;
    for (uint16_t j = 0; j < g_candCount; j++) {
      if (g_cands[j].hash == h) { c = &g_cands[j]; break; }
    }
    if (c && cycle - c->lastCycle <= REGULAR_MAX_MISSED + 1) {
      c->lastCycle = cycle;
      if (!c->firstEpoch) c->firstEpoch = epoch;
      continue;
    }
    if (!c) {
      if (g_candCount >= MAX_CANDIDATES) continue;
      c = &g_cands[g_candCount++];
      c->hash = h;
    }
    c->firstCycle = cycle;
    c->lastCycle = cycle;
    c->firstEpoch = epoch;
  }

  // 2. Promote long streaks; drop streaks broken by too many misses in a row.
  // A clock-based check as well, so extra wheel-press forages can't shorten it.
  const uint32_t needCycles = REGULAR_AFTER_SEC / FORAGE_INTERVAL_SEC;
  uint16_t promoted = 0, kept = 0;
  for (uint16_t j = 0; j < g_candCount; j++) {
    Candidate c = g_cands[j];
    if (cycle - c.lastCycle > REGULAR_MAX_MISSED) continue;  // also drops a previous egg's
    bool longEnough = cycle - c.firstCycle >= needCycles &&
                      (!epoch || !c.firstEpoch || epoch - c.firstEpoch >= REGULAR_AFTER_SEC);
    if (longEnough && c.lastCycle == cycle && addRegular(c.hash)) {
      promoted++;
      continue;
    }
    g_cands[kept++] = c;
  }
  g_candCount = kept;
  return promoted;
}

void regularsSave() {
  if (!g_mounted) return;
  writeArray(CANDIDATES_FILE, g_cands, g_candCount * sizeof(Candidate));
  if (g_regsDirty) {
    writeArray(REGULARS_FILE, g_regs, g_regCount * sizeof(uint64_t));
    g_regsDirty = false;
  }
}

void regularsClear() {
  if (!regularsBegin()) return;
  g_candCount = 0;
  g_regCount = 0;
  LittleFS.remove(CANDIDATES_FILE);
  LittleFS.remove(REGULARS_FILE);
}

uint16_t regularsCount() { return g_regCount; }
uint16_t regularsCandidates() { return g_candCount; }
