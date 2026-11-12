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
 * PokeFinderPlus: new file. XY Trainer ID RNG: initial TinyMT seed from the professor's blinks, as 6genTidSearch
 * (BZL, used with permission, https://blog.bzl-web.com/entry/2020/08/31/202306) does in its 基準seed検索 tab.
 */

#ifndef BLINKSEARCHER6_HPP
#define BLINKSEARCHER6_HPP

#include <Core/Parents/Searchers/SearcherBase.hpp>
#include <array>
#include <vector>

/**
 * @brief One initial seed that explains the tapped blinks
 */
struct BlinkResult6
{
    u32 seed;
    u32 advance; ///< TinyMT output (from the initial seed) that gave the first tapped gap
    std::array<u32, 4> state; ///< TinyMT state at the first tap (6genTidSearch's 生成開始seed, status[0..3])
};

/**
 * @brief Finds the XY initial TinyMT seed from the gaps between the professor's blinks
 *
 * Full search: every 32-bit seed. Near a base seed: base, base + 1, ... (a later boot of the same save), so the right seed
 * comes up within seconds. Matching follows 6genTidSearch (±10 frames per gap).
 */
class BlinkSearcher6 : public SearcherBase<BlinkResult6>
{
public:
    /// 6genTidSearch lists at most this many results
    static constexpr size_t MAX_RESULTS = 1000;

    BlinkSearcher6() = default;

    /**
     * @brief Starts the search
     *
     * @param threads Number of threads
     * @param gaps Frames between consecutive blinks (at least one)
     * @param advanceMin First TinyMT output the first gap may come from
     * @param advanceMax Last one
     * @param base First seed when \p near is set
     * @param count Number of seeds from \p base when \p near is set (ignored otherwise)
     * @param near Search from \p base instead of all seeds
     *
     * @return false if the settings are invalid
     */
    bool startSearch(int threads, const std::vector<u32> &gaps, u32 advanceMin, u32 advanceMax, u32 base, u32 count, bool near);

private:
    size_t found = 0; ///< results so far (under the mutex)

    void search(std::vector<u32> gaps, u32 advanceMin, u32 advanceMax, u32 base, u64 count);
};

#endif // BLINKSEARCHER6_HPP
