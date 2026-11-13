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
 * PokeFinderPlus: new file. ORAS base seed search (see OrasSearcher6.hpp).
 */

#include "OrasSearcher6.hpp"
#include <algorithm>

namespace
{
    constexpr u64 BLOCK = 0x10000;
}

bool OrasSearcher6::startSearch(int threads, const std::vector<OrasTid::Launch> &launches, u32 tolerance, u32 start, u64 count)
{
    if (launches.size() < 2 || launches[0].ms != 0 || tolerance > 60000 || count == 0 || count > 0x100000000ULL)
    {
        return false;
    }

    setMaxProgress((count + BLOCK - 1) / BLOCK);
    index = 0;
    threads = std::max(1, threads);
    activeThreads.store(threads);
    for (int i = 0; i < threads; i++)
    {
        threadContainer.emplace_back([=, this] {
            search(launches, tolerance, start, count);
            activeThreads.fetch_sub(1);
        });
    }
    return true;
}

void OrasSearcher6::search(std::vector<OrasTid::Launch> launches, u32 tolerance, u32 start, u64 count)
{
    for (u64 b = index.fetch_add(1); b * BLOCK < count; b = index.fetch_add(1))
    {
        if (cancelled.load(std::memory_order_relaxed))
        {
            return;
        }

        std::vector<OrasTid::BaseMatch> hits;
        u64 end = std::min(count, (b + 1) * BLOCK);
        for (u64 i = b * BLOCK; i < end; i++)
        {
            if (auto match = OrasTid::matchBase(start + static_cast<u32>(i), launches, tolerance))
            {
                hits.emplace_back(std::move(*match));
            }
        }

        if (!hits.empty())
        {
            std::lock_guard<std::mutex> lock(mutex);
            found += hits.size();
            results.insert(results.end(), hits.begin(), hits.end());
            if (found >= MAX_RESULTS)
            {
                cancelled.store(true, std::memory_order_relaxed);
            }
        }
        progress++;
    }
}
