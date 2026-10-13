// Ether-Beast dashboard: renders data.json, which the CoreInk PUTs via the GitHub API.
(function () {
  const FACE_X = 10, FACE_Y = 12;  // must match firmware config.h
  const $ = (id) => document.getElementById(id);

  function ago(iso) {
    if (!iso) return 'unknown';
    const s = Math.max(0, (Date.now() - Date.parse(iso)) / 1000);
    if (s < 90) return 'just now';
    if (s < 5400) return Math.round(s / 60) + ' min ago';
    if (s < 129600) return Math.round(s / 3600) + ' h ago';
    return Math.round(s / 86400) + ' days ago';
  }

  function drawMonster(stage, face) {
    const canvas = $('monster');
    const ctx = canvas.getContext('2d');
    const ink = getComputedStyle(document.documentElement).getPropertyValue('--ink').trim();
    ctx.clearRect(0, 0, 32, 32);
    ctx.fillStyle = ink;
    const sprites = window.ETHER_SPRITES;
    if (!sprites) return;
    const plot = (rows, ox, oy) => rows.forEach((row, y) => {
      for (let x = 0; x < row.length; x++) if (row[x] === '#') ctx.fillRect(ox + x, oy + y, 1, 1);
    });
    plot(sprites.bodies[Math.min(stage, sprites.bodies.length - 1)], 0, 0);
    if (sprites.faces[face]) plot(sprites.faces[face], FACE_X, FACE_Y);
  }

  function render(d) {
    $('level').textContent = `LV${d.Level} ${d.stage_name || ''}`;
    $('ready').classList.toggle('hidden', !d.ready_to_evolve);
    $('sugar').classList.toggle('hidden', !d.sugar_rush);
    $('starving').classList.toggle('hidden', !d.starving);

    drawMonster(d.stage || 0, d.face || 0);

    const floor = d.level_xp || 0;
    const next = d.next_level_xp || 1;
    const pct = Math.max(0, Math.min(100, ((d.Total_XP - floor) / Math.max(1, next - floor)) * 100));
    $('bar').style.width = pct + '%';
    $('barwrap').setAttribute('aria-valuenow', Math.round(pct));
    $('xp').textContent = `${d.Total_XP} / ${next} XP`;

    const last = d.last_forage || {};
    $('combo').textContent = last.combo > 1 ? `x${Number(last.combo).toFixed(1)} COMBO!` : '';
    $('eaten').textContent = (d.Total_Devices_Eaten || 0).toLocaleString();
    $('mood').textContent = (d.mood || '-').replace('_', ' ');
    $('fed').textContent = d.Last_Fed_Timestamp ? ago(d.Last_Fed_Timestamp) : 'never';
    $('lastForage').textContent = `${last.new_devices || 0} new, +${last.xp_gained || 0} XP`;
    $('cycles').textContent = (d.forage_cycles || 0).toLocaleString();
    $('battery').textContent = d.battery != null ? d.battery + '%' : '-';
    $('synced').textContent = d.synced_at
      ? `Last synced ${ago(d.synced_at)} (${new Date(d.synced_at).toLocaleString()})`
      : 'Waiting for the first sync from the device.';
  }

  async function load() {
    try {
      const res = await fetch('data.json?t=' + Date.now(), { cache: 'no-store' });
      if (!res.ok) throw new Error('HTTP ' + res.status);
      render(await res.json());
    } catch (e) {
      $('synced').innerHTML = '';
      const p = document.createElement('span');
      p.className = 'error';
      p.textContent = 'Could not load data.json: ' + e.message;
      $('synced').appendChild(p);
    }
  }

  load();
  setInterval(load, 5 * 60 * 1000);
})();
