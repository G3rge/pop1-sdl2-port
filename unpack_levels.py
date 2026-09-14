#!/usr/bin/env python3
"""unpack_levels.py - extrae los niveles de LEVELS.DAT de Prince of Persia 1.

Uso:
  python unpack_levels.py [nivel]        # extrae nivel (1..14) a data/levels/NN.bin
  python unpack_levels.py all            # extrae todos
  python unpack_levels.py info           # lista resources + datos basicos del nivel 1
"""

import struct
import os
import sys


def parse_dat(path):
    d = open(path, "rb").read()
    to, ts = struct.unpack("<IH", d[0:6])
    n = struct.unpack("<H", d[to:to + 2])[0]
    entries = []
    for i in range(n):
        b = to + 2 + 8 * i
        rid, off, size = struct.unpack("<HIH", d[b:b + 8])
        entries.append((rid, off, size))
    return d, entries


def get_res(d, entries, rid):
    for r, off, size in entries:
        if r == rid:
            return d[off + 1:off + 1 + size]  # +1 = checksum byte
    return None


# offets dentro del level_type de 2305 bytes
OFF_FG = 0                 # 720 front tiles
OFF_BG = 720               # 720 back tiles
OFF_START = 720 + 720 + 256 + 256 + 24 * 4 + 1 + 24 + 24 + 15  # 2064
LEN_START = 3              # start_room, start_pos, start_dir


def level_info(level, name):
    st = level[OFF_START:OFF_START + LEN_START]
    start_room, start_pos, start_dir = st[0], st[1], struct.unpack("b", bytes([st[2]]))[0]
    used_rooms = level[720 + 720 + 256 + 256 + 24 * 4]  # offset 1968
    counts = {}
    for t in level[OFF_FG:OFF_FG + 720]:
        counts[t] = counts.get(t, 0) + 1
    top = sorted(counts.items(), key=lambda kv: -kv[1])[:6]
    print(f"  {name}: used_rooms={used_rooms} start_room={start_room} "
          f"start_pos={start_pos} start_dir={start_dir} tiles_top={top}")


def main():
    base = os.path.dirname(os.path.abspath(__file__))
    path = os.path.join(base, "data", "LEVELS.DAT")
    if not os.path.exists(path):
        path = os.path.join(base, "prince", "LEVELS.DAT")
    if not os.path.exists(path):
        print("no encuentro LEVELS.DAT (busque data/ y prince/)")
        sys.exit(1)

    d, entries = parse_dat(path)
    rids = [r for r, _, _ in entries if 2000 <= r <= 2020]
    print(f"LEVELS.DAT resources {len(entries)}; levels = {len(rids)}")

    outdir = os.path.join(base, "data", "levels")
    os.makedirs(outdir, exist_ok=True)

    if "all" in sys.argv:
        for rid in rids:
            lv = get_res(d, entries, rid)
            fn = os.path.join(outdir, f"{rid:04d}.bin")
            open(fn, "wb").write(lv)
            level_info(lv, f"res {rid}")
        print(f"write -> {outdir}")
        return

    if "info" in sys.argv:
        lv = get_res(d, entries, 2001)
        if lv:
            level_info(lv, "res 2001 (nivel 1)")
        return

    try:
        n = int(sys.argv[1])
    except (IndexError, ValueError):
        n = 1
    rid = 2000 + n
    lv = get_res(d, entries, rid)
    if not lv:
        print(f"no existe res {rid}")
        sys.exit(1)
    fn = os.path.join(outdir, f"{rid:04d}.bin")
    open(fn, "wb").write(lv)
    level_info(lv, f"res {rid}")


if __name__ == "__main__":
    main()