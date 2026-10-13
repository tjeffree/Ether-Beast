#pragma once
#include <Arduino.h>
#include "game_state.h"

enum StatusLine : uint8_t {
  STATUS_NONE, STATUS_READY, STATUS_SYNCED, STATUS_SYNC_FAILED,
  STATUS_STARVING, STATUS_SUGAR, STATUS_EVOLVED, STATUS_HATCHED,
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
  uint8_t  stage;
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
// Draws `m`. Partial refreshes first replay `prev` so the panel's
// differential update knows what is really on the glass after a power-off.
void uiShow(const RenderModel& m, const RenderModel* prev, bool fullRefresh);
// Multi-stage full-refresh evolution animation, ending on `after`.
void uiEvolve(const RenderModel& before, const RenderModel& after);
void uiSaveLast(const RenderModel& m);
