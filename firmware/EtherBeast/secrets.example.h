#pragma once

// build.ps1 generates secrets.h from .env (WIFI_HOME_SSID, WIFI_HOME_PASSWORD,
// GITHUB_PAT). Without .env, copy this file to secrets.h and fill it in.
// Both .env and secrets.h are git-ignored.

// Home Wi-Fi: when this SSID is heard during a forage, the beast syncs to GitHub.
#define HOME_SSID       "your-home-wifi"
#define HOME_PASSWORD   "your-wifi-password"

// GitHub fine-grained personal access token.
// Repository access: only tjeffree/Ether-Beast. Permissions: Contents = Read and write.
#define GITHUB_TOKEN    "github_pat_xxxxxxxxxxxxxxxxxxxx"
