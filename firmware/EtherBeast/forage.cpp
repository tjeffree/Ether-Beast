#include "forage.h"
#include "seen_filter.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

static ForageResult* g_result = nullptr;
static portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;
static const char* g_homeSsid = nullptr;
static size_t g_homeSsidLen = 0;
static volatile bool g_homeSeen = false;

uint8_t xpForKind(uint8_t kind) {
  switch (kind) {
    case KIND_WIFI_CLIENT:  return XP_WIFI_CLIENT;
    case KIND_WIFI_AP:      return XP_WIFI_AP;
    case KIND_BLE_WEARABLE: return XP_BLE_WEARABLE;
    default:                return XP_BLE_OTHER;
  }
}

// Records a device for this forage. Called from the Wi-Fi task and BLE callback.
static void addSighting(const uint8_t addr[6], uint8_t kind) {
  uint64_t h = hashAddress(addr);
  portENTER_CRITICAL(&g_mux);
  ForageResult& r = *g_result;
  bool found = false;
  for (uint16_t i = 0; i < r.count; i++) {
    if (r.items[i].hash == h) {
      // Seen twice this cycle: keep whichever kind is worth more.
      if (xpForKind(kind) > xpForKind(r.items[i].kind)) r.items[i].kind = kind;
      found = true;
      break;
    }
  }
  if (!found && r.count < MAX_SIGHTINGS) {
    r.items[r.count].hash = h;
    r.items[r.count].kind = kind;
    r.count++;
  }
  portEXIT_CRITICAL(&g_mux);
}

// ---------------------------------------------------------------- BLE ------

static bool isWearable(BLEAdvertisedDevice& d) {
  if (d.haveAppearance()) {
    uint16_t category = d.getAppearance() >> 6;
    // Watch, heart rate, blood pressure, running/walking, cycling,
    // pulse oximeter, outdoor sports activity
    if (category == 3 || category == 13 || category == 14 || category == 17 ||
        category == 18 || category == 49 || category == 81) {
      return true;
    }
  }
  static const uint16_t wearableServices[] = {
    0x180D,  // Heart Rate
    0x1814,  // Running Speed and Cadence
    0x1816,  // Cycling Speed and Cadence
    0xFEE0,  // common fitness-band service
  };
  int n = d.getServiceUUIDCount();
  for (int i = 0; i < n; i++) {
    BLEUUID uuid = d.getServiceUUID(i);
    for (uint16_t svc : wearableServices) {
      if (uuid.equals(BLEUUID(svc))) return true;
    }
  }
  return false;
}

class AdvertCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice d) override {
    BLEAddress addr = d.getAddress();
    addSighting(reinterpret_cast<const uint8_t*>(addr.getNative()),
                isWearable(d) ? KIND_BLE_WEARABLE : KIND_BLE_OTHER);
  }
};

void forageBle(ForageResult& r) {
  g_result = &r;
  static AdvertCallbacks callbacks;
  BLEDevice::init("");
  BLEScan* scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(&callbacks, false);
  scan->setActiveScan(false);  // passive: listen to adverts, never send scan requests
  scan->setInterval(100);
  scan->setWindow(99);
  scan->start(BLE_SCAN_SEC, false);
  scan->clearResults();
  // Hand the Bluetooth memory back: the TLS upload later needs it.
  BLEDevice::deinit(true);
}

// --------------------------------------------------------------- Wi-Fi -----

// 802.11 management frame: frame control (2), duration (2), addr1 (6),
// addr2 = transmitter (6), addr3 (6), sequence (2) = 24-byte header.
static const int MGMT_HEADER_LEN = 24;
static const uint8_t SUBTYPE_PROBE_REQ = 0x4;
static const uint8_t SUBTYPE_BEACON = 0x8;

static void onWifiFrame(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;
  const wifi_promiscuous_pkt_t* pkt = (const wifi_promiscuous_pkt_t*)buf;
  const uint8_t* f = pkt->payload;
  int len = pkt->rx_ctrl.sig_len;
  if (len < MGMT_HEADER_LEN) return;

  uint8_t subtype = f[0] >> 4;
  const uint8_t* transmitter = f + 10;

  if (subtype == SUBTYPE_PROBE_REQ) {
    addSighting(transmitter, KIND_WIFI_CLIENT);
  } else if (subtype == SUBTYPE_BEACON) {
    addSighting(transmitter, KIND_WIFI_AP);
    // Beacon body: timestamp (8), interval (2), capabilities (2), then tagged
    // elements; element 0 is the SSID. Only compared against HOME_SSID.
    const int ssidTag = MGMT_HEADER_LEN + 12;
    if (!g_homeSeen && g_homeSsidLen > 0 && len >= ssidTag + 2 && f[ssidTag] == 0) {
      uint8_t ssidLen = f[ssidTag + 1];
      if (ssidLen == g_homeSsidLen && len >= ssidTag + 2 + ssidLen &&
          memcmp(f + ssidTag + 2, g_homeSsid, ssidLen) == 0) {
        g_homeSeen = true;
      }
    }
  }
}

void forageWifi(ForageResult& r, const char* homeSsid) {
  g_result = &r;
  g_homeSsid = homeSsid;
  g_homeSsidLen = homeSsid ? strlen(homeSsid) : 0;
  g_homeSeen = false;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  wifi_country_t country = { "GB", WIFI_CHANNEL_FIRST,
                             WIFI_CHANNEL_LAST - WIFI_CHANNEL_FIRST + 1, 20,
                             WIFI_COUNTRY_POLICY_MANUAL };
  esp_wifi_set_country(&country);

  wifi_promiscuous_filter_t filter = { .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT };
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb(&onWifiFrame);
  esp_wifi_set_promiscuous(true);

  for (uint8_t ch = WIFI_CHANNEL_FIRST; ch <= WIFI_CHANNEL_LAST; ch++) {
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    delay(WIFI_CHANNEL_DWELL_MS);
  }

  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(nullptr);
  r.homeSeen = g_homeSeen;
}
