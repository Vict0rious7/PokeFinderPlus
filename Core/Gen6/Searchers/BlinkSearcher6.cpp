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
 * PokeFinderPlus: new file. XY Trainer ID RNG: initial TinyMT seed from the professor's blinks (6genTidSearch, BZL).
 */

#include "BlinkSearcher6.hpp"
#include <Core/Gen6/XYTid.hpp>
#include <algorithm>

namespace
{
    constexpr u64 BLOCK = 0x10000;
}

bool BlinkSearcher6::startSearch(int threads, const std::vector<u32> &gaps, u32 advanceMin, u32 advanceMax, u32 base, u32 count, bool near)
{
    if (gaps.empty() || advanceMin > advanceMax || advanceMax > 100000 || (near && count == 0))
    {
        return false;
    }

    u64 total = near ? count : 0x100000000ULL;
    setMaxProgress((total + BLOCK - 1) / BLOCK);
    index = 0;
    threads = std::max(1, threads);
    activeThreads.store(threads);
    for (int i = 0; i < threads; i++)
    {
        threadContainer.emplace_back([=, this] {
            search(gaps, advanceMin, advanceMax, near ? base : 0, total);
            activeThreads.fetch_sub(1);
        });
    }
    return true;
}

void BlinkSearcher6::search(std::vector<u32> gaps, u32 advanceMin, u32 advanceMax, u32 base, u64 count)
{
    // Blocks are handed out in order, so near a base seed the closest seeds are tried first
    for (u64 b = index.fetch_add(1); b * BLOCK < count; b = index.fetch_add(1))
    {
        if (cancelled.load(std::memory_order_relaxed))
        {
            return;
        }

        std::vector<BlinkResult6> hits;
        u64 end = std::min(count, (b + 1) * BLOCK);
        for (u64 i = b * BLOCK; i < end; i++)
        {
            u32 seed = base + static_cast<u32>(i);
            XYTid::Hit hit = XYTid::hitFrame(gaps, seed, advanceMin, advanceMax);
            if (hit.found)
            {
                hits.emplace_back(BlinkResult6 { seed, hit.advance, hit.state });
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
