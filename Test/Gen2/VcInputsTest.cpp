/*
 * PokeFinderPlus: new file. VcInputs tests (see VcInputsTest.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "VcInputsTest.hpp"
#include <Core/Gen2/CelebiSearch.hpp>
#include <Core/Gen2/VcInputs.hpp>
#include <QDirIterator>
#include <QFile>
#include <QTest>
#include <Test/Gen2/Gen2TestFiles.hpp>
#include <cstdlib>

using namespace Gen2;

namespace
{
    /// every *.cia under the folders of $POKEFINDERPLUS_GEN2_CIAS (several folders separated by ':')
    QStringList ciaFiles(const char *env)
    {
        QStringList out;
        for (const QString &d : QString::fromLocal8Bit(env).split(':', Qt::SkipEmptyParts))
        {
            QDirIterator it(d, { "*.cia" }, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext())
            {
                out << it.next();
            }
        }
        return out;
    }
}

void VcInputsTest::sha1()
{
    auto h = [](const std::string &s) { return sha1Hex(reinterpret_cast<const std::uint8_t *>(s.data()), s.size()); };
    QCOMPARE(h(""), std::string("da39a3ee5e6b4b0d3255bfef95601890afd80709"));
    QCOMPARE(h("abc"), std::string("a9993e364706816aba3e25717850c26c9cd0d89d"));
    QCOMPARE(h("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"), std::string("84983e441c3bd26ebaae4aa1f95129e5e54670f1"));
    QCOMPARE(h(std::string(1000, 'a')), std::string("291e9a6c66994949b57ba5e650361e98fc36b1ba"));
}

void VcInputsTest::patchFormat()
{
    // ROM-free: the VC patch format as the prototype's applypatch.py reads it (Mode 1 entries only; a<count>:<hex> or one byte)
    Bytes rom(0x100, 0);
    std::string p = "[Header]\nName=x\n[Fix 1]\nMode = 1 ; comment\nAddress=0x10\nFixcode=a3:AA BB CC DD\n"
                    "[Fix 2]\r\nMode=2\r\nAddress=0x20\r\nFixcode=11\r\n[Fix 3]\nMode=1\nAddress=30\nFixcode=0x7f\n";
    int n = 0;
    Bytes out = applyPatch(rom, p, &n);
    QCOMPARE(n, 2);
    QCOMPARE(out[0x10], std::uint8_t(0xaa));
    QCOMPARE(out[0x12], std::uint8_t(0xcc));
    QCOMPARE(out[0x13], std::uint8_t(0));
    QCOMPARE(out[0x20], std::uint8_t(0));
    QCOMPARE(out[0x30], std::uint8_t(0x7f));
}

void VcInputsTest::notACia()
{
    Bytes junk(4096, 0x5a);
    QCOMPARE(readCia(junk).error, CiaError::NotCia);
    Bytes cia(0x3000, 0);
    cia[0] = 0x20;
    cia[1] = 0x20; // CIA header size 0x2020, no visible NCCH: encrypted content
    CiaTitle t = readCia(cia);
    QCOMPARE(t.error, CiaError::Encrypted);
    QVERIFY(ciaErrorText(t.error).find("decrypted CIA") != std::string::npos);
}

void VcInputsTest::cias()
{
    const json &g = Gen2Test::golden("gen2_inputs.json");
    const char *dir = std::getenv("POKEFINDERPLUS_GEN2_CIAS");
    if (!dir || !*dir)
    {
        Gen2Test::countSkip("VcInputsTest::cias: POKEFINDERPLUS_GEN2_CIAS not set");
        QSKIP("needs the CIAs (POKEFINDERPLUS_GEN2_CIAS)");
    }
    std::set<std::string> seen, refused;
    for (const QString &path : ciaFiles(dir))
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QByteArray qb = f.readAll();
        Bytes b(qb.begin(), qb.end());
        CiaTitle t = readCia(b);
        if (t.error != CiaError::None)
        {
            QCOMPARE(t.error, CiaError::Encrypted);
            std::string head = Gen2Test::fnv64(b.data(), std::min<std::size_t>(b.size(), 65536));
            bool known = false;
            for (const auto &e : g["encrypted"])
            {
                known = known || e["head_fnv"].get<std::string>() == head;
            }
            QVERIFY2(known, "a refused CIA the prototype could read");
            refused.insert(head);
            continue;
        }
        char tid[17];
        std::snprintf(tid, sizeof tid, "%016llX", static_cast<unsigned long long>(t.titleId));
        QVERIFY2(g["titles"].contains(tid), tid);
        const auto &w = g["titles"][tid];
        QCOMPARE(t.codeSha1, w["code"].get<std::string>());
        QCOMPARE(t.codeDecSha1, w["code_dec"].get<std::string>());
        QCOMPARE(t.configSha1, w["config"].get<std::string>());
        QCOMPARE(t.romName, w["rom_name"].get<std::string>());
        QCOMPARE(t.sameEmulator(), w["same"].get<bool>());
        auto dumpB = t.dump();
        QVERIFY(dumpB);
        QCOMPARE(sha1Hex(*dumpB), w["rom_sha1"].get<std::string>());
        auto id = identifyDump(*dumpB);
        QVERIFY(id && id->first == w["lang"].get<std::string>()); // the ROM inside the title is the cartridge dump
        QCOMPARE(sha1Hex(t.romfs.at(t.romName.substr(4) + ".patch")), w["patch_sha1"].get<std::string>());
        auto patched = t.patchedRom();
        QVERIFY(patched);
        QCOMPARE(Gen2Test::fnv64(patched->data(), patched->size()), w["golden_rom"].get<std::string>());
        qInfo("CIA %s (%s): .code / config.ini %s; ROM = %s; ROM + patch = the goldens' VC ROM", tid, w["lang"].get<std::string>().c_str(),
              t.sameEmulator() ? "match the reference" : "DIFFERENT", id->second.c_str());
        seen.insert(tid);
    }
    QCOMPARE(seen.size(), g["titles"].size());
    QCOMPARE(refused.size(), g["encrypted"].size());
    qInfo("%zu decrypted CIAs read, %zu encrypted CIAs refused (\"%s\")", seen.size(), refused.size(),
          ciaErrorText(CiaError::Encrypted).substr(0, 60).c_str());
}

void VcInputsTest::dumpAlone()
{
    // a cartridge dump alone (no VC patch) gives the same Celebi as the VC ROM on 20 golden timelines per language
    const char *dir = std::getenv("POKEFINDERPLUS_GEN2_CIAS");
    auto victor = Gen2Test::namedSave("victor");
    auto enTest = Gen2Test::namedSave("en_test");
    if (!dir || !*dir || !victor || !enTest)
    {
        Gen2Test::countSkip("VcInputsTest::dumpAlone: needs the CIAs and the saves");
        QSKIP("needs the CIAs and the saves");
    }
    std::map<std::string, RomPtr> dumps;
    for (const QString &path : ciaFiles(dir))
    {
        QFile f(path);
        f.open(QIODevice::ReadOnly);
        QByteArray qb = f.readAll();
        CiaTitle t = readCia(Bytes(qb.begin(), qb.end()));
        if (t.error == CiaError::None)
        {
            auto d = t.dump();
            dumps[identifyDump(*d)->first] = std::make_shared<Bytes>(*d);
        }
    }
    const json &core = Gen2Test::golden("gen2_core.json");
    std::map<std::string, int> per;
    int n = 0, bad = 0;
    auto c = std::make_unique<VcCore>();
    for (const auto &cs : core["cases"])
    {
        std::string lang = cs["lang"].get<std::string>();
        if (per[lang]++ >= 20 || !dumps.count(lang))
        {
            continue;
        }
        Bytes j = Gen2Test::joyFromRle(cs["joy"], cs["n"].get<int>());
        boot(*c, dumps[lang], cs["save"] == "victor" ? *victor : *enTest, Gen2Test::rtcFromJson(cs["rtc"]));
        RunResult r = run(*c, j);
        bad += !(r.ret > 0 && Gen2::hex(*r.enemy) == cs["enemy"].get<std::string>());
        n++;
    }
    qInfo("cartridge dump alone: %d/%d timelines give the VC ROM's Celebi", n - bad, n);
    QCOMPARE(n, 100);
    QCOMPARE(bad, 0);
}
