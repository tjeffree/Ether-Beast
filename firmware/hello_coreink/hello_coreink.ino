#include <M5Unified.h>

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);
  Serial.println("Hello from M5Stack Core Ink!");

  M5.Display.setEpdMode(epd_mode_t::epd_quality);
  M5.Display.fillScreen(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString("Hello", M5.Display.width() / 2, M5.Display.height() / 2 - 20);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("Core Ink OK", M5.Display.width() / 2, M5.Display.height() / 2 + 25);
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    Serial.println("Button pressed");
  }
  static uint32_t last = 0;
  if (millis() - last > 5000) {
    last = millis();
    Serial.printf("Uptime: %lu s, battery: %d%%\n", millis() / 1000, M5.Power.getBatteryLevel());
  }
  delay(10);
}
