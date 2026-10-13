#include "seen_filter.h"
#include <Preferences.h>
#include <esp_random.h>

static const char* NVS_NS = "seen";
static uint8_t g_salt[16];

void seenSaltInit() {
  Preferences p;
  p.begin(NVS_NS, false);
  if (p.getBytes("salt", g_salt, sizeof(g_salt)) != sizeof(g_salt)) {
    esp_fill_random(g_salt, sizeof(g_salt));
    p.putBytes("salt", g_salt, sizeof(g_salt));
  }
  p.end();
}

// MurmurHash64A by Austin Appleby (public domain).
static uint64_t murmur64a(const uint8_t* data, size_t len, uint64_t seed) {
  const uint64_t m = 0xc6a4a7935bd1e995ULL;
  const int r = 47;
  uint64_t h = seed ^ (len * m);

  const uint8_t* end = data + (len / 8) * 8;
  while (data != end) {
    uint64_t k;
    memcpy(&k, data, 8);
    data += 8;
    k *= m; k ^= k >> r; k *= m;
    h ^= k; h *= m;
  }
  switch (len & 7) {
    case 7: h ^= (uint64_t)data[6] << 48; [[fallthrough]];
    case 6: h ^= (uint64_t)data[5] << 40; [[fallthrough]];
    case 5: h ^= (uint64_t)data[4] << 32; [[fallthrough]];
    case 4: h ^= (uint64_t)data[3] << 24; [[fallthrough]];
    case 3: h ^= (uint64_t)data[2] << 16; [[fallthrough]];
    case 2: h ^= (uint64_t)data[1] << 8;  [[fallthrough]];
    case 1: h ^= (uint64_t)data[0]; h *= m;
  }
  h ^= h >> r; h *= m; h ^= h >> r;
  return h;
}

uint64_t hashAddress(const uint8_t addr[6]) {
  uint8_t buf[sizeof(g_salt) + 6];
  memcpy(buf, g_salt, sizeof(g_salt));
  memcpy(buf + sizeof(g_salt), addr, 6);
  return murmur64a(buf, sizeof(buf), 0x45746865724265ULL);
}

void SeenFilter::load() {
  Preferences p;
  p.begin(NVS_NS, true);
  if (p.getBytes("bits", _bits, sizeof(_bits)) != sizeof(_bits)) {
    memset(_bits, 0, sizeof(_bits));
    _count = 0;
  } else {
    _count = p.getUInt("count", 0);
  }
  p.end();
}

void SeenFilter::save() {
  Preferences p;
  p.begin(NVS_NS, false);
  p.putBytes("bits", _bits, sizeof(_bits));
  p.putUInt("count", _count);
  p.end();
}

bool SeenFilter::eat(uint64_t hash) {
  // Kirsch-Mitzenmacher double hashing: index_i = h1 + i*h2
  const uint32_t nbits = SEEN_FILTER_BYTES * 8;
  uint32_t h1 = (uint32_t)hash;
  uint32_t h2 = (uint32_t)(hash >> 32) | 1;
  bool isNew = false;
  for (uint32_t i = 0; i < SEEN_FILTER_HASHES; i++) {
    uint32_t bit = (h1 + i * h2) % nbits;
    uint8_t mask = 1 << (bit & 7);
    if (!(_bits[bit >> 3] & mask)) {
      _bits[bit >> 3] |= mask;
      isNew = true;
    }
  }
  if (isNew) _count++;
  return isNew;
}

bool SeenFilter::digestIfFull() {
  if (_count < SEEN_FILTER_CAPACITY) return false;
  memset(_bits, 0, sizeof(_bits));
  _count = 0;
  return true;
}
