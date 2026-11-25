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
 * PokeFinderPlus: new file. See CelebiTool.hpp.
 */

#include "CelebiTool.hpp"
#include <Core/Gen2/VcInputs.hpp>
#include <Core/Resources/Gen2Template.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>

using json = nlohmann::json;

namespace Gen2
{
    namespace
    {
        const char *speedName(TextSpeed t)
        {
            switch (t)
            {
            case TextSpeed::Instant:
                return "instant";
            case TextSpeed::Mid:
                return "mid";
            case TextSpeed::Slow:
                return "slow";
            default:
                return "fast";
            }
        }

        TextSpeed speedOf(const std::string &s)
        {
            return s == "instant" ? TextSpeed::Instant : s == "mid" ? TextSpeed::Mid : s == "slow" ? TextSpeed::Slow : TextSpeed::Fast;
        }

        /// Runs fn(i) for the jobs not yet in the cache, in parallel; false when cancelled.
        bool fill(int n, int threads, const std::function<bool(int)> &cached, const std::function<void(int)> &fn, const Progress &progress,
                  const Cancel *cancel)
        {
            std::vector<int> todo;
            for (int i = 0; i < n; i++)
            {
                if (!cached(i))
                {
                    todo.push_back(i);
                }
            }
            int done0 = n - static_cast<int>(todo.size());
            std::atomic<int> count { 0 };
            if (progress)
            {
                progress(done0, n);
            }
            parallelFor(static_cast<int>(todo.size()), threads, [&](int t) {
                if (cancel && cancel->load())
                {
                    return;
                }
                fn(todo[t]);
                int c = ++count;
                if (progress)
                {
                    progress(done0 + c, n);
                }
            });
            return !(cancel && cancel->load()) || count == static_cast<int>(todo.size());
        }

        std::string fgridKey(int plan, int F, int X)
        {
            char key[32];
            if (X == 0)
            {
                std::snprintf(key, sizeof key, "f_%d_%02d", plan, F);
            }
            else
            {
                std::snprintf(key, sizeof key, "f_%d_%02d_x%+d", plan, F, X);
            }
            return key;
        }
    }

    // ------------------------------------------------------------------ inputs

    json toJson(const Visible &v)
    {
        return { { "textspeed", speedName(v.textSpeed) }, { "sound", v.stereo ? "stereo" : "mono" }, { "frame", v.frame },
                 { "raikou", v.raikou },
                 { "entei", v.entei },
                 { "lass", v.lass },
                 { "swarm", v.swarm } };
    }

    Visible visibleFromJson(const json &j)
    {
        Visible v;
        v.textSpeed = speedOf(j.value("textspeed", std::string("fast")));
        v.stereo = j.value("sound", std::string("mono")) == "stereo";
        v.frame = std::clamp(j.value("frame", 1), 1, 8);
        v.raikou = j.value("raikou", false);
        v.entei = j.value("entei", false);
        v.lass = j.value("lass", true);
        v.swarm = j.value("swarm", false);
        return v;
    }

    const Bytes &templateSave()
    {
        static const Bytes b(GEN2_CRYSTAL_TEMPLATE.begin(), GEN2_CRYSTAL_TEMPLATE.end());
        return b;
    }

    Bytes toolSave(const Visible &v, int kenji, std::optional<std::array<int, 3>> offset)
    {
        CrystalSave tpl(templateSave());
        Minimal m = readMinimal(tpl);
        m.textSpeed = v.textSpeed;
        m.stereo = v.stereo;
        m.frame = v.frame;
        m.raikou = v.raikou;
        m.entei = v.entei;
        m.lass = v.lass;
        m.swarm = v.swarm;
        m.kenji = kenji;
        m.setKenji = true;
        if (offset)
        {
            m.start = { m.start[0], (*offset)[0], (*offset)[1], (*offset)[2] };
        }
        return applyMinimal(tpl, m).bytes();
    }

    namespace
    {
        std::pair<std::optional<GameFiles>, std::string> fromDump(const Bytes &dump, RomPtr rom, const std::string &kind)
        {
            auto id = identifyDump(dump);
            if (!id)
            {
                return { std::nullopt, "This is not a Pokémon Crystal dump the tool knows (FR, DE, IT, ES or EN Rev A cartridge)." };
            }
            GameFiles g;
            g.rom = std::move(rom);
            g.lang = id->first;
            g.kind = kind;
            g.label = id->second;
            return { g, "" };
        }
    }

    std::pair<std::optional<GameFiles>, std::string> loadCia(const Bytes &cia)
    {
        CiaTitle t = readCia(cia);
        if (t.error != CiaError::None)
        {
            return { std::nullopt, ciaErrorText(t.error) };
        }
        auto dump = t.dump();
        auto rom = t.patchedRom();
        if (!dump || !rom)
        {
            return { std::nullopt, ciaErrorText(CiaError::NoRom) };
        }
        auto r = fromDump(*dump, std::make_shared<const Bytes>(std::move(*rom)), "cia");
        if (r.first)
        {
            r.first->sameEmulator = t.sameEmulator();
            r.first->titleCheck = t.sameEmulator()
                ? "VC emulator: the build the tool was made for (.code and config.ini match)."
                : "VC emulator: NOT the build the tool was made for (.code or config.ini differ). Results may not hold; please "
                  "report it.";
            r.first->label += " (" + t.romName + " + its patch)";
        }
        return r;
    }

    std::pair<std::optional<GameFiles>, std::string> loadRomPatch(const Bytes &rom, const std::string &patch)
    {
        int applied = 0;
        Bytes vc = applyPatch(rom, patch, &applied);
        if (applied == 0)
        {
            return { std::nullopt, "The patch file applied nothing: is it the title's .patch file (CGBBYT*)?" };
        }
        auto r = fromDump(rom, std::make_shared<const Bytes>(std::move(vc)), "rom+patch");
        if (r.first)
        {
            r.first->label += " + patch";
        }
        return r;
    }

    std::pair<std::optional<GameFiles>, std::string> loadDump(const Bytes &rom)
    {
        return fromDump(rom, std::make_shared<const Bytes>(rom), "dump");
    }

    // ------------------------------------------------------------------ attempts

    json toJson(const Attempt &a)
    {
        return { { "stats", a.stats }, { "ingame", a.ingame }, { "reset", std::string(1, a.reset) } };
    }

    Attempt attemptFromJson(const json &j)
    {
        Attempt a;
        a.stats = j.at("stats").get<std::array<int, 6>>();
        a.ingame = j.value("ingame", std::string("20:30"));
        std::string r = j.value("reset", std::string("A"));
        a.reset = r.empty() ? 'A' : r[0];
        return a;
    }

    bool shinyStats(const std::array<int, 6> &stats)
    {
        const auto &c = dvCands(stats);
        return !c.empty() && std::all_of(c.begin(), c.end(), [](std::uint16_t dv) { return dvShiny(dv); });
    }

    bool impossibleStats(const std::array<int, 6> &stats)
    {
        return dvCands(stats).empty();
    }

    // ------------------------------------------------------------------ calibration

    namespace
    {
        std::vector<CalJob> jobsFor(const Visible &v, const std::vector<Attempt> &attempts, int accuracy)
        {
            CrystalSave tpl(toolSave(v, 2));
            return calJobs(tpl, readMinimal(tpl), attempts, { 1, 2 }, -40, 40, accuracy);
        }

        bool has(const GridCache &cache, const std::string &key, int D)
        {
            auto it = cache.find(key);
            return it != cache.end() && it->second.D == D;
        }
    }

    int calibrationTodo(const Plan &, const Visible &v, const std::vector<Attempt> &attempts, int accuracy, const GridCache &cache)
    {
        int n = 0;
        for (const auto &j : jobsFor(v, attempts, accuracy))
        {
            n += !has(cache, j.key, accuracy);
        }
        return n;
    }

    CalOutcome calibrate(const RomPtr &rom, const Plan &p, const Visible &v, const std::vector<Attempt> &attempts, int accuracy,
                         std::optional<int> timerMs, GridCache &cache, int threads, const Progress &progress, const Cancel *cancel)
    {
        CalOutcome out;
        auto jobs = jobsFor(v, attempts, accuracy);
        std::vector<Grid> grids(jobs.size());
        bool ok = fill(
            static_cast<int>(jobs.size()), threads, [&](int i) { return has(cache, jobs[i].key, accuracy); },
            [&](int i) {
                const auto &j = jobs[i];
                grids[i] = gridX(rom, j.sram, j.rtc, p, 0, 0, CAL_FINAL, j.X, j.D);
            },
            progress, cancel);
        for (std::size_t i = 0; i < jobs.size(); i++)
        {
            if (!grids[i].dv.empty())
            {
                cache[jobs[i].key] = std::move(grids[i]);
            }
        }
        if (!ok)
        {
            return out;
        }
        CalIndex inv;
        for (const auto &j : jobs)
        {
            calIndexAdd(inv, j.kenji, j.k, j.hour, cache.at(j.key));
        }
        int N = static_cast<int>(attempts.size());
        out.complete = true;
        out.rank = calRank(attempts, inv, { 1, 2 }, accuracy);
        out.verdict = calVerdict(out.rank, N);
        out.state = calState(out.verdict, N);

        std::map<int, std::vector<int>> byHour;
        for (const auto &a : attempts)
        {
            int t = hm(a.ingame);
            byHour[t / 60].push_back(t % 60);
        }
        std::size_t most = 0;
        for (auto &[h, ms] : byHour)
        {
            if (ms.size() > most)
            {
                most = ms.size();
                std::sort(ms.begin(), ms.end());
                out.hour = h;
                out.minute = ms[(ms.size() - 1) / 2];
            }
        }
        if (timerMs && !out.rank.empty())
        {
            const Fit &b = out.verdict.best;
            out.carries = carryEvidence(attempts, inv, b.kenji, b.X, accuracy);
            std::vector<BandObs> obs;
            for (std::size_t i = 0; i < attempts.size(); i++)
            {
                obs.push_back({ out.carries[i], hm(attempts[i].ingame) % 60, attempts[i].reset });
            }
            out.band = bandAdvice(obs, *timerMs);
        }
        return out;
    }

    // ------------------------------------------------------------------ plans

    HuntSetup huntSetup(const Visible &v, int kenji, int k, int hour, int minute)
    {
        HuntSetup h;
        h.kenji = kenji;
        h.k = k;
        h.hour = hour;
        h.minute = minute;
        CarryRep cr = carryRep(hour, minute, k);
        h.rtc = cr.rtc;
        h.sram = toolSave(v, kenji, cr.offset);
        return h;
    }

    json toJson(const HuntSetup &h)
    {
        return { { "kenji", h.kenji }, { "k", h.k }, { "hour", h.hour }, { "minute", h.minute } };
    }

    HuntSetup huntSetupFromJson(const Visible &v, const json &j)
    {
        return huntSetup(v, j.at("kenji").get<int>(), j.at("k").get<int>(), j.at("hour").get<int>(), j.at("minute").get<int>());
    }

    json toJson(const Hit &h)
    {
        return { { "d6", h.d6 }, { "dyes", h.dyes }, { "final", h.final }, { "rtc", h.rtc }, { "enemy", hex(h.enemy) }, { "F_ok", h.fOk },
                 { "s_ok", h.sOk } };
    }

    Hit hitFromJson(const json &j)
    {
        Hit h {};
        h.d6 = j.at("d6").get<int>();
        h.dyes = j.at("dyes").get<int>();
        h.final = j.at("final").get<int>();
        h.rtc = j.at("rtc").get<Rtc>();
        std::string e = j.at("enemy").get<std::string>();
        for (std::size_t i = 0; i < 8 && 2 * i + 1 < e.size(); i++)
        {
            h.enemy[i] = static_cast<std::uint8_t>(std::stoi(e.substr(2 * i, 2), nullptr, 16));
        }
        h.fOk = j.at("F_ok").get<std::vector<int>>();
        h.sOk = j.at("s_ok").get<std::vector<int>>();
        return h;
    }

    FsearchResult planSearch(const RomPtr &rom, const Plan &p, const HuntSetup &h, const std::map<std::string, std::vector<Hit>> &done,
                             int threads, const Progress &progress, const Cancel *cancel)
    {
        auto jobs = fsJobs(p, { h.rtc });
        return fsearch(rom, h.sram, p, jobs, FS_TARGET, FS_UNION, threads, progress, done, cancel);
    }

    namespace
    {
        std::vector<FgridJob> switchJobs(int accuracy)
        {
            std::vector<FgridJob> jobs;
            for (int X : SWITCH_XS)
            {
                auto v = fgridJobs(2, accuracy, X);
                jobs.insert(jobs.end(), v.begin(), v.end());
            }
            return jobs;
        }
    }

    int switchTodo(int accuracy, const GridCache &cache)
    {
        int n = 0;
        for (const auto &j : switchJobs(accuracy))
        {
            n += !has(cache, j.key, accuracy);
        }
        return n;
    }

    bool switchGridsCached(const RomPtr &rom, const Plan &p, const HuntSetup &h, const Hit &p1, const Hit &p2, int accuracy,
                           GridCache &cache, int threads, const Progress &progress, const Cancel *cancel)
    {
        auto jobs = switchJobs(accuracy);
        std::vector<Grid> grids(jobs.size());
        bool ok = fill(
            static_cast<int>(jobs.size()), threads, [&](int i) { return has(cache, jobs[i].key, accuracy); },
            [&](int i) {
                const auto &j = jobs[i];
                const Hit &hit = j.plan == 0 ? p1 : p2;
                grids[i] = fgridJob(rom, h.sram, p, hit.d6, hit.dyes, hit.final, hit.rtc, j.F, j.D, j.X);
            },
            progress, cancel);
        for (std::size_t i = 0; i < jobs.size(); i++)
        {
            if (!grids[i].dv.empty())
            {
                cache[jobs[i].key] = std::move(grids[i]);
            }
        }
        return ok && switchTodo(accuracy, cache) == 0;
    }

    json toJson(const HuntAttempt &a)
    {
        return { { "plan", a.plan }, { "stats", a.stats }, { "timer_moved", a.timerMoved } };
    }

    HuntAttempt huntAttemptFromJson(const json &j)
    {
        HuntAttempt a;
        a.plan = j.value("plan", 1);
        a.stats = j.at("stats").get<std::array<int, 6>>();
        a.timerMoved = j.value("timer_moved", 0);
        return a;
    }

    SwitchAdvice huntAdvice(const GridCache &cache, const Hit &p1, const Hit &p2, int accuracy, const std::vector<HuntAttempt> &attempts)
    {
        GridOf gridOf = [&](int plan, int F, int X) -> const Grid * {
            auto it = cache.find(fgridKey(plan, F, X));
            return it == cache.end() ? nullptr : &it->second;
        };
        JointModel lik = jointModel(gridOf, errModel(0.5, accuracy, 0.05), 2);
        std::vector<HuntObs> obs;
        for (const auto &a : attempts)
        {
            obs.push_back({ a.plan - 1, dvCands(a.stats), a.timerMoved });
        }
        std::set<int> S1(p1.fOk.begin(), p1.fOk.end()), S2(p2.fOk.begin(), p2.fOk.end());
        return switchRule(lik, obs, S1, S2);
    }

    // ------------------------------------------------------------------ files

    namespace
    {
        constexpr char MAGIC[8] = { 'G', '2', 'G', 'R', 'I', 'D', '1', '\n' };

        template <class T>
        void put(std::ostream &o, T v)
        {
            o.write(reinterpret_cast<const char *>(&v), sizeof v);
        }

        template <class T>
        bool get(std::istream &i, T &v)
        {
            return static_cast<bool>(i.read(reinterpret_cast<char *>(&v), sizeof v));
        }
    }

    void saveGrids(const std::string &path, const GridCache &cache)
    {
        std::string tmp = path + ".tmp";
        {
            std::ofstream o(tmp, std::ios::binary);
            o.write(MAGIC, sizeof MAGIC);
            put<std::uint32_t>(o, static_cast<std::uint32_t>(cache.size()));
            for (const auto &[k, g] : cache)
            {
                put<std::uint16_t>(o, static_cast<std::uint16_t>(k.size()));
                o.write(k.data(), static_cast<std::streamsize>(k.size()));
                put<std::int16_t>(o, static_cast<std::int16_t>(g.X));
                put<std::uint8_t>(o, static_cast<std::uint8_t>(g.D));
                put<std::uint32_t>(o, static_cast<std::uint32_t>(g.dv.size()));
                o.write(reinterpret_cast<const char *>(g.dv.data()), static_cast<std::streamsize>(g.dv.size() * 2));
            }
        }
        std::remove(path.c_str());
        std::rename(tmp.c_str(), path.c_str());
    }

    GridCache loadGrids(const std::string &path)
    {
        GridCache out;
        std::ifstream in(path, std::ios::binary);
        char magic[8];
        if (!in.read(magic, sizeof magic) || !std::equal(magic, magic + 8, MAGIC))
        {
            return out;
        }
        std::uint32_t n = 0;
        if (!get(in, n))
        {
            return out;
        }
        for (std::uint32_t i = 0; i < n; i++)
        {
            std::uint16_t kl;
            std::int16_t X;
            std::uint8_t D;
            std::uint32_t m;
            if (!get(in, kl))
            {
                return {};
            }
            std::string k(kl, '\0');
            if (!in.read(k.data(), kl) || !get(in, X) || !get(in, D) || !get(in, m) || m > 100000)
            {
                return {};
            }
            Grid g;
            g.X = X;
            g.D = D;
            g.dv.resize(m);
            if (!in.read(reinterpret_cast<char *>(g.dv.data()), static_cast<std::streamsize>(m * 2)))
            {
                return {};
            }
            out[k] = std::move(g);
        }
        return out;
    }

    int framesToMs(int frames)
    {
        return static_cast<int>(std::lround(frames * 1000.0 / FPS));
    }
}
