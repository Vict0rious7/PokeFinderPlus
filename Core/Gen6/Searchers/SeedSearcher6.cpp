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

#include "SeedSearcher6.hpp"
#include <algorithm>
#include <memory>
#include <vector>

namespace
{
    // Misc consumption before the IVs: 60 + 1 + EC + PID (poke6-seed-finder's PRE_ADVANCE_FRAME)
    constexpr u32 PRE_ADVANCE = 63;
    constexpr int N = 624;
    constexpr int M = 397;
    constexpr int LANES = 8;
    constexpr int EXTRA = PRE_ADVANCE + SeedSearcher6::MAX_FRAME + 6 + 8;

    /**
     * @brief 8 Mersenne Twisters side by side (seeds s .. s + 7), extended only as far as needed
     *
     * Same idea as poke6-seed-finder's MultiMT19937. The lane loops are plain so the compiler vectorizes them
     * (SSE/AVX on x86, NEON on Apple silicon).
     */
    struct MultiMT
    {
        alignas(32) u32 state[N + EXTRA][LANES];
        int index;
        int size;

        void init(u32 seed)
        {
            for (int l = 0; l < LANES; l++)
            {
                state[0][l] = seed + l;
            }
            for (int i = 1; i < N; i++)
            {
                for (int l = 0; l < LANES; l++)
                {
                    state[i][l] = 1812433253 * (state[i - 1][l] ^ (state[i - 1][l] >> 30)) + i;
                }
            }
            index = 0;
            size = 0;
        }

        /// Makes outputs [0, count) available
        void reserve(int count)
        {
            for (int i = size; i < count; i++)
            {
                for (int l = 0; l < LANES; l++)
                {
                    u32 y = (state[i][l] & 0x80000000) | (state[i + 1][l] & 0x7fffffff);
                    state[i + N][l] = state[i + M][l] ^ (y >> 1) ^ ((0u - (y & 1)) & 0x9908b0df);
                }
            }
            size = count;
        }

        void discard(int count)
        {
            index += count;
        }

        /// Next output >> 27 (an IV), for all lanes
        void nextIV(u32 *out)
        {
            const u32 *x = state[N + index++];
            for (int l = 0; l < LANES; l++)
            {
                u32 y = x[l];
                y ^= y >> 11;
                y ^= (y << 7) & 0x9d2c5680;
                y ^= (y << 15) & 0xefc60000;
                y ^= y >> 18;
                out[l] = y >> 27;
            }
        }
    };

    u32 encode(const std::array<u8, 6> &ivs)
    {
        return (ivs[0] << 25) | (ivs[1] << 20) | (ivs[2] << 15) | (ivs[3] << 10) | (ivs[4] << 5) | ivs[5];
    }

    /**
     * @brief Slides a 6-IV window over frames [begin, end] (inclusive) for each lane; found[l] = last matching frame + 1, or 0
     *
     * Advances end + 6 - begin outputs.
     */
    void findFrame(MultiMT &mt, u32 ivs, u32 begin, u32 end, u32 *found)
    {
        u32 window[LANES];
        u32 iv[LANES];
        for (int l = 0; l < LANES; l++)
        {
            window[l] = 0;
            found[l] = 0;
        }
        for (int k = 0; k < 5; k++)
        {
            mt.nextIV(iv);
            for (int l = 0; l < LANES; l++)
            {
                window[l] = (window[l] << 5) | iv[l];
            }
        }
        for (u32 frame = begin; frame <= end; frame++)
        {
            mt.nextIV(iv);
            for (int l = 0; l < LANES; l++)
            {
                window[l] = ((window[l] & 0x1ffffff) << 5) | iv[l];
                found[l] = window[l] == ivs ? frame + 1 : found[l];
            }
        }
    }
}

bool SeedSearcher6::startFullSearch(int threads, const std::array<u8, 6> &ivs1, std::pair<u32, u32> frame1, const std::array<u8, 6> &ivs2,
                                    std::pair<u32, u32> frame2, std::pair<u32, u32> seedHigh)
{
    for (int i = 0; i < 6; i++)
    {
        if (ivs1[i] > 31 || ivs2[i] > 31)
        {
            return false;
        }
    }
    if (frame1.first > frame1.second || frame1.second + 6 > frame2.first || frame2.first > frame2.second || frame2.second > MAX_FRAME
        || seedHigh.first >= seedHigh.second || seedHigh.second > 0x10000)
    {
        return false;
    }

    setMaxProgress(seedHigh.second - seedHigh.first);
    index = seedHigh.first;
    u32 encoded1 = encode(ivs1);
    u32 encoded2 = encode(ivs2);
    threads = std::max(1, threads);
    activeThreads.store(threads);
    for (int i = 0; i < threads; i++)
    {
        threadContainer.emplace_back([=, this] {
            fullSearch(encoded1, frame1, encoded2, frame2, seedHigh);
            activeThreads.fetch_sub(1);
        });
    }
    return true;
}

bool SeedSearcher6::startQuickSearch(int threads, u32 baseSeed, u32 count, const std::array<u8, 6> &ivMin, const std::array<u8, 6> &ivMax,
                                     std::pair<u32, u32> frame)
{
    for (int i = 0; i < 6; i++)
    {
        if (ivMin[i] > ivMax[i] || ivMax[i] > 31)
        {
            return false;
        }
    }
    if (frame.first > frame.second || frame.second > MAX_FRAME || count == 0)
    {
        return false;
    }

    constexpr u32 block = 0x10000;
    setMaxProgress((static_cast<u64>(count) + block - 1) / block);
    index = 0;
    threads = std::max(1, threads);
    activeThreads.store(threads);
    for (int i = 0; i < threads; i++)
    {
        threadContainer.emplace_back([=, this] {
            quickSearch(baseSeed, count, ivMin, ivMax, frame);
            activeThreads.fetch_sub(1);
        });
    }
    return true;
}

void SeedSearcher6::fullSearch(u32 ivs1, std::pair<u32, u32> frame1, u32 ivs2, std::pair<u32, u32> frame2, std::pair<u32, u32> seedHigh)
{
    auto mt = std::make_unique<MultiMT>();
    u32 found1[LANES];
    u32 found2[LANES];
    for (u64 high = index.fetch_add(1); high < seedHigh.second; high = index.fetch_add(1))
    {
        if (cancelled.load(std::memory_order_relaxed))
        {
            return;
        }

        std::vector<SeedResult6> hits;
        u32 start = static_cast<u32>(high) << 16;
        for (u32 low = 0; low < 0x10000; low += LANES)
        {
            u32 seed = start | low;
            mt->init(seed);
            mt->reserve(PRE_ADVANCE + frame1.second + 6);
            mt->discard(PRE_ADVANCE + frame1.first);
            findFrame(*mt, ivs1, frame1.first, frame1.second, found1);

            bool any = false;
            for (int l = 0; l < LANES; l++)
            {
                any |= found1[l] != 0;
            }
            if (!any)
            {
                continue;
            }

            mt->reserve(PRE_ADVANCE + frame2.second + 6);
            mt->discard(frame2.first - frame1.second - 6);
            findFrame(*mt, ivs2, frame2.first, frame2.second, found2);
            for (int l = 0; l < LANES; l++)
            {
                if (found1[l] != 0 && found2[l] != 0)
                {
                    hits.emplace_back(SeedResult6 { seed + l, found1[l] - 1, found2[l] - 1 });
                }
            }
        }

        if (!hits.empty())
        {
            std::lock_guard<std::mutex> lock(mutex);
            results.insert(results.end(), hits.begin(), hits.end());
        }
        progress++;
    }
}

void SeedSearcher6::quickSearch(u32 baseSeed, u32 count, std::array<u8, 6> ivMin, std::array<u8, 6> ivMax, std::pair<u32, u32> frame)
{
    auto mt = std::make_unique<MultiMT>();
    constexpr u32 block = 0x10000;
    u32 iv[LANES];
    for (u64 b = index.fetch_add(1); b * block < count; b = index.fetch_add(1))
    {
        if (cancelled.load(std::memory_order_relaxed))
        {
            return;
        }

        std::vector<SeedResult6> hits;
        u32 end = static_cast<u32>(std::min<u64>(count, (b + 1) * block));
        for (u32 offset = static_cast<u32>(b * block); offset < end; offset += LANES)
        {
            u32 seed = baseSeed + offset;
            mt->init(seed);
            mt->reserve(PRE_ADVANCE + frame.second + 6);
            mt->discard(PRE_ADVANCE + frame.first);

            // 6 IVs per lane, slid one output per frame; first matching frame wins (as in GPU Tools' test_seed)
            u32 window[6][LANES];
            for (int k = 0; k < 6; k++)
            {
                mt->nextIV(window[k]);
            }
            u32 found[LANES] = {};
            int start = 0;
            for (u32 f = frame.first; f <= frame.second; f++)
            {
                for (int l = 0; l < LANES; l++)
                {
                    if (found[l] != 0)
                    {
                        continue;
                    }
                    bool valid = true;
                    for (int k = 0; k < 6 && valid; k++)
                    {
                        u32 value = window[(start + k) % 6][l];
                        valid = value >= ivMin[k] && value <= ivMax[k];
                    }
                    if (valid)
                    {
                        found[l] = f + 1;
                    }
                }
                if (f != frame.second)
                {
                    mt->nextIV(iv);
                    for (int l = 0; l < LANES; l++)
                    {
                        window[start][l] = iv[l];
                    }
                    start = (start + 1) % 6;
                }
            }
            for (int l = 0; l < LANES; l++)
            {
                if (found[l] != 0 && offset + l < end)
                {
                    hits.emplace_back(SeedResult6 { seed + l, found[l] - 1, 0 });
                }
            }
        }

        if (!hits.empty())
        {
            std::lock_guard<std::mutex> lock(mutex);
            results.insert(results.end(), hits.begin(), hits.end());
        }
        progress++;
    }
}
