// Ether-Beast: a passive wardriving Tamagotchi for the M5Stack CoreInk.
//
// Each boot is one short life cycle:
//   RTC alarm   -> forage (passive BLE + Wi-Fi listen), feed, maybe sync, redraw
//   button/USB  -> status screen; top button evolves, wheel press forages now
// then power off until the RTC alarm brings it back (FORAGE_INTERVAL_SEC).

#include <M5Unified.h>
#include <WiFi.h>
#include <esp_system.h>

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Copy secrets.example.h to secrets.h and fill in your Wi-Fi and GitHub token"
#endif

#include "config.h"
#include "game_state.h"
#include "seen_filter.h"
#include "forage.h"
#include "github_sync.h"
#include "ui.h"

static GameState    state;
static SeenFilter   seen;
static ForageResult foraged;
static RenderModel  onScreen;      // what the panel currently shows
static bool         haveOnScreen = false;

static uint8_t batteryLevel() {
  int32_t b = M5.Power.getBatteryLevel();
  return b < 0 ? 0 : (b > 100 ? 100 : b);
}

static void show(const RenderModel& m, bool force = false) {
  if (!force && haveOnScreen && memcmp(&m, &onScreen, sizeof(m)) == 0) return;
  bool full = !haveOnScreen || state.displayCount % FULL_REFRESH_EVERY == 0;
  uiShow(m, haveOnScreen ? &onScreen : nullptr, full);
  state.displayCount++;
  onScreen = m;
  haveOnScreen = true;
  uiSaveLast(m);
}

static uint8_t idleStatus() {
  if (state.readyToEvolve) return STATUS_READY;
  if (isSugarRush(state)) return STATUS_SUGAR;
  if (isStarving(state)) return STATUS_STARVING;
  return STATUS_NONE;
}

static float comboMultiplier(uint16_t newDevices) {
  if (newDevices >= COMBO_FRENZY_MIN) return 3.0f;
  if (newDevices >= COMBO_FEAST_MIN) return 2.0f;
  if (newDevices >= COMBO_SNACK_MIN) return 1.5f;
  return 1.0f;
}

static void forageCycle() {
  memset(&foraged, 0, sizeof(foraged));
  uint32_t t0 = millis();
  forageBle(foraged);
  forageWifi(foraged, HOME_SSID);
  Serial.printf("Foraged %u unique devices in %lus, home %s\n", foraged.count,
                (unsigned long)((millis() - t0) / 1000), foraged.homeSeen ? "yes" : "no");

  seen.load();
  if (seen.digestIfFull()) state.digestions++;

  // Pass 1: base XP for devices never eaten before.
  uint32_t baseXp = 0;
  uint16_t newDevices = 0;
  for (uint16_t i = 0; i < foraged.count; i++) {
    if (seen.eat(foraged.items[i].hash)) {
      newDevices++;
      baseXp += xpForKind(foraged.items[i].kind);
    }
  }
  if (newDevices) seen.save();

  // Pass 2: swarm multiplier on the whole cycle's XP.
  float mult = comboMultiplier(newDevices);
  uint32_t gain = (uint32_t)(baseXp * mult);

  state.cycles++;
  state.cyclesSinceSync++;
  state.totalXp += gain;
  state.devicesEaten += newDevices;
  state.lastNewDevices = newDevices;
  state.lastXpGain = gain;
  state.lastComboX10 = (uint8_t)(mult * 10.0f + 0.5f);
  if (newDevices) {
    state.cyclesSinceFed = 0;
    state.lastFedEpoch = nowEpoch();
  } else {
    state.cyclesSinceFed++;
  }
  if (mult >= 3.0f) {
    state.sugarCyclesLeft = SUGAR_RUSH_SEC / FORAGE_INTERVAL_SEC;
  } else if (state.sugarCyclesLeft) {
    state.sugarCyclesLeft--;
  }
  checkEvolveReady(state);
  Serial.printf("New %u, base XP %lu x%.1f = +%lu, total %lu, level %u%s\n", newDevices,
                (unsigned long)baseXp, mult, (unsigned long)gain, (unsigned long)state.totalXp,
                state.level, state.readyToEvolve ? " (ready to evolve)" : "");

  // Home: upload at most hourly, or straight away after a level-up.
  bool syncDue = state.forceSync ||
                 (uint64_t)state.cyclesSinceSync * FORAGE_INTERVAL_SEC >= SYNC_MIN_INTERVAL_SEC;
  if (foraged.homeSeen && syncDue) {
    String error;
    if (syncToGitHub(state, faceFor(state, true), batteryLevel(), error)) {
      state.cyclesSinceSync = 0;
      state.forceSync = 0;
      state.lastSyncOk = 1;
      Serial.println("Synced to GitHub");
    } else {
      state.lastSyncOk = 2;
      Serial.printf("Sync failed: %s\n", error.c_str());
    }
  }
  WiFi.mode(WIFI_OFF);

  uint8_t status = idleStatus();
  bool sleeping = false;
  if (foraged.homeSeen && status == STATUS_NONE) {
    if (state.lastSyncOk == 1) { status = STATUS_SYNCED; sleeping = true; }
    else if (state.lastSyncOk == 2) status = STATUS_SYNC_FAILED;
  }
  show(uiModelFrom(state, faceFor(state, sleeping), status, batteryLevel()));
}

static void evolve() {
  RenderModel before = onScreen;
  state.level++;
  state.forceSync = 1;
  checkEvolveReady(state);
  RenderModel after = uiModelFrom(state, FACE_HAPPY, STATUS_EVOLVED, batteryLevel());
  Serial.printf("Evolved to level %u (%s)\n", state.level, stageName(after.stage));
  uiEvolve(before, after);
  state.displayCount++;
  onScreen = after;
  uiSaveLast(after);
}

static void buttonWake() {
  M5.Power.setLed(255);
  show(uiModelFrom(state, faceFor(state, false), idleStatus(), batteryLevel()));
  M5.Power.setLed(0);

  uint32_t awakeMs = state.readyToEvolve ? EVOLVE_WAIT_MS : BUTTON_WAKE_AWAKE_MS;
  uint32_t start = millis();
  while (millis() - start < awakeMs) {
    M5.update();
    if (state.readyToEvolve && M5.BtnEXT.wasPressed()) {
      evolve();
      return;
    }
    if (M5.BtnB.wasPressed()) {  // wheel press: go foraging right now
      forageCycle();
      return;
    }
    delay(10);
  }
}

static void sleepUntilNextForage() {
  stateSave(state);
  Serial.printf("Sleeping %us\n", FORAGE_INTERVAL_SEC);
  Serial.flush();
  // On battery this powers the CoreInk off; the RTC alarm powers it back on.
  // On USB it light-sleeps and restarts instead.
  M5.Power.timerSleep(FORAGE_INTERVAL_SEC);
}

void setup() {
  auto cfg = M5.config();
  cfg.clear_display = false;    // the e-ink keeps the last frame; don't flash it white
  cfg.disable_rtc_irq = false;  // we need the alarm flag to know why we woke
  M5.begin(cfg);

  // RTC alarm => scheduled forage. On USB, timerSleep() ends in esp_restart().
  bool timerWake = M5.Rtc.getIRQstatus() || esp_reset_reason() == ESP_RST_SW;
  M5.Rtc.disableIRQ();
  M5.Rtc.clearIRQ();

  uiBegin();
  seenSaltInit();
  haveOnScreen = uiLoadLast(onScreen);
  bool existing = stateLoad(state);
  Serial.printf("\nEther-Beast: %s wake, level %u, %lu XP\n", timerWake ? "timer" : "button",
                state.level, (unsigned long)state.totalXp);

  if (!existing) {
    show(uiModelFrom(state, FACE_NORMAL, STATUS_HATCHED, batteryLevel()), true);
  } else if (timerWake) {
    forageCycle();
  } else {
    buttonWake();
  }
  sleepUntilNextForage();
}

void loop() {
}
