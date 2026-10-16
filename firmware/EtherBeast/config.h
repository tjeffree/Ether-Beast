#pragma once

// ---- Schedule -------------------------------------------------------------
#define FORAGE_INTERVAL_SEC     600     // RTC wakes the beast every 10 minutes
#define BLE_SCAN_SEC            8       // passive BLE listen
#define WIFI_CHANNEL_FIRST      1
#define WIFI_CHANNEL_LAST       13      // UK/EU channels 1-13
#define WIFI_CHANNEL_DWELL_MS   1000    // 13 channels x 1s = ~13s of Wi-Fi listening
#define BUTTON_WAKE_AWAKE_MS    8000    // how long the status screen waits for input
#define EVOLVE_WAIT_MS          20000   // ...or this long when ready to evolve

// ---- XP -------------------------------------------------------------------
#define XP_WIFI_CLIENT          1       // phone/laptop probe request
#define XP_WIFI_AP              2       // access point beacon
#define XP_BLE_WEARABLE         3       // watch / fitness tracker / heart-rate strap
#define XP_BLE_OTHER            1       // any other BLE advertiser

#define COMBO_SNACK_MIN         5       // x1.5
#define COMBO_FEAST_MIN         15      // x2.0
#define COMBO_FRENZY_MIN        30      // x3.0 + Sugar Rush
#define FEEDING_MIN_NEW         6       // "Feeding" face when new devices > 5

// Total XP needed to reach level L is LEVEL_XP_STEP * L * (L-1):
// L2=100, L3=300, L4=600, L5=1000 ... L10=4500
#define LEVEL_XP_STEP           50

#define STARVING_AFTER_SEC      (24UL * 3600UL)
#define NULL_BYTE_AFTER_SEC     (72UL * 3600UL)  // devolves into a Null Byte until fed again
#define SUGAR_RUSH_SEC          3600UL

// ---- Evolution -----------------------------------------------------------
// Forms change at these levels; the branch taken depends on what the beast ate
// while in its previous form (see chooseForm in game_state.cpp).
#define TIER2_LEVEL             5       // diet:        Beacon Imp / Blue Gremlin / Pulse Leech
#define TIER3_LEVEL             10      // lifestyle:   Packet Hound / Router Golem / Static Moth
#define TIER4_LEVEL             17      // temperament: Mesh Hydra / Firewall Drake / Hollow Wraith
#define TIER5_LEVEL             25      // final:       Ether Wyrm / Promiscuous One
#define DIET_WEARABLE_PCT       8       // share of wearables that makes a Pulse Leech
#define NIGHT_FEEDER_PCT        25      // share eaten 00:00-05:59 that makes a Static Moth
#define NIGHT_END_HOUR          6
#define ROAMER_PCT              50      // share eaten away from home that makes a Packet Hound
#define HYDRA_FRENZIES          10      // frenzies in the previous form that make a Mesh Hydra
#define HYDRA_HEAD_FRENZIES     10      // ...and each further 10 as a Hydra grows a head (max 3)
#define FARADAY_SILENT_FORAGES  50      // forages in a row hearing nothing at all
#define LOCAL_TZ                "GMT0BST,M3.5.0/1,M10.5.0"  // for "night"; the RTC keeps UTC
#define MAX_SCARS               10      // filter clears shown as marks beside the beast

// ---- Seen-device memory (salted hashes in a Bloom filter, NVS) -------------
#define SEEN_FILTER_BYTES       4096    // 32768 bits
#define SEEN_FILTER_HASHES      4
#define SEEN_FILTER_CAPACITY    3500    // ~1% false positives; beyond this it is "digested" (cleared)
#define MAX_SIGHTINGS           512     // unique devices held in RAM per forage

// ---- Display ----------------------------------------------------------------
#define FULL_REFRESH_EVERY      6       // full (flashing) refresh every Nth update, partial otherwise
#define SPRITE_SCALE            4       // 32x32 art -> 128x128 on screen
#define FACE_X                  10      // face position inside the 32x32 body art
#define FACE_Y                  12

// ---- Home sync ----------------------------------------------------------------
#define SYNC_MIN_INTERVAL_SEC   3600UL  // at home, push to GitHub at most hourly (plus on level-up)
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define NTP_SERVER              "pool.ntp.org"

#define GITHUB_OWNER            "tjeffree"
#define GITHUB_REPO             "Ether-Beast"
#define GITHUB_PATH             "data.json"
#define GITHUB_BRANCH           "main"
