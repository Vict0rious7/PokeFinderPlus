"""PokeFinderPlus: pull the farmed FireRed/LeafGreen initial seed sheets into Core/Resources/FRLG/*.bin.

Ported from Ten Lines' src/wasm/generate_ten_lines_precalc.py (pull_frlg_seeds) by Lincoln-LM, GPL-3.0:
https://github.com/Lincoln-LM/ten-lines . Same sheets, same column mapping, same binary format, so the seeds
and times match the Ten Lines website.

FRLG seeds farmed by blisy, po, HunarPG, 10Ben, Real96, ColdStoneSys, Papa Jefe and Tono (see Ten Lines).

Run it on a computer that can reach docs.google.com, before a release:
    python3 Core/Resources/FRLG/pull_frlg_seeds.py
It overwrites the .bin files next to this script and writes timestamp.txt. Commit the result.

Optional check of the European sheets (Spanish/French/Italian/German, converted by Mucks) against the
English table + Ten Lines' European offsets:
    python3 Core/Resources/FRLG/pull_frlg_seeds.py --check-eu "path/to/EU Fire Red Seeds - Fire Red Seeds.csv"
"""

import argparse
import csv
import io
import os
import urllib.request
from datetime import datetime, timezone

OUT_DIR = os.path.dirname(os.path.abspath(__file__))

SHEET = "https://docs.google.com/spreadsheets/d/{}/gviz/tq?tqx=out:csv&{}"

# 14 columns of the English/mGBA GBA sheets, in sheet order
ENG_COLUMNS = (
    ("mono", "r", "a"), ("mono", "a", "a"), ("mono", "h", "a"),
    ("stereo", "r", "a"), ("stereo", "a", "a"), ("stereo", "h", "a"),
    ("mono", "r", "start"), ("mono", "a", "start"), ("mono", "h", "start"),
    ("stereo", "r", "start"), ("stereo", "a", "start"), ("stereo", "h", "start"),
    ("mono", "a", "l"), ("stereo", "a", "l"),
)  # fmt: skip
JPN_COLUMNS = ENG_COLUMNS[:6]

# name, sheet id, query, rows to skip, first seed column, columns, starting frame (None = Switch format)
SOURCES = (
    ("fr_eng", "1ZNchTvoCpHFVPBscEJZG3JaaqR41D8VVnbXb23fzc44", "gid=0", 1, 3, ENG_COLUMNS, 4062 // 2),
    ("lg_eng", "12TUcXGbLY_bBDfVsgWZKvqrX13U6XAATQZrYnzBKP6Y", "sheet=Leaf%20Green%20Seeds", 1, 3, ENG_COLUMNS, 4062 // 2),
    ("fr_jpn_1_0", "1xSYuAuGSZQ4JbgQN262cfo80_A2CYko74bYGzl5ABTA", "sheet=JPN%20Fire%20Red%201.0%20Seeds", 3, 1, JPN_COLUMNS, 2090 - 45),
    ("fr_jpn_1_1", "1aQeWaZSi1ycSytrNEOwxJNoEg-K4eItYagU_dh9VIeU", "sheet=JPN%20Fire%20Red%201.0%20Seeds", 3, 1, JPN_COLUMNS, 2090 - 45),
    ("lg_jpn", "1LSRVD0_zK6vyd6ettUDfaCFJbm00g451d8s96dqAbA4", "sheet=JPN%20Leaf%20Green%20Seeds", 3, 1, JPN_COLUMNS, 2090 - 41),
    ("fr_eng_mgba", "1aWo6FAjkLIut5TIKior4_04PlGessxUJhE8YWz_nQwc", "sheet=Fire%20Red%20Seeds", 2, 2, ENG_COLUMNS, 4062 // 2 - 45),
    ("lg_eng_mgba", "1YiQiII2v3AJK6RANMsQcBzVLk9dO6L99Zxt9FsCyKrI", "sheet=Leaf%20Green%20Seeds", 2, 2, ENG_COLUMNS, 4062 // 2 - 45),
    # Switch (NSO) sheets: column 1 is the seed time in ms; Ten Lines only reads these columns
    ("fr_eng_nx", "1mbn2-XAtmV7HZ1p4esgvUG710VX6FlfhN_HYL_zLJSk", "sheet=FireRed%20Seeds", 2, None,
     {2: ("mono", "h", "a"), 3: ("stereo", "h", "a"), 4: ("mono", "h", "start")}, None),
    ("lg_eng_nx", "1mbn2-XAtmV7HZ1p4esgvUG710VX6FlfhN_HYL_zLJSk", "sheet=LeafGreen%20Seeds", 2, None,
     {2: ("mono", "h", "a"), 3: ("stereo", "h", "a"), 4: ("mono", "h", "start"), 7: ("stereo", "r", "a")}, None),
    ("fr_jpn_nx", "1mbn2-XAtmV7HZ1p4esgvUG710VX6FlfhN_HYL_zLJSk", "sheet=JPN%20FireRed%20Seeds", 2, None, {2: ("mono", "h", "a")}, None),
    ("lg_jpn_nx", "1mbn2-XAtmV7HZ1p4esgvUG710VX6FlfhN_HYL_zLJSk", "sheet=JPN%20LeafGreen%20Seeds", 2, None, {2: ("mono", "h", "a")}, None),
)  # fmt: skip

# Ten Lines' offsets for the European versions (key "fr_eu"/"lg_eu" in initial_seed.hpp): EU = ENG + offset
EU_OFFSETS = {"r": -8, "a": -1, "h": 0}


def to_seed(text):
    """Ten Lines' add_str_seed: blank, '-' or anything above 0xFFFF marks an invalid entry (0x10000)."""
    text = text.strip()
    if text in ("", "-"):
        return 0x10000
    try:
        seed = int(text, 16)
    except ValueError:
        return 0x10000
    return seed if seed <= 0xFFFF else 0x10000


def fetch(sheet_id, query):
    request = urllib.request.Request(SHEET.format(sheet_id, query), headers={"User-Agent": "PokeFinderPlus"})
    with urllib.request.urlopen(request, timeout=30) as response:
        return list(csv.reader(io.StringIO(response.read().decode("utf-8"))))


def serialize(keys, starting_frame, seed_times=None):
    """Ten Lines' SeedDataStore / NXSeedDataStore format."""
    data = bytes()
    if seed_times is not None:
        data += len(seed_times).to_bytes(4, "little")
        for seed_time in seed_times:
            data += seed_time.to_bytes(2, "little")
    for key, seeds in keys.items():
        data += key.encode("utf-8") + b"\0"
        if seed_times is None:
            data += starting_frame.to_bytes(2, "little") + (1).to_bytes(1, "little")
        data += len(seeds).to_bytes(4, "little")
        for seed in seeds:
            data += seed.to_bytes(3, "little")
    return data


def pull():
    for name, sheet_id, query, skip, first, columns, starting_frame in SOURCES:
        rows = fetch(sheet_id, query)
        keys, seed_times = {}, ([] if starting_frame is None else None)
        mapping = columns if isinstance(columns, dict) else {first + i: c for i, c in enumerate(columns)}
        for row in rows[skip:]:
            if not row or not row[0]:
                continue
            if seed_times is not None:
                seed_times.append(int(row[1]))
            for column, (sound, mode, button) in mapping.items():
                keys.setdefault(f"{sound}_{mode}_{button}", []).append(to_seed(row[column]) if column < len(row) else 0x10000)
        with open(os.path.join(OUT_DIR, f"{name}.bin"), "wb") as f:
            f.write(serialize(keys, starting_frame, seed_times))
        print(f"{name}: {len(rows) - skip} rows, {len(keys)} settings")
    with open(os.path.join(OUT_DIR, "timestamp.txt"), "w", encoding="utf-8") as f:
        f.write(datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC"))


def check_eu(path):
    """Compare a European sheet with the English table + Ten Lines' offsets (same seeds expected)."""
    game = "lg_eng" if "Leaf" in os.path.basename(path) else "fr_eng"
    source = next(s for s in SOURCES if s[0] == game)
    eng_rows = fetch(source[1], source[2])[source[3]:]
    eu_rows = list(csv.reader(open(path, encoding="utf-8")))
    header = eu_rows[1]
    for j, name in enumerate(header[3:], start=3):
        sound, rest = name.split("/", 1)
        mode = {"LR": "r", "L=A": "a", "HELP": "h"}[rest.split(",")[0].strip()]
        button = rest.split(",")[1].strip().lower() if "," in rest else "a"
        column = source[4] + ENG_COLUMNS.index((sound.lower(), mode, button))
        eng = {to_seed(r[column]) for r in eng_rows if r and r[0] and column < len(r)} - {0x10000}
        eu = {to_seed(r[j]) for r in eu_rows[2:] if r and r[0].strip().isdigit() and j < len(r)} - {0x10000}
        shifted = {(s + EU_OFFSETS[mode]) & 0xFFFF for s in eng}
        print(f"{name:20} ENG {len(eng):5}  EU {len(eu):5}  ENG+offset in EU: {len(shifted & eu) / max(1, len(shifted)):.1%}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--check-eu", metavar="CSV", help="compare a European sheet CSV with ENG + Ten Lines' offsets")
    args = parser.parse_args()
    if args.check_eu:
        check_eu(args.check_eu)
    else:
        pull()
