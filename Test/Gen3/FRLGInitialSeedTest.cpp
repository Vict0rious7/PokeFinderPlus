/*
 * This file is part of PokeFinderPlus, an unofficial fork of PokéFinder
 * Copyright (C) 2026 by Vict0rious7
 *
 * Tests for the port of Ten Lines' FRLG initial seed lookup (Ten Lines by Lincoln-LM, GPL-3.0).
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
 */

#include "FRLGInitialSeedTest.hpp"
#include <Core/Gen3/Tools/FRLGInitialSeed.hpp>
#include <QTest>

// Expected values: an independent Python re-implementation of Ten Lines' ten_lines_frlg over the same tables
// (pulled 2026-11-02). C8938938 -> 16B3 at 14,071 advances is the FR/LG Perfect IV Static Manip guide's example.
void FRLGInitialSeedTest::find_data()
{
    QTest::addColumn<u32>("target");
    QTest::addColumn<std::string>("version");
    QTest::addColumn<FRLGSettingsFilter>("filter");
    QTest::addColumn<u32>("ttvOut");
    QTest::addColumn<u16>("initialSeed");
    QTest::addColumn<u32>("advances");
    QTest::addColumn<int>("gbaMS");
    QTest::addColumn<std::string>("settings");

    QTest::newRow("Guide example, ENG Mono/L=A/A") << 0xC8938938u << std::string("fr") << FRLGSettingsFilter { "mono", "a", "a", "none" }
                                                   << 0u << u16(0x16B3) << 14071u << 36423 << std::string("mono_a_a_none");
    QTest::newRow("Guide example, EU any") << 0xC8938938u << std::string("fr_eu") << FRLGSettingsFilter { } << 0u << u16(0x16B3) << 14071u
                                           << 44962 << std::string("mono_a_a_startup_select");
    QTest::newRow("EU any, Teachy TV 3600") << 0xC8938938u << std::string("fr_eu") << FRLGSettingsFilter { } << 3600u << u16(0x16B3)
                                            << 14071u << 44962 << std::string("mono_a_a_startup_select");
    QTest::newRow("EU Mono/Start") << 0xDC8AFFCAu << std::string("fr_eu") << FRLGSettingsFilter { "mono", "", "start", "" } << 0u
                                   << u16(0x8D84) << 76734u << 47506 << std::string("mono_h_start_startup_select");
    QTest::newRow("LG ENG any") << 0xC8938938u << std::string("lg") << FRLGSettingsFilter { } << 0u << u16(0x16B3) << 14071u << 44945
                                << std::string("stereo_a_l_blackout_r");
    QTest::newRow("mGBA") << 0xC8938938u << std::string("fr_mgba") << FRLGSettingsFilter { } << 0u << u16(0x7E02) << 147550u << 45464
                          << std::string("mono_a_a_none");
}

void FRLGInitialSeedTest::find()
{
    QFETCH(u32, target);
    QFETCH(std::string, version);
    QFETCH(FRLGSettingsFilter, filter);
    QFETCH(u32, ttvOut);
    QFETCH(u16, initialSeed);
    QFETCH(u32, advances);
    QFETCH(int, gbaMS);
    QFETCH(std::string, settings);

    QVERIFY(FRLGInitialSeed::hasData(version));
    auto results = FRLGInitialSeed::find(target, 1, 0, version, ttvOut, filter);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].initialSeed, initialSeed);
    QCOMPARE(results[0].advances, advances);
    QCOMPARE(FRLGInitialSeed::frameToMS(results[0].seedTime / 16.0, "GBA"), gbaMS);
    QCOMPARE(results[0].settings, settings);
}

void FRLGInitialSeedTest::filterExcludesEverything()
{
    // Switch sheets have no L seed button: the lookup must stop instead of looping
    auto results = FRLGInitialSeed::find(0xC8938938, 1, 0, "fr_nx", 0, { "", "", "l", "" });
    QVERIFY(results.empty());
}

void FRLGInitialSeedTest::teachyTV()
{
    u32 ttv, regular;
    FRLGInitialSeed::teachyTVConversion(14071, 3600, ttv, regular);
    QCOMPARE(ttv, 33u);
    QCOMPARE(regular, 3742u); // final A press frame = 33 + 3742 = 3775
    QCOMPARE(FRLGInitialSeed::frameToMS(2031, "GBA"), 33744);
    QCOMPARE(FRLGInitialSeed::frameToMS(2031, "NDS"), 34833);
}

void FRLGInitialSeedTest::humanize()
{
    QCOMPARE(FRLGInitialSeed::humanizeSettings("mono_a_start_startup_select"),
             std::string("Mono | L=A | Seed Button: Start | Extra Button: Startup Select"));
    QCOMPARE(FRLGInitialSeed::humanizeSettings("stereo_h_a_none"), std::string("Stereo | Help | Seed Button: A | Extra Button: None "));
}

// Ten Lines' calibration list (get_contiguous_seed_list), expected values from the Python reader of the same table
void FRLGInitialSeedTest::neighbourSeeds()
{
    auto none = FRLGInitialSeed::neighbourSeeds("fr", "mono_a_a_none", 0x16B3, 2);
    std::vector<std::pair<u16, u32>> expected
        = { { 0xAB93, 35024 }, { 0xC915, 35040 }, { 0x16B3, 35056 }, { 0x6071, 35072 }, { 0xA8CE, 35088 } };
    QCOMPARE(none.size(), expected.size());
    for (size_t i = 0; i < none.size(); i++)
    {
        QCOMPARE(none[i].initialSeed, expected[i].first);
        QCOMPARE(none[i].seedTime, expected[i].second);
    }

    // Startup Select shifts every seed by Ten Lines' offset (-1 in L=A mode)
    auto select = FRLGInitialSeed::neighbourSeeds("fr", "mono_a_a_startup_select", 0x16B2, 2);
    QCOMPARE(select.size(), 5);
    QCOMPARE(select[0].initialSeed, u16(0xAB92));
    QCOMPARE(select[2].seedTime, 35056u);

    // Start of the list: fewer seeds before; unknown seed or settings: empty
    QCOMPARE(FRLGInitialSeed::neighbourSeeds("fr", "mono_a_a_none", 0x11AC, 2).size(), 3);
    QVERIFY(FRLGInitialSeed::neighbourSeeds("fr", "mono_a_a_none", 0x16B2, 2).empty());
    QVERIFY(FRLGInitialSeed::neighbourSeeds("fr", "mono_a", 0x16B3, 2).empty());
}
