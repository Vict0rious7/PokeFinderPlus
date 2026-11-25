/*
 * PokeFinderPlus: new file. VcCore tests (see VcCoreTest.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "VcCoreTest.hpp"
#include <Core/Gen2/VcCore.hpp>
#include <QTest>
#include <Test/Gen2/Gen2TestFiles.hpp>
#include <cstdio>
#include <thread>

using Gen2::VcCore;

namespace
{
    std::string hex(const std::uint8_t *p, int n)
    {
        std::string s;
        char b[3];
        for (int i = 0; i < n; i++)
        {
            std::snprintf(b, sizeof b, "%02x", p[i]);
            s += b;
        }
        return s;
    }

    std::string hex64(std::uint64_t v)
    {
        char b[17];
        std::snprintf(b, sizeof b, "%016llx", static_cast<unsigned long long>(v));
        return b;
    }

    struct Outcome
    {
        std::string enemy, hash;
        int ret;
        long long frames;
        bool operator==(const Outcome &) const = default;
    };

    /// Runs one golden case on core c (ROM and save given). Same steps as g2lib.boot + g2lib.run.
    Outcome runCase(VcCore &c, const json &cs, const json &g, const std::shared_ptr<const Gen2Test::Bytes> &rom,
                    const Gen2Test::Bytes &sram)
    {
        auto j = Gen2Test::joyFromRle(cs["joy"], cs["n"].get<int>());
        const auto &r = cs["rtc"];
        c.loadRom(rom);
        c.loadSram(sram);
        c.reset(g["divcd0"].get<int>(), g["counter0"].get<int>());
        c.setRtc(r[0].get<int>(), r[1].get<int>(), r[2].get<int>(), r[3].get<int>(), r[4].get<int>());
        std::uint8_t o[8] = {};
        int ret = c.runPlan(j, 4000, 0xfb, o);
        return { ret > 0 ? hex(o, 8) : std::string(), hex64(c.stateHash()), ret, c.frames() };
    }

    Outcome expected(const json &cs)
    {
        return { cs["enemy"].is_null() ? std::string() : cs["enemy"].get<std::string>(), cs["hash"].get<std::string>(),
                 cs["ret"].get<int>(), cs["frames"].get<long long>() };
    }

    /// ROMs and saves the goldens need; false (and a recorded skip) when any is missing.
    bool files(const json &g, std::map<std::string, std::shared_ptr<const Gen2Test::Bytes>> &roms,
               std::map<std::string, Gen2Test::Bytes> &saves, const char *test)
    {
        for (const auto &[lang, h] : g["roms"].items())
        {
            auto r = Gen2Test::rom(h.get<std::string>());
            if (!r)
            {
                Gen2Test::countSkip(std::string("VcCoreTest::") + test + ": no " + lang + " ROM");
                return false;
            }
            roms[lang] = r;
        }
        for (const auto &[name, h] : g["saves"].items())
        {
            auto s = Gen2Test::save(h.get<std::string>());
            if (!s)
            {
                Gen2Test::countSkip(std::string("VcCoreTest::") + test + ": no save '" + name + "'");
                return false;
            }
            saves[name] = *s;
        }
        return true;
    }
}

void VcCoreTest::goldenLoaded()
{
    const json &g = Gen2Test::golden("gen2_core.json");
    QVERIFY(!g.is_null());
    QCOMPARE(g["cases"].size(), 1200u);
    QCOMPARE(g["r8_13"]["enemy"].get<std::string>(), std::string("fb00d7dbf6f82aaa"));
}

void VcCoreTest::snapshotRestore()
{
    // ROM-free: a tiny program (LD A,n / INC A / JR -3) runs the same after a restore. Checks that the snapshot is a full
    // copy of the state and that two cores do not share state.
    auto rom = std::make_shared<Gen2Test::Bytes>(0x8000, 0);
    (*rom)[0x100] = 0x3e; // LD A,0x00
    (*rom)[0x101] = 0x00;
    (*rom)[0x102] = 0x3c; // INC A
    (*rom)[0x103] = 0x18; // JR -3
    (*rom)[0x104] = 0xfd;
    auto a = std::make_unique<VcCore>();
    auto b = std::make_unique<VcCore>();
    a->loadRom(rom);
    b->loadRom(rom);
    a->reset();
    b->reset();
    for (int i = 0; i < 3; i++)
    {
        a->frame();
    }
    auto snap = std::make_unique<VcCore::State>(a->snapshot());
    std::uint64_t h0 = a->stateHash();
    for (int i = 0; i < 5; i++)
    {
        a->frame();
    }
    std::uint64_t h1 = a->stateHash();
    QVERIFY(h0 != h1);
    a->restore(*snap);
    QCOMPARE(a->stateHash(), h0);
    for (int i = 0; i < 5; i++)
    {
        a->frame();
    }
    QCOMPARE(a->stateHash(), h1);
    for (int i = 0; i < 8; i++)
    {
        b->frame();
    }
    QCOMPARE(b->stateHash(), h1);
    QCOMPARE(sizeof(VcCore::State) > 80000, true);
}

void VcCoreTest::timelines()
{
    const json &g = Gen2Test::golden("gen2_core.json");
    std::map<std::string, std::shared_ptr<const Gen2Test::Bytes>> roms;
    std::map<std::string, Gen2Test::Bytes> saves;
    if (!files(g, roms, saves, "timelines"))
    {
        QSKIP("needs the Crystal ROMs and saves (POKEFINDERPLUS_GBC_ROMS, POKEFINDERPLUS_GEN2_SAVES)");
    }
    auto c = std::make_unique<VcCore>();
    int bad = 0, n = 0;
    for (const auto &cs : g["cases"])
    {
        Outcome got = runCase(*c, cs, g, roms[cs["lang"]], saves[cs["save"]]);
        Outcome want = expected(cs);
        n++;
        if (!(got == want) && bad++ < 5)
        {
            qWarning("case %d (%s, %s): enemy %s / %s, hash %s / %s, ret %d / %d", n - 1, cs["lang"].get<std::string>().c_str(),
                     cs["save"].get<std::string>().c_str(), got.enemy.c_str(), want.enemy.c_str(), got.hash.c_str(),
                     want.hash.c_str(), got.ret, want.ret);
        }
    }
    qInfo("VcCore timelines: %d/%d identical to the prototype", n - bad, n);
    QCOMPARE(bad, 0);
}

void VcCoreTest::r8_13()
{
    const json &g = Gen2Test::golden("gen2_core.json");
    const json &r = g["r8_13"];
    auto rom = Gen2Test::rom(g["roms"]["fr"].get<std::string>());
    auto sav = Gen2Test::save(g["saves"]["victor"].get<std::string>());
    if (!rom || !sav)
    {
        Gen2Test::countSkip("VcCoreTest::r8_13: no FR ROM or Victor's save");
        QSKIP("needs the FR ROM and Victor's save");
    }
    auto c = std::make_unique<VcCore>();
    c->loadRom(rom);
    c->loadSram(*sav);
    c->reset(g["divcd0"].get<int>(), g["counter0"].get<int>());
    const auto &ir = r["init_rtc"];
    c->setRtc(ir[0].get<int>(), ir[1].get<int>(), ir[2].get<int>(), ir[3].get<int>(), ir[4].get<int>());
    for (const auto &f : r["frames"])
    {
        if (!f[1].is_null())
        {
            const auto &t = f[1];
            c->setRtc(t[0].get<int>(), t[1].get<int>(), t[2].get<int>(), t[3].get<int>(), t[4].get<int>());
        }
        c->setJoy(f[0].get<std::uint8_t>());
        QCOMPARE(c->frame(), 0);
    }
    std::uint8_t e[8];
    for (int i = 0; i < 8; i++)
    {
        e[i] = c->wram(1, 0x206 + i);
    }
    QCOMPARE(hex(e, 8), std::string("fb00d7dbf6f82aaa"));
    QCOMPARE(hex64(c->stateHash()), r["hash"].get<std::string>());
}

void VcCoreTest::threads()
{
    const json &g = Gen2Test::golden("gen2_core.json");
    std::map<std::string, std::shared_ptr<const Gen2Test::Bytes>> roms;
    std::map<std::string, Gen2Test::Bytes> saves;
    if (!files(g, roms, saves, "threads"))
    {
        QSKIP("needs the Crystal ROMs and saves (POKEFINDERPLUS_GBC_ROMS, POKEFINDERPLUS_GEN2_SAVES)");
    }
    // 2 threads, each with its own core, on interleaved halves of 200 cases (different languages run at the same time)
    const auto &cases = g["cases"];
    std::vector<int> idx;
    for (int i = 0; i < static_cast<int>(cases.size()); i += 6)
    {
        idx.push_back(i);
    }
    std::vector<Outcome> got(idx.size());
    auto work = [&](int part) {
        auto c = std::make_unique<VcCore>();
        for (std::size_t k = part; k < idx.size(); k += 2)
        {
            const auto &cs = cases[idx[k]];
            got[k] = runCase(*c, cs, g, roms[cs["lang"]], saves[cs["save"]]);
        }
    };
    std::thread t0(work, 0), t1(work, 1);
    t0.join();
    t1.join();
    int bad = 0;
    for (std::size_t k = 0; k < idx.size(); k++)
    {
        bad += !(got[k] == expected(cases[idx[k]]));
    }
    qInfo("VcCore 2 threads: %d/%zu identical", static_cast<int>(idx.size()) - bad, idx.size());
    QCOMPARE(bad, 0);
}
