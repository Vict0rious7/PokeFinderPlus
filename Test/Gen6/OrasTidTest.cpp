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
 * PokeFinderPlus: new file. ORAS TID tests (see OrasTidTest.hpp).
 */

#include "OrasTidTest.hpp"
#include <Core/Gen6/OrasTid.hpp>
#include <Core/Gen6/Searchers/OrasSearcher6.hpp>
#include <QFile>
#include <QTest>
#include <Test/Data.hpp>
#include <algorithm>
#include <chrono>
#include <thread>

namespace
{
    const json &golden()
    {
        static json data = [] {
            QFile file(":/data/orastid_6gentidsearch.json");
            if (!file.open(QIODevice::ReadOnly))
            {
                return json();
            }
            return json::parse(file.readAll().toStdString());
        }();
        return data;
    }

    std::string hex8(u32 v)
    {
        char buf[9];
        std::snprintf(buf, sizeof(buf), "%08X", v);
        return buf;
    }
}

void OrasTidTest::idOutput()
{
    const json &data = golden();
    QVERIFY(data["ids"].size() >= 200);
    for (const auto &c : data["ids"])
    {
        QCOMPARE(OrasTid::idOutput(c[0].get<u32>(), c[1].get<u32>()), c[2].get<u32>());
    }
}

void OrasTidTest::publishedExample()
{
    // e52301147.hatenablog.com, 2024-02-12: seed 0x5E367A67, 1397 advances by "No" → ID 00000-55098 (TID-SID)
    XYTid::Ids ids = XYTid::ids(OrasTid::idOutput(0x5E367A67, 1397));
    QCOMPARE(ids.tid, u16(0));
    QCOMPARE(ids.sid, u16(55098));
}

void OrasTidTest::identify()
{
    const json &data = golden();
    QVERIFY(data["offset"].size() >= 200);
    size_t rows = 0;
    for (const auto &c : data["offset"])
    {
        auto got = OrasTid::identify(c["seed"].get<u32>(), c["tid"].get<u16>(), c["no"].get<u32>(), c["tolerance"].get<u32>());
        const auto &want = c["rows"];
        QCOMPARE(got.size(), want.size());
        for (size_t i = 0; i < got.size(); i++)
        {
            const auto &w = want[i];
            QCOMPARE(hex8(got[i].seed), w[0].get<std::string>());
            QCOMPARE(std::to_string(got[i].offset), w[1].get<std::string>());
            QCOMPARE(std::to_string(got[i].ids.tid), w[2].get<std::string>());
            QCOMPARE(std::to_string(got[i].ids.sid), w[3].get<std::string>());
            QCOMPARE(std::to_string(got[i].ids.tsv), w[4].get<std::string>());
            QCOMPARE(std::to_string(got[i].ids.trv), w[5].get<std::string>());
        }
        rows += got.size();
    }
    QVERIFY(rows > 100);
}

void OrasTidTest::matchBase()
{
    const json &data = golden();
    u32 range = data["searchRange"].get<u32>();
    QVERIFY(data["search"].size() >= 60);
    size_t rows = 0, plantedFound = 0;
    for (const auto &c : data["search"])
    {
        std::vector<OrasTid::Launch> launches;
        for (int k = 0; k < 4; k++)
        {
            launches.emplace_back(OrasTid::Launch { c["seconds"][k].get<u32>() * 1000, c["tids"][k].get<u16>() });
        }
        u32 tolerance = c["tolerance"].get<u32>();
        std::vector<OrasTid::BaseMatch> got;
        for (u32 seed = 0; seed < range; seed++)
        {
            if (auto match = OrasTid::matchBase(seed, launches, tolerance))
            {
                got.push_back(*match);
            }
        }
        const auto &want = c["rows"];
        QCOMPARE(got.size(), want.size());
        for (size_t i = 0; i < got.size(); i++)
        {
            QCOMPARE(hex8(got[i].seed), want[i][0].get<std::string>());
            for (int k = 0; k < 4; k++)
            {
                QCOMPARE(std::to_string(got[i].offsets[k]), want[i][k + 1].get<std::string>());
            }
            plantedFound += got[i].seed == c["planted"].get<u32>();
        }
        rows += got.size();
    }
    QVERIFY(rows > 20);
    QVERIFY(plantedFound > 20);
}

void OrasTidTest::searcher()
{
    // A planted base seed with launch errors at both edges, found by the threaded searcher over a slice of seeds
    u32 base = 0x89ABCDEF;
    u32 tolerance = 300;
    std::vector<OrasTid::Launch> launches { { 0, OrasTid::tid(base) },
                                            { 180123, OrasTid::tid(base + 180123 - 300) },
                                            { 361000, OrasTid::tid(base + 361000 + 299) },
                                            { 540456, OrasTid::tid(base + 540456 + 17) } };
    OrasSearcher6 searcher;
    QVERIFY(!searcher.startSearch(2, { launches[0] }, tolerance));
    QVERIFY(searcher.startSearch(2, launches, tolerance, base - 0x30000, 0x60000));
    while (searcher.isSearching())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    auto results = searcher.getResults();
    auto it = std::find_if(results.begin(), results.end(), [=](const auto &r) { return r.seed == base; });
    QVERIFY(it != results.end());
    QCOMPARE(it->offsets, (std::vector<s32> { 0, -300, 299, 17 }));
}
