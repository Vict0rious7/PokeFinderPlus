/*
 * PokeFinderPlus: new file. Game files for the Gen 2 tests, found at run time (never in the repository).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 *
 * POKEFINDERPLUS_GBC_ROMS: a folder with the player's Crystal ROMs (any file names; each is recognised by its hash).
 * POKEFINDERPLUS_GEN2_SAVES: a folder with the saves the goldens were made with (any file names; recognised by the hash
 * of their first 32 KB). Tests that need a missing file are skipped, and countSkip() records why.
 */

#ifndef GEN2TESTFILES_HPP
#define GEN2TESTFILES_HPP

#include <Core/Gen2/CelebiPlan.hpp>
#include <Core/Gen2/CrystalSave.hpp>
#include <Test/Data.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Gen2Test
{
    using Bytes = std::vector<std::uint8_t>;

    /// FNV-1a 64 of a byte range, as hex (same as make_goldens.py's fnv64).
    std::string fnv64(const std::uint8_t *p, std::size_t n);

    /// Golden file from the test resources (:/data/<name>).
    const json &golden(const std::string &name);

    /// ROM whose FNV-1a 64 is fnvHex, from the $POKEFINDERPLUS_GBC_ROMS folders; null when missing.
    std::shared_ptr<const Bytes> rom(const std::string &fnvHex);
    /// Every file of the $POKEFINDERPLUS_GBC_ROMS folders (several, separated by ':' or ';' on Windows), by FNV-1a 64.
    const std::map<std::string, std::shared_ptr<const Bytes>> &romFiles();
    /// First 32 KB of the save whose first 32 KB hash to fnvHex, from $POKEFINDERPLUS_GEN2_SAVES; empty when missing.
    std::optional<Bytes> save(const std::string &fnvHex);

    /// Records a skipped test and its reason; summary() prints the count at the end of the run.
    void countSkip(const std::string &what);
    std::string summary();

    /// Joypad timeline from the golden run-length form [[start, end, mask], ...] over n VBlanks.
    Bytes joyFromRle(const json &rle, int n);

    /// Minimal values from the prototype's read_minimal / apply_minimal dict.
    Gen2::Minimal minimalFromJson(const json &m);
    json minimalToJson(const Gen2::Minimal &m);

    /// Plan from the prototype's plan json (discover output).
    Gen2::Plan planFromJson(const json &p);
    json planToJson(const Gen2::Plan &p);
    Gen2::Rtc rtcFromJson(const json &r);

    /// The VC ROM of a language (hash from golden2/core.json); null when missing.
    Gen2::RomPtr langRom(const std::string &lang);
    /// Victor's save / the EN test save (hash from golden2/core.json); empty when missing.
    std::optional<Bytes> namedSave(const std::string &name);
}

#endif // GEN2TESTFILES_HPP
