#include "ui.h"
#include "config.h"
#include "sprites.h"
#include <M5Unified.h>
#include <Preferences.h>

static M5Canvas canvas(&M5.Display);
static const int W = 200;
static const int H = 200;
static const int MONSTER_SIZE = 32 * SPRITE_SCALE;
static const int MONSTER_X = (W - MONSTER_SIZE) / 2;
static const int MONSTER_Y = 21;

RenderModel uiModelFrom(const GameState& s, uint8_t face, uint8_t status, uint8_t battery) {
  RenderModel m;
  memset(&m, 0, sizeof(m));
  m.totalXp = s.totalXp;
  m.levelXp = xpForLevel(s.level);
  m.nextXp = xpForLevel(s.level + 1);
  m.devicesEaten = s.devicesEaten;
  m.level = s.level;
  m.lastXpGain = s.lastXpGain;
  m.form = displayForm(s);
  m.sprite = spriteFor(s);
  m.teaser = teaserSprite(s);
  m.scars = s.digestions < MAX_SCARS ? s.digestions : MAX_SCARS;
  m.face = face;
  m.comboX10 = s.lastComboX10;
  m.battery = (battery / 5) * 5;  // coarse, so it doesn't force a redraw every wake
  m.status = status;
  m.ready = s.readyToEvolve;
  m.zzz = status == STATUS_SYNCED;
  m.valid = 1;
  return m;
}

void uiBegin() {
  M5.Display.setAutoDisplay(false);
  canvas.setColorDepth(1);
  canvas.createSprite(W, H);
}

static void drawSprite(const Sprite& s, int x, int y, int scale, uint32_t color) {
  const int bytesPerRow = (s.w + 7) / 8;
  for (int py = 0; py < s.h; py++) {
    for (int px = 0; px < s.w; px++) {
      uint8_t b = pgm_read_byte(s.bits + py * bytesPerRow + px / 8);
      if (b & (0x80 >> (px & 7))) {
        canvas.fillRect(x + px * scale, y + py * scale, scale, scale, color);
      }
    }
  }
}

static void drawMonster(uint8_t sprite, uint8_t face, uint32_t color) {
  if (sprite >= BODY_SPRITES_COUNT) sprite = 0;
  drawSprite(BODY_SPRITES[sprite], MONSTER_X, MONSTER_Y, SPRITE_SCALE, color);
  if (face < FACE_SPRITES_COUNT) {
    drawSprite(FACE_SPRITES[face], MONSTER_X + FACE_X * SPRITE_SCALE,
               MONSTER_Y + FACE_Y * SPRITE_SCALE, SPRITE_SCALE, color);
  }
}

static const char* statusText(uint8_t status) {
  switch (status) {
    case STATUS_READY:       return "PRESS TOP BTN";
    case STATUS_SYNCED:      return "Zzz (Uploaded)";
    case STATUS_SYNC_FAILED: return "sync failed";
    case STATUS_STARVING:    return "STARVING";
    case STATUS_SUGAR:       return "SUGAR RUSH!";
    case STATUS_EVOLVED:     return "EVOLVED!";
    case STATUS_HATCHED:     return "hatched!";
    default:                 return "";
  }
}

static void render(const RenderModel& m) {
  canvas.fillScreen(TFT_WHITE);
  canvas.setTextColor(TFT_BLACK);
  canvas.setFont(&fonts::Font2);

  // Header: level + form name, battery.
  char buf[40];
  snprintf(buf, sizeof(buf), "LV%u %s", m.level, formName(m.form));
  canvas.setTextDatum(top_left);
  canvas.drawString(buf, 4, 1);
  snprintf(buf, sizeof(buf), "%u%%", m.battery);
  canvas.setTextDatum(top_right);
  canvas.drawString(buf, W - 4, 1);
  canvas.drawFastHLine(0, 18, W, TFT_BLACK);

  drawMonster(m.sprite, m.face, TFT_BLACK);

  // Left margin: what it is about to become (top) and its scars (bottom).
  if (m.teaser < BODY_SPRITES_COUNT) {
    drawSprite(BODY_SPRITES[m.teaser], 2, MONSTER_Y + 1, 1, TFT_BLACK);
    canvas.setTextDatum(top_center);
    canvas.drawString("???", 18, MONSTER_Y + 34);
  }
  for (uint8_t i = 0; i < m.scars; i++) {
    int x = 8 + (i % 2) * 12;
    int y = MONSTER_Y + 86 + (i / 2) * 8;
    canvas.drawLine(x, y, x + 5, y + 5, TFT_BLACK);
    canvas.drawLine(x + 5, y, x, y + 5, TFT_BLACK);
  }

  if (m.ready) {
    // Evolution prompt: exclamation marks either side of the monster.
    canvas.setFont(&fonts::FreeSansBold18pt7b);
    canvas.setTextDatum(middle_center);
    canvas.drawString("!", 16, MONSTER_Y + MONSTER_SIZE / 2);
    canvas.drawString("!", W - 16, MONSTER_Y + MONSTER_SIZE / 2);
    canvas.setFont(&fonts::Font2);
  }
  if (m.zzz) {
    canvas.setTextDatum(top_right);
    canvas.drawString("z", W - 22, 44);
    canvas.drawString("Z", W - 12, 30);
  }

  // XP bar from this level's threshold to the next.
  const int barY = MONSTER_Y + MONSTER_SIZE + 3;
  canvas.drawRect(4, barY, W - 8, 11, TFT_BLACK);
  uint32_t span = m.nextXp > m.levelXp ? m.nextXp - m.levelXp : 1;
  uint32_t into = m.totalXp > m.levelXp ? m.totalXp - m.levelXp : 0;
  if (into > span) into = span;
  int fill = (int)((uint64_t)(W - 12) * into / span);
  if (fill > 0) canvas.fillRect(6, barY + 2, fill, 7, TFT_BLACK);

  // XP numbers, and the combo right next to the bar when there was one.
  const int lineY = barY + 13;
  snprintf(buf, sizeof(buf), "%lu/%lu XP", (unsigned long)m.totalXp, (unsigned long)m.nextXp);
  canvas.setTextDatum(top_left);
  canvas.drawString(buf, 4, lineY);
  if (m.comboX10 > 10) {
    snprintf(buf, sizeof(buf), "x%u.%u COMBO!", m.comboX10 / 10, m.comboX10 % 10);
  } else if (m.lastXpGain > 0) {
    snprintf(buf, sizeof(buf), "+%u", m.lastXpGain);
  } else {
    buf[0] = 0;
  }
  canvas.setTextDatum(top_right);
  canvas.drawString(buf, W - 4, lineY);

  // Footer: devices eaten, status.
  const int footY = lineY + 16;
  snprintf(buf, sizeof(buf), "Ate %lu", (unsigned long)m.devicesEaten);
  canvas.setTextDatum(top_left);
  canvas.drawString(buf, 4, footY);
  canvas.setTextDatum(top_right);
  canvas.drawString(statusText(m.status), W - 4, footY);
}

static void push(epd_mode_t mode) {
  M5.Display.setEpdMode(mode);
  M5.Display.startWrite();
  canvas.pushSprite(&M5.Display, 0, 0);
  M5.Display.endWrite();
  M5.Display.display();
  M5.Display.waitDisplay();
}

void uiShow(const RenderModel& m, const RenderModel* prev, bool fullRefresh) {
  if (!fullRefresh && prev && prev->valid) {
    // After power-off the driver assumes a blank panel. Replaying the old
    // frame (no visible change) makes the next partial update diff correctly.
    render(*prev);
    push(epd_mode_t::epd_fastest);
  }
  render(m);
  push(fullRefresh ? epd_mode_t::epd_quality : epd_mode_t::epd_fast);
}

void uiEvolve(const RenderModel& before, const RenderModel& after) {
  // Each step is a full refresh, which also scrubs any ghosting.
  render(before);
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextDatum(middle_center);
  canvas.drawString("?!", W - 22, 40);
  push(epd_mode_t::epd_quality);

  canvas.fillScreen(TFT_BLACK);
  push(epd_mode_t::epd_quality);

  canvas.fillScreen(TFT_BLACK);
  drawMonster(after.sprite, 0xFF, TFT_WHITE);  // silhouette of the new form
  push(epd_mode_t::epd_quality);

  canvas.fillScreen(TFT_WHITE);
  push(epd_mode_t::epd_quality);

  render(after);
  push(epd_mode_t::epd_quality);
}

bool uiLoadLast(RenderModel& m) {
  Preferences p;
  p.begin("ui", true);
  size_t n = p.getBytes("last", &m, sizeof(m));
  p.end();
  return n == sizeof(m) && m.valid;
}

void uiSaveLast(const RenderModel& m) {
  Preferences p;
  p.begin("ui", false);
  p.putBytes("last", &m, sizeof(m));
  p.end();
}
