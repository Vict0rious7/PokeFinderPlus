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
#include <cctype>
#include <cstdlib>
#include <set>

using namespace Gen2;

namespace
{
    /// every *.cia under the folders of $POKEFINDERPLUS_GEN2_CIAS (several folders separated by ':', or ';' on Windows)
    QStringList ciaFiles(const char *env)
    {
        QStringList out;
        for (const QString &d : QString::fromLocal8Bit(env).split(QDir::listSeparator(), Qt::SkipEmptyParts))
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

void VcInputsTest::knownRoms()
{
    // ROM-free: the hash table (Data/ROMS, 2026-11-25) and its matching; JP is not in it
    std::set<std::string> hashes, labels;
    for (const auto &k : KNOWN_ROMS)
    {
        std::string h = k.sha1;
        QCOMPARE(h.size(), std::size_t(40));
        QVERIFY(h.find_first_not_of("0123456789abcdef") == std::string::npos);
        hashes.insert(h);
        labels.insert(k.label());
        QCOMPARE(knownRomBySha1(h), &k);
        std::string upper = h;
        for (char &c : upper)
        {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        QCOMPARE(knownRomBySha1(upper), &k);
        std::string off = h;
        off[39] = off[39] == '0' ? '1' : '0';
        QVERIFY(knownRomBySha1(off) == nullptr);
    }
    QCOMPARE(hashes.size(), std::size_t(6));
    QVERIFY(labels == (std::set<std::string> { "French", "German", "Italian", "Spanish", "English Rev 0", "English Rev A" }));
    QVERIFY(knownRomBySha1("95127b901bbce2407daf43cce9f45d4c27ef635d") == nullptr); // Japanese: not supported
    QVERIFY(knownRomBySha1("") == nullptr);
    QVERIFY(identifyRom(Bytes(0x200000, 0)) == nullptr);
    QCOMPARE(languageName("de"), std::string("German"));
    QCOMPARE(std::string(knownRomBySha1("f4cd194bdee0d04ca4eac29e09b8e4e9d818c133")->lang), std::string("en"));
}

void VcInputsTest::romCheckMarks()
{
    // every supported ROM in $POKEFINDERPLUS_GBC_ROMS gets its check mark; one byte changed, no check mark
    std::map<std::string, std::string> found; // label -> sha1
    for (const auto &[fnv, b] : Gen2Test::romFiles())
    {
        const KnownRom *k = identifyRom(*b);
        if (!k)
        {
            continue;
        }
        found[k->label()] = k->sha1;
        for (std::size_t at : { std::size_t(0), b->size() / 2, b->size() - 1 })
        {
            Bytes changed = *b;
            changed[at] ^= 0x01;
            QVERIFY2(identifyRom(changed) == nullptr, k->label().c_str());
        }
    }
    if (found.empty())
    {
        Gen2Test::countSkip("VcInputsTest::romCheckMarks: no Crystal ROM in POKEFINDERPLUS_GBC_ROMS");
        QSKIP("needs the Crystal ROMs (POKEFINDERPLUS_GBC_ROMS)");
    }
    QStringList l;
    for (const auto &[label, h] : found)
    {
        l << QString::fromStdString(label);
    }
    qInfo("check marks: %s (each with one byte changed: none)", qPrintable(l.join(", ")));
    QCOMPARE(found.size(), std::size_t(6));
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
        auto rom = t.rom();
        QVERIFY(rom);
        QCOMPARE(sha1Hex(*rom), w["rom_sha1"].get<std::string>());
        const KnownRom *k = identifyRom(*rom); // the ROM inside the title gets the same check mark as a .gbc
        QVERIFY(k && k->lang == w["lang"].get<std::string>());
        QCOMPARE(t.romfs.size(), std::size_t(1)); // only the ROM is kept (no patch file)
        qInfo("CIA %s (%s): .code / config.ini %s; ROM inside: check mark %s", tid, w["lang"].get<std::string>().c_str(),
              t.sameEmulator() ? "match the reference" : "DIFFERENT", k->label().c_str());
        seen.insert(tid);
    }
    QCOMPARE(seen.size(), g["titles"].size());
    QCOMPARE(refused.size(), g["encrypted"].size());
    qInfo("%zu decrypted CIAs read, %zu encrypted CIAs refused (\"%s\")", seen.size(), refused.size(),
          ciaErrorText(CiaError::Encrypted).substr(0, 60).c_str());
}

void VcInputsTest::romsGiveVcCelebi()
{
    // each supported ROM alone (no VC patch) gives the VC ROM's Celebi: 20 golden timelines per ROM, and all 400 EN timelines
    // for EN Rev 0 (the VC title ships Rev A, so Rev 0 never ran in the core before)
    auto victor = Gen2Test::namedSave("victor");
    auto enTest = Gen2Test::namedSave("en_test");
    std::map<std::string, RomPtr> roms; // label -> ROM
    std::map<std::string, std::string> langOf;
    for (const auto &[fnv, b] : Gen2Test::romFiles())
    {
        if (const KnownRom *k = identifyRom(*b))
        {
            roms[k->label()] = b;
            langOf[k->label()] = k->lang;
        }
    }
    if (roms.empty() || !victor || !enTest)
    {
        Gen2Test::countSkip("VcInputsTest::romsGiveVcCelebi: needs the Crystal ROMs and the saves");
        QSKIP("needs the Crystal ROMs (POKEFINDERPLUS_GBC_ROMS) and the saves");
    }
    const json &core = Gen2Test::golden("gen2_core.json");
    auto c = std::make_unique<VcCore>();
    int total = 0, totalBad = 0;
    for (const auto &[label, rom] : roms)
    {
        int n = 0, bad = 0, cap = label == "English Rev 0" ? 1000 : 20;
        for (const auto &cs : core["cases"])
        {
            if (cs["lang"].get<std::string>() != langOf[label] || n >= cap)
            {
                continue;
            }
            Bytes j = Gen2Test::joyFromRle(cs["joy"], cs["n"].get<int>());
            boot(*c, rom, cs["save"] == "victor" ? *victor : *enTest, Gen2Test::rtcFromJson(cs["rtc"]));
            RunResult r = run(*c, j);
            bool same = cs["enemy"].is_null() ? r.ret <= 0 : (r.ret > 0 && Gen2::hex(*r.enemy) == cs["enemy"].get<std::string>());
            bad += !same;
            n++;
        }
        qInfo("%s alone: %d/%d timelines give the VC ROM's Celebi", label.c_str(), n - bad, n);
        QVERIFY2(n > 0, label.c_str());
        total += n;
        totalBad += bad;
    }
    QCOMPARE(totalBad, 0);
    QCOMPARE(roms.size(), std::size_t(6));
    QVERIFY(total >= 5 * 20 + 400);
}
