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
 * PokeFinderPlus: new file. Checks the Gen 6 seed search from IVs on planted seeds (the IVs are read from the reference MT).
 */

#include "SeedSearcher6Test.hpp"
#include <Core/Gen6/Searchers/SeedSearcher6.hpp>
#include <Core/RNG/MT.hpp>
#include <QTest>
#include <thread>

namespace
{
    /// IVs of frame \p frame as poke6-seed-finder reads them: MT outputs 63 + frame .. 68 + frame, >> 27
    std::array<u8, 6> ivsAt(u32 seed, u32 frame)
    {
        MT mt(seed, 63 + frame);
        std::array<u8, 6> ivs;
        for (u8 &iv : ivs)
        {
            iv = mt.next() >> 27;
        }
        return ivs;
    }

    std::vector<SeedResult6> run(SeedSearcher6 &searcher)
    {
        std::vector<SeedResult6> results;
        while (searcher.isSearching())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            auto r = searcher.getResults();
            results.insert(results.end(), r.begin(), r.end());
        }
        auto r = searcher.getResults();
        results.insert(results.end(), r.begin(), r.end());
        return results;
    }
}

void SeedSearcher6Test::fullSearch_data()
{
    QTest::addColumn<u32>("seed");
    QTest::addColumn<u32>("frame1");
    QTest::addColumn<u32>("frame2");
    QTest::addColumn<u32>("min1");
    QTest::addColumn<u32>("max1");
    QTest::addColumn<u32>("min2");
    QTest::addColumn<u32>("max2");

    // retailrng's ranges: XY 750-900 / 1500-1900, ORAS 600-800 / 1500-1700; frames on the range edges too
    QTest::newRow("XY") << 0x8a2b1c3du << 812u << 1633u << 750u << 900u << 1500u << 1900u;
    QTest::newRow("ORAS") << 0x00012345u << 600u << 1700u << 600u << 800u << 1500u << 1700u;
    QTest::newRow("last lane") << 0xfffffff7u << 800u << 1500u << 600u << 800u << 1500u << 1700u;
}

void SeedSearcher6Test::fullSearch()
{
    QFETCH(u32, seed);
    QFETCH(u32, frame1);
    QFETCH(u32, frame2);
    QFETCH(u32, min1);
    QFETCH(u32, max1);
    QFETCH(u32, min2);
    QFETCH(u32, max2);

    SeedSearcher6 searcher;
    u32 high = seed >> 16;
    QVERIFY(searcher.startFullSearch(2, ivsAt(seed, frame1), { min1, max1 }, ivsAt(seed, frame2), { min2, max2 }, { high, high + 1 }));
    auto results = run(searcher);
    QCOMPARE(searcher.getProgress(), 100);

    bool found = false;
    for (const auto &r : results)
    {
        // every hit must really have the IVs at the reported frames
        QCOMPARE(ivsAt(r.seed, r.frame1), ivsAt(seed, frame1));
        QCOMPARE(ivsAt(r.seed, r.frame2), ivsAt(seed, frame2));
        found |= r.seed == seed && r.frame1 == frame1 && r.frame2 == frame2;
    }
    QVERIFY(found);
}

void SeedSearcher6Test::quickSearch()
{
    u32 base = 0x5d3e0000;
    u32 seed = base + 123457; // ~2 min of boot time after the base seed
    std::array<u8, 6> ivs = ivsAt(seed, 700);

    SeedSearcher6 searcher;
    QVERIFY(searcher.startQuickSearch(3, base, 200000, ivs, ivs, { 600, 800 }));
    auto results = run(searcher);
    bool found = false;
    for (const auto &r : results)
    {
        QVERIFY(r.seed >= base && r.seed < base + 200000);
        QCOMPARE(ivsAt(r.seed, r.frame1), ivs);
        found |= r.seed == seed && r.frame1 <= 700;
    }
    QVERIFY(found);

    // IV range: one stat off by one still matches a range around it
    std::array<u8, 6> min = ivs;
    std::array<u8, 6> max = ivs;
    min[2] = min[2] > 0 ? min[2] - 1 : 0;
    max[2] = max[2] < 31 ? max[2] + 1 : 31;
    SeedSearcher6 ranged;
    QVERIFY(ranged.startQuickSearch(1, seed - 3, 8, min, max, { 600, 800 }));
    found = false;
    for (const auto &r : run(ranged))
    {
        found |= r.seed == seed;
    }
    QVERIFY(found);
}

void SeedSearcher6Test::invalid()
{
    std::array<u8, 6> ivs = { 1, 2, 3, 4, 5, 6 };
    SeedSearcher6 a;
    QVERIFY(!a.startFullSearch(1, ivs, { 750, 900 }, ivs, { 903, 1500 })); // frame2 must start >= frame1 end + 6
    SeedSearcher6 b;
    QVERIFY(!b.startFullSearch(1, ivs, { 750, 900 }, ivs, { 1500, 3001 }));
    SeedSearcher6 c;
    std::array<u8, 6> bad = { 32, 0, 0, 0, 0, 0 };
    QVERIFY(!c.startQuickSearch(1, 0, 10, ivs, bad, { 600, 800 }));
}
