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
 * PokeFinderPlus: new file. Checks the Gen 6 static generator against 3DSRNGTool 1.0.6's own output
 * (Test/Gen6/static6_3dsrngtool.json, made by running 3DSRNGTool's Stationary6 code; see the golden3ds tool).
 */

#include "StaticGenerator6Test.hpp"
#include <Core/Enum/Game.hpp>
#include <Core/Enum/Lead.hpp>
#include <Core/Gen6/Encounters6.hpp>
#include <Core/Gen6/Generators/StaticGenerator6.hpp>
#include <Core/Gen6/States/State6.hpp>
#include <Core/Gen6/StaticTemplate6.hpp>
#include <QFile>
#include <QTest>
#include <Test/Data.hpp>

namespace
{
    Game toGame(const std::string &game)
    {
        if (game == "X")
        {
            return Game::X;
        }
        if (game == "Y")
        {
            return Game::Y;
        }
        if (game == "OR")
        {
            return Game::OR;
        }
        return Game::AS;
    }

    json golden()
    {
        QFile file(":/data/static6_3dsrngtool.json");
        if (!file.open(QIODevice::ReadOnly))
        {
            return json();
        }
        return json::parse(file.readAll().toStdString());
    }
}

void StaticGenerator6Test::generate_data()
{
    QTest::addColumn<std::string>("data");

    int i = 0;
    json data = golden();
    for (const auto &c : data["cases"])
    {
        std::string name = c["game"].get<std::string>() + " " + c["group"].get<std::string>() + " #"
            + std::to_string(c["pokemon"].get<int>()) + " species " + std::to_string(c["species"].get<int>()) + " case "
            + std::to_string(i++);
        QTest::newRow(name.data()) << c.dump();
    }
}

void StaticGenerator6Test::generate()
{
    QFETCH(std::string, data);
    json c = json::parse(data);

    Game version = toGame(c["game"].get<std::string>());
    std::vector<std::string> categories = Encounters6::getStaticCategories(version);
    auto it = std::find(categories.begin(), categories.end(), c["group"].get<std::string>());
    QVERIFY(it != categories.end());
    const StaticTemplate6 *staticTemplate = Encounters6::getStaticEncounter(version, it - categories.begin(), c["pokemon"].get<int>());
    QVERIFY(staticTemplate != nullptr);
    QCOMPARE(staticTemplate->getSpecie(), c["species"].get<u16>());
    QCOMPARE(staticTemplate->getLevel(), c["level"].get<u8>());

    // 3DSRNGTool takes TSV and TRV directly: TID = TSV << 4 | TRV, SID = 0 gives the same values
    u16 tid = (c["tsv"].get<u16>() << 4) | c["trv"].get<u16>();
    Profile6 profile("-", version, tid, 0, c["shinyCharm"].get<bool>());
    QCOMPARE(profile.getTSV(), c["tsv"].get<u16>());

    u8 syncNature = c["syncNature"].get<u8>();
    Lead lead = syncNature < 25 ? static_cast<Lead>(syncNature) : Lead::None;
    u32 offset = c["considerDelay"].get<bool>() ? staticTemplate->getDelay() : 0;
    u32 min = c["min"].get<u32>();
    u32 max = c["max"].get<u32>();

    std::array<u8, 6> ivMin;
    ivMin.fill(0);
    std::array<u8, 6> ivMax;
    ivMax.fill(31);
    std::array<bool, 25> natures;
    natures.fill(true);
    std::array<bool, 16> powers;
    powers.fill(true);
    StateFilter filter(255, 255, 255, 1, 100, 0, 255, 0, 255, true, ivMin, ivMax, natures, powers);

    StaticGenerator6 generator(min, max - min, offset, lead, c["assumeSynced"].get<bool>(), *staticTemplate, profile, filter);
    std::vector<State6> states = generator.generate(c["seed"].get<u32>());

    const auto &results = c["results"];
    QCOMPARE(states.size(), results.size());
    for (size_t i = 0; i < states.size(); i++)
    {
        const State6 &s = states[i];
        const auto &r = results[i];
        // [frame, EC, PID, iv0..iv5, ability (1/2/3), nature, gender (1 male, 2 female, 0 none), shiny (0/1/2), synchronized]
        QCOMPARE(s.getAdvances(), r[0].get<u32>());
        QCOMPARE(s.getEC(), r[1].get<u32>());
        QCOMPARE(s.getPID(), r[2].get<u32>());
        for (int iv = 0; iv < 6; iv++)
        {
            QCOMPARE(s.getIV(iv), r[3 + iv].get<u8>());
        }
        QCOMPARE(s.getAbility() + 1, r[9].get<int>());
        QCOMPARE(s.getNature(), r[10].get<u8>());
        u8 gender = r[11].get<u8>();
        QCOMPARE(s.getGender(), static_cast<u8>(gender == 1 ? 0 : gender == 2 ? 1 : 2));
        QCOMPARE(s.getShiny(), r[12].get<u8>());
        QCOMPARE(s.getSync(), r[13].get<int>() == 1);
    }
}

void StaticGenerator6Test::categories()
{
    QCOMPARE(Encounters6::getStaticCategories(Game::X).size(), 8u);
    QCOMPARE(Encounters6::getStaticCategories(Game::OR).size(), 9u);
    QVERIFY(Encounters6::getStaticEncounter(Game::X, 0, 999) == nullptr);
    QVERIFY(Encounters6::getStaticEncounters(Game::X, 99) == nullptr);
}
