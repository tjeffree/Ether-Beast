#include "github_sync.h"
#include "config.h"
#include "certs.h"
#include "secrets.h"
#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

String base64Encode(const uint8_t* data, size_t len) {
  static const char table[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String out;
  out.reserve(((len + 2) / 3) * 4);
  for (size_t i = 0; i < len; i += 3) {
    uint32_t n = (uint32_t)data[i] << 16;
    if (i + 1 < len) n |= (uint32_t)data[i + 1] << 8;
    if (i + 2 < len) n |= data[i + 2];
    out += table[(n >> 18) & 63];
    out += table[(n >> 12) & 63];
    out += i + 1 < len ? table[(n >> 6) & 63] : '=';
    out += i + 2 < len ? table[n & 63] : '=';
  }
  return out;
}

static String isoTime(uint32_t epoch) {
  if (!epoch) return String();
  time_t t = epoch;
  struct tm tm;
  gmtime_r(&t, &tm);
  char buf[24];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return String(buf);
}

String buildStatePayload(const GameState& s, uint8_t face, uint8_t battery) {
  JsonDocument doc;
  doc["Level"] = s.level;
  doc["Total_XP"] = s.totalXp;
  // Fed this cycle before NTP had set the clock: "now" is within a minute.
  uint32_t fed = s.lastFedEpoch ? s.lastFedEpoch : (s.lastNewDevices ? nowEpoch() : 0);
  if (fed) doc["Last_Fed_Timestamp"] = isoTime(fed);
  else doc["Last_Fed_Timestamp"] = nullptr;
  doc["Total_Devices_Eaten"] = s.devicesEaten;

  uint8_t stage = stageForLevel(s.level);
  doc["stage"] = stage;
  doc["stage_name"] = stageName(stage);
  doc["level_xp"] = xpForLevel(s.level);
  doc["next_level_xp"] = xpForLevel(s.level + 1);
  doc["ready_to_evolve"] = (bool)s.readyToEvolve;
  doc["mood"] = moodName(face);
  doc["face"] = face;
  doc["sugar_rush"] = isSugarRush(s);
  doc["starving"] = isStarving(s);
  JsonObject last = doc["last_forage"].to<JsonObject>();
  last["new_devices"] = s.lastNewDevices;
  last["xp_gained"] = s.lastXpGain;
  last["combo"] = s.lastComboX10 / 10.0f;
  doc["forage_cycles"] = s.cycles;
  doc["battery"] = battery;
  doc["synced_at"] = isoTime(nowEpoch());

  String out;
  serializeJsonPretty(doc, out);
  return out;
}

static bool joinHomeWifi(String& error) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(HOME_SSID, HOME_PASSWORD);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
      error = "wifi timeout";
      return false;
    }
    delay(100);
  }
  return true;
}

static void syncClock() {
  configTime(0, 0, NTP_SERVER);
  uint32_t start = millis();
  while (!nowEpoch() && millis() - start < 8000) delay(100);
  uint32_t now = nowEpoch();
  if (now) {
    time_t t = now;
    struct tm tm;
    gmtime_r(&t, &tm);
    M5.Rtc.setDateTime(&tm);  // RTC keeps UTC
  }
}

static void addGitHubHeaders(HTTPClient& http) {
  http.addHeader("Authorization", String("Bearer ") + GITHUB_TOKEN);
  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("X-GitHub-Api-Version", "2022-11-28");
  http.setUserAgent("Ether-Beast-CoreInk");
}

bool syncToGitHub(const GameState& s, uint8_t face, uint8_t battery, String& error) {
  if (!joinHomeWifi(error)) return false;
  syncClock();  // before building the payload so its timestamps are real
  const String json = buildStatePayload(s, face, battery);

  WiFiClientSecure client;
  client.setCACert(GITHUB_ROOT_CA);
  const String url = String("https://api.github.com/repos/") + GITHUB_OWNER + "/" +
                     GITHUB_REPO + "/contents/" + GITHUB_PATH;

  // 1. GET the current file to learn its sha (404 = first upload).
  String sha;
  {
    HTTPClient http;
    http.begin(client, url + "?ref=" + GITHUB_BRANCH);
    addGitHubHeaders(http);
    int code = http.GET();
    if (code == 200) {
      JsonDocument filter;
      filter["sha"] = true;
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, http.getStream(),
                                                 DeserializationOption::Filter(filter));
      if (!err) sha = doc["sha"].as<String>();
    } else if (code != 404) {
      error = "GET " + String(code);
      http.end();
      WiFi.disconnect(true);
      return false;
    }
    http.end();
  }

  // 2. PUT the new contents.
  JsonDocument body;
  body["message"] = "Device Sync";
  body["content"] = base64Encode((const uint8_t*)json.c_str(), json.length());
  body["branch"] = GITHUB_BRANCH;
  if (sha.length()) body["sha"] = sha;
  String bodyStr;
  serializeJson(body, bodyStr);

  HTTPClient http;
  http.begin(client, url);
  addGitHubHeaders(http);
  http.addHeader("Content-Type", "application/json");
  int code = http.PUT(bodyStr);
  http.end();
  WiFi.disconnect(true);

  if (code != 200 && code != 201) {
    error = "PUT " + String(code);
    return false;
  }
  return true;
}
