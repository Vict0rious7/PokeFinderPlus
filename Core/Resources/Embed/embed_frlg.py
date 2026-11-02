# PokeFinderPlus (unofficial fork of PokeFinder): embeds the FRLG initial seed tables pulled by
# Resources/FRLG/pull_frlg_seeds.py (Ten Lines' format). Missing files embed as empty arrays.
import compression.zstd as zstd
from pathlib import Path

from .embed_util import write_data

FILES = ("fr_eng", "lg_eng", "fr_jpn_1_0", "fr_jpn_1_1", "lg_jpn", "fr_eng_mgba", "lg_eng_mgba",
         "fr_eng_nx", "lg_eng_nx", "fr_jpn_nx", "lg_jpn_nx")


def embed_frlg(parent_dir: str, output_dir: str):
    arrays = ["struct FRLGSeedFile\n{\n    const char *name;\n    const u8 *data;\n    u32 size;\n};"]
    entries = []
    for name in FILES:
        path = Path(parent_dir) / "FRLG" / f"{name}.bin"
        data = zstd.compress(path.read_bytes(), 22) if path.exists() else b""
        values = ", ".join(str(b) for b in data) if data else "0"
        arrays.append(f"constexpr std::array<u8, {max(1, len(data))}> FRLG_{name.upper()} = {{ {values} }};")
        entries.append(f'{{ "{name}", FRLG_{name.upper()}.data(), {len(data)} }}')
    arrays.append(f"constexpr std::array<FRLGSeedFile, {len(FILES)}> FRLG_SEED_FILES = {{ {{ {', '.join(entries)} }} }};")

    timestamp = Path(parent_dir) / "FRLG" / "timestamp.txt"
    stamp = timestamp.read_text(encoding="utf-8").strip() if timestamp.exists() else ""
    arrays.append(f'constexpr const char *FRLG_SEEDS_TIMESTAMP = "{stamp}";')

    write_data(arrays, f"{output_dir}/FRLGSeeds.hpp", ("Core/Global.hpp", "array"))
