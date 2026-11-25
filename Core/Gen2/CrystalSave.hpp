/*
 * Pokémon Crystal save (international layout: FR, DE, IT, ES, EN): field read/write with both copies and both checksums,
 * and the minimal values that decide the GS Ball Celebi (read_minimal / apply_minimal).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 *
 * Port of the research prototype's savetool.py and savebuild.py (crystal-vc-celebi, 2026-11-18). WRAM symbols are the
 * pokecrystal (EN) names; the same addresses hold in the FR / DE / IT / ES saves (checked on Victor's FR save).
 * Checked byte for byte against the prototype (Test/Gen2).
 */

#ifndef CRYSTALSAVE_HPP
#define CRYSTALSAVE_HPP

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace Gen2
{
    enum class TextSpeed
    {
        Instant, // text delay 0 (only reachable by save editing)
        Fast,
        Mid,
        Slow
    };

    /// The values of a shrine save that change Celebi (SPEC section 2.1), plus the hidden ones (F, play seconds, clock
    /// offset, Kenji class).
    struct Minimal
    {
        TextSpeed textSpeed = TextSpeed::Fast;
        bool stereo = false;
        int frame = 1; // 1-8
        bool raikou = false, entei = false; // still roaming
        bool lass = true; // Lass standing left of the shrine
        bool swarm = false; // Dunsparce / Yanma swarm today
        int F = 0; // wGameTimeFrames 0-59
        int playSec = 0; // wGameTimeSeconds 0-59
        std::array<int, 4> start { 0, 0, 0, 0 }; // wStartDay, wStartHour, wStartMinute, wStartSecond (clock offset)
        int kenji = 2; // wKenjiBreakTimer class: 0, 1 or 2 (= 2 or more)
        bool setKenji = true; // apply_minimal writes the Kenji timer only when the value is given

        bool operator==(const Minimal &) const = default;
    };

    namespace Sym
    {
        constexpr int wStartDay = 0xd4b6;
        constexpr int wGameTimeSeconds = 0xd4c7;
        constexpr int wGameTimeFrames = 0xd4c8;
        constexpr int wKenjiBreakTimer = 0xdc58;
    }

    class CrystalSave
    {
    public:
        static constexpr int SIZE = 0x8000;

        CrystalSave() = default;
        /// The first 32 KB of a VC sav.dat (or a .sav). Shorter data is padded with 0.
        explicit CrystalSave(std::span<const std::uint8_t> data);

        /// Offset of a WRAM address in the main copy (or the backup copy); throws std::out_of_range outside the saved segments.
        static int offset(int addr, bool backup = false);

        std::vector<std::uint8_t> rd(int addr, int n = 1) const;
        std::uint8_t rd8(int addr) const;
        /// Writes both copies (main and backup). Checksums are fixed by bytes().
        void wr(int addr, std::span<const std::uint8_t> data);
        void wr8(int addr, std::uint8_t v);
        /// Options block (sOptions, 8 bytes): both copies.
        std::uint8_t opt(int i) const;
        void setOpt(int i, std::uint8_t v);

        /// Recomputes both checksums.
        CrystalSave &fix();
        /// The 32 KB with both checksums fixed (as the prototype's Save.bytes(), which also fixes this object).
        const std::vector<std::uint8_t> &bytes();
        const std::vector<std::uint8_t> &raw() const
        {
            return b;
        }

    private:
        std::vector<std::uint8_t> b = std::vector<std::uint8_t>(SIZE, 0);
    };

    int kenjiClass(int v);
    Minimal readMinimal(const CrystalSave &s);
    /// Template + minimal values -> equivalent shrine save (checksums not yet fixed: call bytes()).
    CrystalSave applyMinimal(const CrystalSave &tpl, const Minimal &m);

    std::string textSpeedName(TextSpeed t);
}

#endif // CRYSTALSAVE_HPP
