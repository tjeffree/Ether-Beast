# Ether-Beast

A passive wardriving Tamagotchi for the **M5Stack CoreInk**. Every 10 minutes it wakes,
quietly listens for Wi-Fi and Bluetooth LE devices, and "eats" the ones it has never seen
before. New devices become XP; enough XP and the monster is ready to evolve. When it hears
your home Wi-Fi it uploads its state here, and the GitHub Pages dashboard shows it.

**Privacy:** the device only listens; it never transmits during a forage. Device addresses
are reduced straight away to a salted hash and stored only as bits in a Bloom filter
(plus, for *regulars*, the salted hash itself). The
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

Only devices with a stable address count. Phones, watches and earbuds mostly advertise
from random addresses that change every few minutes (and probe for Wi-Fi from randomised
MACs), so those are skipped rather than eaten again and again.

**Regulars:** a device heard in every forage for 3 hours (missing at most one forage in
a row) becomes a *regular* and is never eaten again, even after the seen-device memory
is cleared. Your home's and office's devices, and the ones you carry with you, end up
here. Regulars are kept as salted hashes in LittleFS on the spare `spiffs` partition.

Reaching level L takes `50 * L * (L-1)` total XP.

### Evolution

The beast changes form at levels 5, 10, 17 and 25. Which branch it takes depends on
what it ate while in its previous form, so you can steer it:

| Levels | Branches on | Forms |
|--------|-------------|-------|
| 1      |             | Egg |
| 2-4    |             | Static Blob |
| 5-9    | diet        | **Pulse Leech** (8%+ wearables), **Blue Gremlin** (more Bluetooth than Wi-Fi), else **Beacon Imp** |
| 10-16  | lifestyle   | **Static Moth** (25%+ eaten 00:00-05:59), **Packet Hound** (50%+ eaten away from home), else **Router Golem** |
| 17-24  | temperament | **Hollow Wraith** (starved and recovered), **Mesh Hydra** (10+ frenzies; one more head per 10 as a Hydra), else **Firewall Drake** |
| 25+    | final       | **Ether Wyrm**, or the secret **Promiscuous One** |

Secrets and conditions:

- **Promiscuous One**: eat all four kinds of device in a single x3.0 frenzy, any time
  before level 25.
- **Null Byte**: 3 days without food and it devolves into a cracked, glitched shell.
  Feeding it brings back its form.
- **Faraday Ghost**: 50 forages in a row hearing nothing at all (a drawer, a dead zone).
- **Scars**: each time the seen-device memory fills up and is cleared, the beast gets a
  mark beside it on screen.
- When it is ready to evolve into a new form, that form is previewed top-left with "???".

Thresholds are in `firmware/EtherBeast/config.h`. Changing the saved state's format
(the `STATE_MAGIC` in `game_state.cpp`) hatches a fresh egg and clears its memory of
devices.

Moods: *starving* after 24h with no new devices, *feeding* after a forage with more than
5 new devices, *sleeping* at home after an upload.

## Buttons

The CoreInk is fully **powered off** between forages; the RTC alarm turns it back on.
It also wakes for a second or two each minute, radios off, to thicken the next of the
ten dashes under the header: a countdown to the next forage, which shows "FORAGING..."
while it listens.

- **Side power button**: wake it up. It shows the status screen for a few seconds.
- **Top button** (while awake, when "PRESS TOP BTN" shows): evolve.
- **Wheel press** (while awake): forage right now instead of waiting.

On USB power it can't fully power off, so it light-sleeps instead. The side button
still wakes it.

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
