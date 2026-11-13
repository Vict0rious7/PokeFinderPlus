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
 * PokeFinderPlus: new file. ORAS Trainer ID: base seed from the TIDs of new games launched at known times, as
 * 6genTidSearch (BZL, used with permission, https://blog.bzl-web.com/entry/2020/08/24/225424) does in its ORAS
 * 基準seed検索 tab.
 */

#ifndef ORASSEARCHER6_HPP
#define ORASSEARCHER6_HPP

#include <Core/Gen6/OrasTid.hpp>
#include <Core/Parents/Searchers/SearcherBase.hpp>
#include <vector>

/**
 * @brief Finds the ORAS base seed (the first launch's TinyMT seed) from the launches' times and TIDs, over every 32-bit seed
 */
class OrasSearcher6 : public SearcherBase<OrasTid::BaseMatch>
{
public:
    /// Results kept at most (6genTidSearch has no limit; more than a handful means the tolerance is too wide)
    static constexpr size_t MAX_RESULTS = 1000;

    OrasSearcher6() = default;

    /**
     * @brief Starts the search
     *
     * @param threads Number of threads
     * @param launches At least two launches, the first at 0 ms
     * @param tolerance Launch tolerance (± ms), at most 60000
     * @param start First seed tried (tests)
     * @param count Number of seeds tried, all of them by default (tests)
     *
     * @return false if the settings are invalid
     */
    bool startSearch(int threads, const std::vector<OrasTid::Launch> &launches, u32 tolerance, u32 start = 0, u64 count = 0x100000000ULL);

private:
    size_t found = 0; ///< results so far (under the mutex)

    void search(std::vector<OrasTid::Launch> launches, u32 tolerance, u32 start, u64 count);
};

#endif // ORASSEARCHER6_HPP
