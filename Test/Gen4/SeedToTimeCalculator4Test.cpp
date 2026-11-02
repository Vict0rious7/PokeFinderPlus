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
 * Modified 2026-11-02 by Vict0rious7 for PokeFinderPlus (unofficial fork):
 * added tests for the "Today" seed picker.
 */

#include "SeedToTimeCalculator4Test.hpp"
#include <Core/Gen4/Tools/SeedToTimeCalculator4.hpp>
#include <QTest>
#include <Test/Data.hpp>

static bool operator==(const SeedTime4 &left, const json &right)
{
    auto dt = left.getDateTime();
    auto date = dt.getDate();
    auto time = dt.getTime();

    return date.year() == right["year"].get<u16>() && date.month() == right["month"].get<u8>() && date.day() == right["day"].get<u8>()
        && time.hour() == right["hour"].get<u8>() && time.minute() == right["minute"].get<u8>()
        && time.second() == right["second"].get<u8>() && left.getDelay() == right["delay"].get<u32>();
}

void SeedToTimeCalculator4Test::calculateTimes_data()
{
    QTest::addColumn<u32>("seed");
    QTest::addColumn<u16>("year");
    QTest::addColumn<std::string>("results");

    json data = readData("seedtotime4", "calculateTimes");
    for (const auto &d : data)
    {
        QTest::newRow(d["name"].get<std::string>().data())
            << d["seed"].get<u32>() << d["year"].get<u16>() << d["results"].get<json>().dump();
    }
}

void SeedToTimeCalculator4Test::calculateTimes()
{
    QFETCH(u32, seed);
    QFETCH(u16, year);
    QFETCH(std::string, results);

    json j = json::parse(results);

    auto states = SeedToTimeCalculator4::calculateTimes(seed, year, true, 0);
    QCOMPARE(states.size(), j.size());

    for (size_t i = 0; i < states.size(); i++)
    {
        const auto &state = states[i];
        QVERIFY(state == j[i]);
    }
}

// PokeFinderPlus: reference cases from the seed picker plan (28 Oct 2026 in-game checks and the 28 Oct seeds.txt ranking)
void SeedToTimeCalculator4Test::parseSeedList()
{
    u32 rejected;
    std::string text = "Seed\tAdvances\tPID\n"
                       "661202B4\t554\t12345678\n"
                       "\n"
                       "  0x691202f9, 745\n"
                       "661202B4\t554\n"
                       "ZZZ\n"
                       "123456789\n"
                       "2c;1\r\n";
    auto seeds = SeedToTimeCalculator4::parseSeedList(text, rejected);

    std::vector<u32> expected = { 0x661202B4, 0x691202F9, 0x2C };
    QCOMPARE(seeds, expected);
    QCOMPARE(rejected, 3u); // header, ZZZ, 9 digits
}

// calculateTimesToday() must return exactly the rows of calculateTimes() for that day, from the next full minute on
void SeedToTimeCalculator4Test::calculateTimesToday()
{
    std::vector<u32> seeds = { 0x53130DBC, 0x661202B4, 0x80140E3D, 0x620A032C, 0xF10A042A, 0xA60F0FF8, 0x00000000, 0xFF2A1234 };
    std::vector<DateTime> nows = { DateTime(2026, 10, 28, 18, 43, 10), DateTime(2026, 2, 28, 0, 0, 0), DateTime(2024, 2, 29, 9, 59, 59),
                                   DateTime(2026, 12, 31, 23, 0, 0), DateTime(2000, 1, 1, 0, 0, 0) };
    for (const auto &now : nows)
    {
        for (u32 seed : seeds)
        {
            std::vector<SeedTime4> expected;
            for (const auto &result : SeedToTimeCalculator4::calculateTimes(seed, now.getDate().year(), false, 0))
            {
                DateTime dt = result.getDateTime();
                Time t = dt.getTime(), n = now.getTime();
                if (dt.getDate() == now.getDate() && (t.hour() > n.hour() || (t.hour() == n.hour() && t.minute() > n.minute())))
                {
                    expected.emplace_back(result);
                }
            }

            auto today = SeedToTimeCalculator4::calculateTimesToday(seed, now);
            QCOMPARE(today.size(), expected.size());
            for (size_t i = 0; i < today.size(); i++)
            {
                QCOMPARE(today[i].getDateTime().toString(), expected[i].getDateTime().toString());
                QCOMPARE(today[i].getDelay(), expected[i].getDelay());
            }
        }
    }
}

void SeedToTimeCalculator4Test::rankToday_data()
{
    QTest::addColumn<std::vector<u32>>("seeds");
    QTest::addColumn<DateTime>("now");
    QTest::addColumn<int>("parity");
    QTest::addColumn<std::vector<u32>>("order");
    QTest::addColumn<std::vector<u32>>("tries");
    QTest::addColumn<u32>("delay");
    QTest::addColumn<std::string>("first");
    QTest::addColumn<std::string>("last");

    std::vector<u32> all = { 0x53130DBC, 0x541305C8, 0x53131697, 0x511306F7, 0x661202B4, 0x691202F9, 0x80140E3D, 0x620A032C, 0xF10A042A };

    // 10:xx, clock unchanged: 620A032C reachable (S = 74, minutes 15-59), F10A042A not
    QTest::newRow("28 Oct 10:00 620A032C") << std::vector<u32> { 0xF10A042A, 0x620A032C } << DateTime(2026, 10, 28, 10, 0, 30) << -1
                                           << std::vector<u32> { 0x620A032C } << std::vector<u32> { 45 } << 786u
                                           << std::string("2026-10-28 10:15:59") << std::string("2026-10-28 10:59:15");

    // 18:43: current hour first, so the 18:xx seeds beat the 60-try 19:xx seeds
    QTest::newRow("28 Oct 18:43 even") << all << DateTime(2026, 10, 28, 18, 43, 10) << 0
                                       << std::vector<u32> { 0x661202B4, 0x53130DBC, 0x541305C8 } << std::vector<u32> { 16, 60, 59 } << 666u
                                       << std::string("2026-10-28 18:44:34") << std::string("2026-10-28 18:59:19");
    QTest::newRow("28 Oct 18:43 odd") << all << DateTime(2026, 10, 28, 18, 43, 10) << 1
                                      << std::vector<u32> { 0x691202F9, 0x53131697, 0x511306F7, 0x80140E3D }
                                      << std::vector<u32> { 16, 60, 58, 15 } << 735u << std::string("2026-10-28 18:44:37")
                                      << std::string("2026-10-28 18:59:22");

    // 19:00 hour, strict ranking: most tries beats shortest delay
    QTest::newRow("28 Oct 19h even") << all << DateTime(2026, 10, 28, 18, 59, 30) << 0 << std::vector<u32> { 0x53130DBC, 0x541305C8 }
                                     << std::vector<u32> { 60, 59 } << 3490u << std::string("2026-10-28 19:00:59")
                                     << std::string("2026-10-28 19:59:00");
    QTest::newRow("28 Oct 19h odd") << all << DateTime(2026, 10, 28, 18, 59, 30) << 1
                                    << std::vector<u32> { 0x53131697, 0x511306F7, 0x80140E3D } << std::vector<u32> { 60, 58, 15 } << 5757u
                                    << std::string("2026-10-28 19:00:59") << std::string("2026-10-28 19:59:00");

    // 80140E3D: 20:45:59 -> 20:59:45, 15 tries
    QTest::newRow("28 Oct 80140E3D") << std::vector<u32> { 0x80140E3D } << DateTime(2026, 10, 28, 20, 0, 0) << -1
                                     << std::vector<u32> { 0x80140E3D } << std::vector<u32> { 15 } << 3619u
                                     << std::string("2026-10-28 20:45:59") << std::string("2026-10-28 20:59:45");

    // Same maths as Seed to Time: A60F0FF8 in 2026 -> 2026-02-28 15:58:52, delay 4062
    QTest::newRow("A60F0FF8 2026") << std::vector<u32> { 0xA60F0FF8 } << DateTime(2026, 2, 28, 15, 57, 0) << -1
                                   << std::vector<u32> { 0xA60F0FF8 } << std::vector<u32> { 2 } << 4062u
                                   << std::string("2026-02-28 15:58:52") << std::string("2026-02-28 15:59:51");

    // Next full minute: at 10:59:00 the 10:59 try is already gone
    QTest::newRow("Next full minute") << std::vector<u32> { 0x620A032C } << DateTime(2026, 10, 28, 10, 59, 0) << -1 << std::vector<u32> { }
                                      << std::vector<u32> { } << 0u << std::string() << std::string();

    // A seed whose hour has passed is skipped
    QTest::newRow("Hour passed") << std::vector<u32> { 0x661202B4 } << DateTime(2026, 10, 28, 19, 0, 0) << -1 << std::vector<u32> { }
                                 << std::vector<u32> { } << 0u << std::string() << std::string();

    // S > 118: F10A042A can't be hit at all on 28 Oct
    QTest::newRow("S over 118") << std::vector<u32> { 0xF10A042A } << DateTime(2026, 10, 28, 9, 0, 0) << -1 << std::vector<u32> { }
                                << std::vector<u32> { } << 0u << std::string() << std::string();
}

void SeedToTimeCalculator4Test::rankToday()
{
    QFETCH(std::vector<u32>, seeds);
    QFETCH(DateTime, now);
    QFETCH(int, parity);
    QFETCH(std::vector<u32>, order);
    QFETCH(std::vector<u32>, tries);
    QFETCH(u32, delay);
    QFETCH(std::string, first);
    QFETCH(std::string, last);

    auto ranked = SeedToTimeCalculator4::rankToday(seeds, now, parity);
    QCOMPARE(ranked.size(), order.size());
    for (size_t i = 0; i < ranked.size(); i++)
    {
        QCOMPARE(ranked[i].seed, order[i]);
        QCOMPARE(ranked[i].times.size(), tries[i]);
    }

    if (!ranked.empty())
    {
        const auto &best = ranked.front();
        QCOMPARE(best.times.front().getDelay(), delay);
        QCOMPARE(best.times.front().getDateTime().toString(), first);
        QCOMPARE(best.times.back().getDateTime().toString(), last);
    }
}
