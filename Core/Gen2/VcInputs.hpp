/*
 * Pokémon Crystal VC: the player's game files. Two inputs (SPEC v0.7 section 5.1; Victor 2026-11-25: a ROM alone is
 * enough, test 4 showed the VC patch never changes the encounter):
 *  1. a .gbc ROM (recommended), recognised by its SHA-1 (KNOWN_ROMS: FR, DE, IT, ES, EN Rev 0, EN Rev A);
 *  2. a decrypted CIA of the VC title: the ROM is taken from its romfs (rom/<code>) and gets the same check, and the title
 *     check hashes the ExeFS .code and romfs config.ini against the reference build (same emulator as the FR / EN titles).
 * An encrypted CIA can't be read: the caller gets a clear refusal.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 *
 * Port of the research prototype's vc_title_check.py and fr/romfs.py. Everything is read locally; no Nintendo file ships
 * with the tool, only the hashes below.
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

    /// A Crystal ROM the tool knows: full SHA-1 of the cartridge dump (Data/ROMS, 2026-11-25), language, revision.
    struct KnownRom
    {
        const char *sha1;
        const char *lang; // fr, de, it, es, en
        const char *language; // French, German, Italian, Spanish, English
        const char *revision; // "" or "Rev 0" / "Rev A"
        std::string label() const; // "French", "English Rev A"
    };
    /// JP is left out (different save layout, not supported).
    inline constexpr KnownRom KNOWN_ROMS[6] = {
        { "c055992b16b7399c687647725cdd1f4f13a2f75c", "fr", "French", "" },
        { "accb584293ba056152f1fd908439b019017ff2fe", "de", "German", "" },
        { "6cee05e5b95beeae74b8365ad18ec4a07a8c4af8", "it", "Italian", "" },
        { "889a06fc0bb863666865aa69def0adf97945ac2a", "es", "Spanish", "" },
        { "f4cd194bdee0d04ca4eac29e09b8e4e9d818c133", "en", "English", "Rev 0" },
        { "f2f52230b536214ef7c9924f483392993e226cfb", "en", "English", "Rev A" },
    };
    /// The known ROM with this SHA-1 (40 hex digits, any case), or null.
    const KnownRom *knownRomBySha1(const std::string &sha1);
    /// The known ROM these bytes are, or null.
    const KnownRom *identifyRom(const Bytes &rom);
    /// Language name of a language code ("fr" -> "French"); the code itself when unknown.
    std::string languageName(const std::string &lang);

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
        std::map<std::string, Bytes> romfs; // the Crystal ROM: "rom/<code>" (CGBBYT* / CGBBXT*)
        std::string romName; // "rom/CGBBYTF0.849"
        bool sameEmulator() const;
        /// the ROM inside the title (romfs rom/<code>, a cartridge dump)
        std::optional<Bytes> rom() const;
    };
    /// Reads a CIA (or a bare NCCH) from memory.
    CiaTitle readCia(const Bytes &cia);

    /// BLZ (bottom LZ) decompression of an ExeFS .code.
    Bytes blzDecompress(const Bytes &c);
}

#endif // VCINPUTS_HPP
