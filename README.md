# Ether-Beast

A passive wardriving Tamagotchi for the **M5Stack CoreInk**. Every 10 minutes it wakes,
quietly listens for Wi-Fi and Bluetooth LE devices, and "eats" the ones it has never seen
before. New devices become XP; enough XP and the monster is ready to evolve. When it hears
your home Wi-Fi it uploads its state here, and the GitHub Pages dashboard shows it.

**Privacy:** the device only listens; it never transmits during a forage. Device addresses
are reduced straight away to a salted hash and stored only as bits in a Bloom filter. The
salt is random, never leaves the device, and no addresses are uploaded. Only the monster's
stats go to GitHub.

## How it plays

| Device heard for the first time       | XP |
|---------------------------------------|----|
| Phone/laptop probing for Wi-Fi        | 1  |
| Wi-Fi access point (beacon)           | 2  |
| BLE wearable (watch, fitness tracker) | 3  |
| Any other BLE advertiser              | 1  |

Combos from new devices in a single forage: **5+** x1.5, **15+** x2.0, **30+** x3.0 plus
an hour of *Sugar Rush*.

Forms: Egg (LV1), Blob (LV2-3), Imp (LV4-6), Brute (LV7-9), Ether Wyrm (LV10+).
Reaching level L takes `50 * L * (L-1)` total XP.

Moods: *starving* after 24h with no new devices, *feeding* after a forage with more than
5 new devices, *sleeping* at home after an upload.

## Buttons

The CoreInk is fully **powered off** between forages; the RTC alarm turns it back on.

- **Side power button**: wake it up. It shows the status screen for a few seconds.
- **Top button** (while awake, when "PRESS TOP BTN" shows): evolve.
- **Wheel press** (while awake): forage right now instead of waiting.

On USB power it can't fully power off, so it sleeps and restarts instead. Use the reset
button to wake it.

## Building

Uses the Arduino IDE's bundled `arduino-cli` with the **Espressif ESP32 core 2.0.17**.
The 3.x cores run out of IRAM with Bluetooth and Wi-Fi together on this chip. Libraries:
M5Unified, M5GFX, ArduinoJson 7.

1. Create `.env` in the repo root (git-ignored):
   ```
   WIFI_HOME_SSID=your-home-wifi
   WIFI_HOME_PASSWORD=...
   GITHUB_PAT=github_pat_...
   ```
   The token should be **fine-grained**, limited to this repository, with
   *Contents: Read and write* only.
2. `.\build.ps1` compiles and uploads to COM5. `.\build.ps1 build` only compiles,
   `.\build.ps1 monitor` opens the serial log, and `-Port COM7` uses another port.
   The script regenerates `firmware/EtherBeast/secrets.h` from `.env` and the sprite
   bitmaps from `sprites/*.txt`.

Tunables (scan times, XP, thresholds, refresh cadence) are in
`firmware/EtherBeast/config.h`.

## Layout

```
firmware/EtherBeast/   the firmware (Arduino sketch)
firmware/hello_coreink first hardware test sketch
sprites/*.txt          monster art as ASCII; tools/gen_sprites.ps1 converts it
index.html, app.js     GitHub Pages dashboard
sprites.js             generated art for the dashboard
data.json              written by the device via the GitHub contents API
```

## Dashboard

Enable GitHub Pages under *Settings → Pages → Deploy from branch → main / (root)*.
The device commits `data.json` to `main` at most hourly when home (and straight after
a level-up), so `git pull` before pushing your own changes.
