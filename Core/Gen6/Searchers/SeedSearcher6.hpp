/*
 * This file is part of PokéFinder
 * Copyright (C) 2017-2024 by Admiral_Fish, bumba, and EzPzStreamz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * PokeFinderPlus: new file. Gen 6 initial seed search from IVs: port of poke6-seed-finder (ukikagi, MIT) for the full
 * search and of Gen 6 GPU Tools' IV Search (Lincoln-LM, GPL-3.0) for the quick search near a base seed.
 */

#ifndef SEEDSEARCHER6_HPP
#define SEEDSEARCHER6_HPP

#include <Core/Parents/Searchers/SearcherBase.hpp>
#include <array>

/**
 * @brief One initial seed found from IVs
 *
 * Frames follow poke6-seed-finder: frame f means the IVs are MT outputs 63 + f .. 68 + f
 * (60 + 1 + EC + PID before them).
 */
struct SeedResult6
{
    u32 seed;
    u32 frame1;
    u32 frame2; ///< 0 in a quick search (one Pokemon)
};

/**
 * @brief Finds the Gen 6 initial MT seed from the IVs of Pokemon received right after boot (retailrng's fossil method)
 *
 * Full search: every 32-bit seed, two Pokemon with exact IVs, each within its frame range (poke6-seed-finder).
 * Quick search: seeds from a base seed onwards (the seed grows by 1 per millisecond of boot time), one Pokemon with an IV
 * range (Gen 6 GPU Tools' IV Search).
 */
class SeedSearcher6 : public SearcherBase<SeedResult6>
{
public:
    /// Largest frame accepted (as in poke6-seed-finder)
    static constexpr u32 MAX_FRAME = 3000;

    SeedSearcher6() = default;

    /**
     * @brief Starts a full search over all 2^32 seeds
     *
     * @param threads Number of threads
     * @param ivs1 IVs of the first Pokemon (HP, Atk, Def, SpA, SpD, Spe)
     * @param frame1 Inclusive frame range of the first Pokemon
     * @param ivs2 IVs of the second Pokemon
     * @param frame2 Inclusive frame range of the second Pokemon, starting at least 6 frames after frame1's end
     * @param seedHigh Range of the seed's upper 16 bits to search, right-open (default: all)
     *
     * @return false if the ranges are invalid
     */
    bool startFullSearch(int threads, const std::array<u8, 6> &ivs1, std::pair<u32, u32> frame1, const std::array<u8, 6> &ivs2,
                         std::pair<u32, u32> frame2, std::pair<u32, u32> seedHigh = { 0, 0x10000 });

    /**
     * @brief Starts a quick search of \p count seeds from \p baseSeed
     *
     * @param threads Number of threads
     * @param baseSeed First seed (the base seed found with a full search)
     * @param count Number of seeds (GPU Tools: 0x4000000, i.e. ~18.6 h of boot time)
     * @param ivMin Minimum IVs of the Pokemon
     * @param ivMax Maximum IVs of the Pokemon
     * @param frame Inclusive frame range
     *
     * @return false if the range is invalid
     */
    bool startQuickSearch(int threads, u32 baseSeed, u32 count, const std::array<u8, 6> &ivMin, const std::array<u8, 6> &ivMax,
                          std::pair<u32, u32> frame);

private:
    void fullSearch(u32 ivs1, std::pair<u32, u32> frame1, u32 ivs2, std::pair<u32, u32> frame2, std::pair<u32, u32> seedHigh);

    void quickSearch(u32 baseSeed, u32 count, std::array<u8, 6> ivMin, std::array<u8, 6> ivMax, std::pair<u32, u32> frame);
};

#endif // SEEDSEARCHER6_HPP
