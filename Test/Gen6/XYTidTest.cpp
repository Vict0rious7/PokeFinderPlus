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
 * PokeFinderPlus: new file. Checks the XY TID code against 6genTidSearch's own code (Test/Gen6/xytid_6gentidsearch.json).
 */

#include "XYTidTest.hpp"
#include <Core/Gen6/Generators/IDGenerator6.hpp>
#include <Core/Gen6/Searchers/BlinkSearcher6.hpp>
#include <Core/Gen6/XYTid.hpp>
#include <Core/RNG/TinyMT.hpp>
#include <QFile>
#include <QTest>
#include <Test/Data.hpp>

namespace
{
    const json &golden()
    {
        static json data = [] {
            QFile file(":/data/xytid_6gentidsearch.json");
            if (!file.open(QIODevice::ReadOnly))
            {
                return json();
            }
            return json::parse(file.readAll().toStdString());
        }();
        return data;
    }

    std::array<u32, 4> words(const json &j)
    {
        return { j[0].get<u32>(), j[1].get<u32>(), j[2].get<u32>(), j[3].get<u32>() };
    }
}

void XYTidTest::tinyMT()
{
    const json &data = golden();
    QVERIFY(data["tinymt"].size() > 200);
    for (const auto &c : data["tinymt"])
    {
        u32 seed = c["seed"].get<u32>();
        XYTid::Tiny mt = XYTid::Tiny::fromSeed(seed);
        QCOMPARE(mt.s, words(c["status"]));
        TinyMT pokefinder(seed); // PokéFinder's own TinyMT must agree too
        for (const auto &out : c["out"])
        {
            u32 r = mt.next();
            QCOMPARE(r, out.get<u32>());
            QCOMPARE(pokefinder.next(), r);
        }
    }
    for (const auto &c : data["tinymt_state"])
    {
        XYTid::Tiny mt = XYTid::Tiny::fromState(words(c["state"]));
        for (const auto &out : c["out"])
        {
            QCOMPARE(mt.next(), out.get<u32>());
        }
    }
}

void XYTidTest::blinkInterval()
{
    for (const auto &c : golden()["calcblink"])
    {
        QCOMPARE(XYTid::blinkInterval(c[0].get<u32>()), c[1].get<u32>());
    }
}

void XYTidTest::hitFrame()
{
    const json &cases = golden()["search"];
    QVERIFY(cases.size() >= 3000);
    int found = 0;
    for (const auto &c : cases)
    {
        std::vector<u32> gaps = c["gaps"].get<std::vector<u32>>();
        XYTid::Hit hit = XYTid::hitFrame(gaps, c["seed"].get<u32>(), c["fmin"].get<u32>(), c["fmax"].get<u32>());
        QCOMPARE(hit.found, c["found"].get<bool>());
        if (hit.found)
        {
            found++;
            const json &h = c["hit"];
            QCOMPARE(hit.advance, h[0].get<u32>());
            // 6genTidSearch returns status[3], [2], [1], [0]
            QCOMPARE(hit.state[3], h[1].get<u32>());
            QCOMPARE(hit.state[2], h[2].get<u32>());
            QCOMPARE(hit.state[1], h[3].get<u32>());
            QCOMPARE(hit.state[0], h[4].get<u32>());
        }
    }
    QVERIFY(found > 1000); // both outcomes are covered
}

void XYTidTest::timeline_data()
{
    QTest::addColumn<int>("number");
    for (int i = 0; i < static_cast<int>(golden()["timeline"].size()); i++)
    {
        QTest::newRow(std::to_string(i).c_str()) << i;
    }
}

void XYTidTest::timeline()
{
    QFETCH(int, number);
    const json &c = golden()["timeline"][number];
    bool boy = c["male"].get<bool>();
    XYTid::Session session(words(c["state"]), c["elapsedMs"].get<u32>());
    std::vector<u32> blinks;
    for (const auto &n : c["nudges"])
    {
        session.advanceTo(n[0].get<u32>() - 1, &blinks);
        session.nudge(n[1].get<int>());
    }
    const json &steps = c["steps"];
    for (int i = 0; i < 4; i++)
    {
        u32 press = steps[i].get<u32>();
        session.advanceTo(press, &blinks);
        QCOMPARE(session.step(press, boy), i + 1);
    }
    QVERIFY(!session.hasSnapshot());
    session.advanceTo(steps[3].get<u32>() + (boy ? XYTid::STEP4_RESET_BOY : XYTid::STEP4_RESET_GIRL), &blinks);
    QVERIFY(session.hasSnapshot());

    const json &snap = c["snapshot"];
    QCOMPARE(session.getSnapshot().getState(), words(snap["seed"]));
    QCOMPARE(session.getSnapshot().getNpc(), snap["npc"].get<u32>());
    for (int i = 0; i < 4; i++)
    {
        QCOMPARE(session.getSnapshot().getCounters()[i], snap["counter"][i].get<u32>());
    }

    const json &expected = c["blinks"];
    QVERIFY(blinks.size() >= expected.size());
    for (size_t i = 0; i < expected.size(); i++)
    {
        QCOMPARE(blinks[i], expected[i].get<u32>());
    }

    const json &rows = c["rows"];
    std::vector<XYTid::Row> list = XYTid::waitList(session.getSnapshot(), boy, static_cast<u32>(rows.size()));
    QCOMPARE(list.size(), rows.size());
    for (size_t i = 0; i < list.size(); i++)
    {
        const json &r = rows[i];
        QCOMPARE(list[i].offset, r[0].get<u32>());
        QCOMPARE(list[i].id.tid, r[1].get<u16>());
        QCOMPARE(list[i].id.sid, r[2].get<u16>());
        QCOMPARE(list[i].id.tsv, r[3].get<u16>());
        QCOMPARE(static_cast<u16>(list[i].id.trv), r[4].get<u16>());
        QCOMPARE(list[i].wait, r[5].get<u32>());
        QCOMPARE(list[i].window, r[6].get<u32>());
    }
}

void XYTidTest::searchNearBase()
{
    // Planted seed: taps with a few frames of error must find it, near a base seed below it
    const u32 seed = 0x5EED1234;
    XYTid::Tiny mt = XYTid::Tiny::fromSeed(seed);
    mt.advance(11);
    std::array<u32, 4> state = mt.s;
    std::vector<u32> gaps;
    const int jitter[] = { 3, -2, 0, 4, -4, 1, -1, 2, -3, 0 };
    for (int j : jitter)
    {
        gaps.push_back(XYTid::blinkInterval(mt.next()) + j);
    }

    BlinkSearcher6 searcher;
    QVERIFY(!searcher.startSearch(1, {}, 5, 20, 0, 10, true));
    QVERIFY(searcher.startSearch(2, gaps, XYTid::DEFAULT_ADVANCE_MIN, XYTid::DEFAULT_ADVANCE_MAX, seed - 70000, 200000, true));
    while (searcher.isSearching())
    {
        QTest::qWait(10);
    }
    std::vector<BlinkResult6> results = searcher.getResults();
    auto it = std::find_if(results.begin(), results.end(), [seed](const BlinkResult6 &r) { return r.seed == seed; });
    QVERIFY(it != results.end());
    QCOMPARE(it->advance, 11u);
    QCOMPARE(it->state, state);
    QCOMPARE(searcher.getProgress(), 100);
}

void XYTidTest::idList()
{
    std::array<u32, 4> state = { 0x11111111, 0x22222222, 0x33333333, 0x44444444 };
    std::vector<IDState6> all = generateIDs6(state, 0, 199, IDFilter6 {});
    QCOMPARE(all.size(), 200u);
    XYTid::Tiny mt = XYTid::Tiny::fromState(state);
    for (const auto &id : all)
    {
        u32 r = mt.next();
        QCOMPARE(id.getTID(), static_cast<u16>(r & 0xffff));
        QCOMPARE(id.getSID(), static_cast<u16>(r >> 16));
        QCOMPARE(id.getTSV(), static_cast<u16>(((r & 0xffff) ^ (r >> 16)) >> 4));
        QCOMPARE(id.getTRV(), static_cast<u8>(((r & 0xffff) ^ (r >> 16)) & 15));
    }

    IDFilter6 filter;
    filter.tid = all[57].getTID();
    std::vector<IDState6> one = generateIDs6(state, 10, 199, filter);
    QVERIFY(!one.empty());
    QCOMPARE(one.front().getAdvances(), 57u);

    std::vector<IDState6> later = generateIDs6(state, 150, 160, IDFilter6 {});
    QCOMPARE(later.size(), 11u);
    QCOMPARE(later.front().getTID(), all[150].getTID());
}

void XYTidTest::handoff()
{
    XYTid::Handoff h;
    h.game = 'Y';
    h.state = { 1, 0xabcdef01, 3, 0xffffffff };
    h.tapEpochMs = 1794482303966LL;
    h.target = 165;
    h.tid = 12345;
    h.sid = 54321;
    std::string text = h.toString();
    QCOMPARE(text,
             std::string("PokeTimerPlus XYTID 1 game=Y state=FFFFFFFF,00000003,ABCDEF01,00000001 tap=1794482303966 target=165 "
                         "tid=12345 sid=54321"));
    auto back = XYTid::Handoff::fromString(" " + text + " unknown=1\n");
    QVERIFY(back.has_value());
    QCOMPARE(back->game, 'Y');
    QCOMPARE(back->state, h.state);
    QCOMPARE(back->tapEpochMs, h.tapEpochMs);
    QCOMPARE(*back->target, 165u);
    QCOMPARE(*back->tid, static_cast<u16>(12345));
    QCOMPARE(*back->sid, static_cast<u16>(54321));
    QVERIFY(!XYTid::Handoff::fromString("hello").has_value());
    QVERIFY(!XYTid::Handoff::fromString("PokeTimerPlus XYTID 1 game=X tap=5").has_value());
    QVERIFY(!XYTid::Handoff::fromString("PokeTimerPlus XYTID 1 state=1,2,3 tap=5").has_value());
    QVERIFY(!XYTid::Handoff::fromString("PokeTimerPlus XYTID 1 state=1,2,3,ZZ tap=5").has_value());
}
