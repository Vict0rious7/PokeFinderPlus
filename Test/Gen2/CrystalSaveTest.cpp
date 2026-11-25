/*
 * PokeFinderPlus: new file. CrystalSave tests (see CrystalSaveTest.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CrystalSaveTest.hpp"
#include <Core/Gen2/CrystalSave.hpp>
#include <QTest>
#include <Test/Gen2/Gen2TestFiles.hpp>

using namespace Gen2;

void CrystalSaveTest::checksumsAndCopies()
{
    // ROM-free, no save needed: writes land in both copies; the checksums are the 16-bit byte sums of 0xb7a bytes.
    std::vector<std::uint8_t> blank(CrystalSave::SIZE);
    for (std::size_t i = 0; i < blank.size(); i++)
    {
        blank[i] = static_cast<std::uint8_t>(i * 7);
    }
    CrystalSave s(blank);
    s.wr8(Sym::wGameTimeFrames, 42);
    QCOMPARE(s.raw()[CrystalSave::offset(Sym::wGameTimeFrames)], std::uint8_t(42));
    QCOMPARE(s.raw()[CrystalSave::offset(Sym::wGameTimeFrames, true)], std::uint8_t(42));
    QCOMPARE(CrystalSave::offset(0xd47b), 0x2009);
    QCOMPARE(CrystalSave::offset(0xdcd7, true), 0x1a65);
    bool threw = false;
    try
    {
        CrystalSave::offset(0xc000);
    }
    catch (const std::out_of_range &)
    {
        threw = true;
    }
    QVERIFY(threw);
    const auto &b = s.bytes();
    unsigned sum = 0;
    for (int i = 0x2009; i < 0x2009 + 0xb7a; i++)
    {
        sum += b[i];
    }
    QCOMPARE(b[0x2d0d] | (b[0x2d0e] << 8), static_cast<int>(sum & 0xffff));
    Minimal m;
    m.textSpeed = TextSpeed::Slow;
    m.stereo = true;
    m.frame = 8;
    m.raikou = true;
    m.F = 59;
    m.playSec = 1;
    m.start = { 3, 21, 4, 59 };
    m.kenji = 1;
    m.lass = true;
    CrystalSave t = Gen2::applyMinimal(s, m);
    Minimal r = readMinimal(t);
    QCOMPARE(r.textSpeed, TextSpeed::Slow);
    QCOMPARE(r.F, 59);
    QCOMPARE(r.kenji, 1);
    QCOMPARE(r.start[1], 21);
    QVERIFY(r.raikou);
}

void CrystalSaveTest::readTemplates()
{
    const json &g = Gen2Test::golden("gen2_save.json");
    QVERIFY(!g.is_null());
    int skipped = 0;
    for (const auto &[name, t] : g["templates"].items())
    {
        auto sav = Gen2Test::save(t["fnv"].get<std::string>());
        if (!sav)
        {
            skipped++;
            continue;
        }
        Minimal m = readMinimal(CrystalSave(*sav));
        QCOMPARE(Gen2Test::minimalToJson(m).dump(), t["minimal"].dump());
    }
    if (skipped)
    {
        Gen2Test::countSkip("CrystalSaveTest::readTemplates: " + std::to_string(skipped) + " template save(s) missing");
        QSKIP("needs the template saves (POKEFINDERPLUS_GEN2_SAVES)");
    }
}

void CrystalSaveTest::applyMinimal()
{
    const json &g = Gen2Test::golden("gen2_save.json");
    std::map<std::string, Gen2Test::Bytes> tpl;
    for (const auto &[name, t] : g["templates"].items())
    {
        auto sav = Gen2Test::save(t["fnv"].get<std::string>());
        if (!sav)
        {
            Gen2Test::countSkip("CrystalSaveTest::applyMinimal: no template save '" + name + "'");
            QSKIP("needs the template saves (POKEFINDERPLUS_GEN2_SAVES)");
        }
        tpl[name] = *sav;
    }
    int n = 0, bad = 0;
    for (const auto &c : g["cases"])
    {
        CrystalSave s = Gen2::applyMinimal(CrystalSave(tpl[c["template"].get<std::string>()]), Gen2Test::minimalFromJson(c["minimal"]));
        const auto &b = s.bytes();
        bool ok = Gen2Test::fnv64(b.data(), b.size()) == c["fnv"].get<std::string>();
        ok = ok && Gen2Test::minimalToJson(readMinimal(s)).dump() == c["readback"].dump();
        bad += !ok;
        n++;
    }
    qInfo("apply_minimal: %d/%d saves byte-identical to the prototype", n - bad, n);
    QCOMPARE(bad, 0);
}
