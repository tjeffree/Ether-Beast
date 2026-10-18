#pragma once
#include <Arduino.h>
#include "game_state.h"

enum StatusLine : uint8_t {
  STATUS_NONE, STATUS_READY, STATUS_SYNCED, STATUS_SYNC_FAILED,
  STATUS_STARVING, STATUS_SUGAR, STATUS_EVOLVED, STATUS_HATCHED, STATUS_FORAGING,
};

// Everything the screen shows. Saved after each update so the next boot can
// redraw the previous frame exactly (see uiShow) - keep it free of padding noise
// by always building it with uiModelFrom().
struct RenderModel {
  uint32_t totalXp;
  uint32_t levelXp;
  uint32_t nextXp;
  uint32_t devicesEaten;
  uint16_t level;
  uint16_t lastXpGain;
  uint8_t  form;       // shown in the header
  uint8_t  sprite;     // body art (Hydra head count, Null Byte...)
  uint8_t  teaser;     // next form's sprite while it is ready to evolve into it, or NO_SPRITE
  uint8_t  scars;      // seen-filter clears survived
  uint8_t  face;
  uint8_t  comboX10;
  uint8_t  battery;
  uint8_t  status;
  uint8_t  ready;
  uint8_t  zzz;
  uint8_t  valid;
};

RenderModel uiModelFrom(const GameState& s, uint8_t face, uint8_t status, uint8_t battery);

void uiBegin();
bool uiLoadLast(RenderModel& m);
// Draws `m` with `ticks` countdown notches filled. On the M09 panel, partial
// refreshes first replay `prev` so the panel's differential update knows what
// is really on the glass after a power-off.
void uiShow(const RenderModel& m, const RenderModel* prev, bool fullRefresh, uint8_t ticks);
// Fills countdown notches over `m`, which must already be on the glass.
void uiTick(const RenderModel& m, uint8_t ticks);
// Multi-stage full-refresh evolution animation, ending on `after`.
void uiEvolve(const RenderModel& before, const RenderModel& after, uint8_t ticks);
void uiSaveLast(const RenderModel& m);
// When the current forage cycle began (RTC seconds), so ticks can save nothing.
bool uiLoadCycleStart(uint32_t& t);
void uiSaveCycleStart(uint32_t t);
