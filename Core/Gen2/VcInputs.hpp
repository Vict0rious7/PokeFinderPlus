/*
 * Pokémon Crystal VC: the player's game files. Three inputs (SPEC v0.6 section 2.1):
 *  1. a decrypted CIA of their VC title: the ROM and the title's patch file are taken from its romfs, and the title check
 *     hashes the ExeFS .code and romfs config.ini against the reference build (same emulator as the FR / EN titles);
 *  2. a ROM + the title's patch file;
 *  3. a cartridge dump alone (the VC patch never matters on the shrine path).
 * An encrypted CIA can't be read: the caller gets a clear refusal.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 *
 * Port of the research prototype's vc_title_check.py, fr/romfs.py and fr/applypatch.py. Everything is read locally; no
 * Nintendo file ships with the tool, and only hashes are reported.
 */

#ifndef VCINPUTS_HPP
#define VCINPUTS_HPP

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Gen2
{
    using Bytes = std::vector<std::uint8_t>;

    std::string sha1Hex(const std::uint8_t *p, std::size_t n);
    inline std::string sha1Hex(const Bytes &b)
    {
        return sha1Hex(b.data(), b.size());
    }

    /// Reference hashes of the Crystal VC emulator build (FR 0004000000172E00 = EN 0004000000172800; IT, DE, ES the same).
    namespace VcRef
    {
        constexpr const char *CODE = "9fb177cb5b0a7173a7343c507adb53a3292c923f"; // ExeFS .code as stored
        constexpr const char *CODE_DEC = "d48acf4c062884c9ef6b546c573db2125f5f9253"; // .code decompressed
        constexpr const char *CONFIG = "1baf63b8037d2b869a1d620665bb94e3e14357d4"; // romfs config.ini
    }

    /// Cartridge dumps by SHA-1 prefix: language code ("fr", "de", "es", "it", "en") and a label.
    std::optional<std::pair<std::string, std::string>> identifyDump(const Bytes &rom);

    enum class CiaError
    {
        None,
        NotCia, // no NCCH found and nothing suggests a CIA
        Encrypted, // CIA content or NCCH encrypted
        NoRom, // no Crystal ROM in the romfs
    };
    std::string ciaErrorText(CiaError e);

    struct CiaTitle
    {
        CiaError error = CiaError::None;
        std::uint64_t titleId = 0;
        std::string codeSha1, codeDecSha1, configSha1;
        std::map<std::string, Bytes> romfs; // the Crystal files: "rom/<code>" and "<code>.patch" (CGBBYT* / CGBBXT*)
        std::string romName; // "rom/CGBBYTF0.849"
        bool sameEmulator() const;
        /// the ROM inside the title and its own patch file applied
        std::optional<Bytes> patchedRom() const;
        std::optional<Bytes> dump() const;
    };
    /// Reads a CIA (or a bare NCCH) from memory.
    CiaTitle readCia(const Bytes &cia);

    /// BLZ (bottom LZ) decompression of an ExeFS .code.
    Bytes blzDecompress(const Bytes &c);
    /// Applies a VC patch file (.patch, INI-style sections; Mode=1 entries) to a ROM.
    Bytes applyPatch(const Bytes &rom, const std::string &patchText, int *applied = nullptr);
}

#endif // VCINPUTS_HPP
