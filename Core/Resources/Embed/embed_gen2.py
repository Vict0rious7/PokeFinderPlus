# PokeFinderPlus (unofficial fork of PokeFinder): embeds the Gen 2 Celebi save template (Resources/Gen2/crystal_template.sav,
# made by Resources/Gen2/make_template.py: a Crystal shrine save with every name and the trainer ID blanked).
from pathlib import Path

from .embed_util import write_data


def embed_gen2(parent_dir: str, output_dir: str):
    data = (Path(parent_dir) / "Gen2" / "crystal_template.sav").read_bytes()
    values = ", ".join(str(b) for b in data)
    arrays = [f"constexpr std::array<u8, {len(data)}> GEN2_CRYSTAL_TEMPLATE = {{ {values} }};"]
    write_data(arrays, f"{output_dir}/Gen2Template.hpp", ("Core/Global.hpp", "array"))
