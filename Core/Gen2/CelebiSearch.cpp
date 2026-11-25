/*
 * GS Ball Celebi: grids, fsearch, pick, coverage, windows (see CelebiSearch.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CelebiSearch.hpp"
#include <Core/Gen2/CrystalSave.hpp>
#include <algorithm>
#include <cstdio>
#include <set>

namespace Gen2
{
    bool dvShiny(std::uint16_t dv)
    {
        int a = (dv >> 12) & 15, d = (dv >> 8) & 15, s = (dv >> 4) & 15, c = dv & 15;
        return d == 10 && s == 10 && c == 10 && (a == 2 || a == 3 || a == 6 || a == 7 || a == 10 || a == 11 || a == 14 || a == 15);
    }

    std::string hex(const Enemy &e)
    {
        char b[17];
        std::snprintf(b, sizeof b, "%02x%02x%02x%02x%02x%02x%02x%02x", e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7]);
        return b;
    }

    std::string dvHex(std::uint16_t dv)
    {
        char b[5];
        std::snprintf(b, sizeof b, "%04x", dv);
        return b;
    }

    Grid gridX(const RomPtr &rom, const Bytes &sram, const Rtc &rtc, const Plan &p, int d6, int dyes, int final, int X, int D)
    {
        auto shiftedPlan = [&](int e1, int e2, int e3) { return shifted(build(planEvents(p, d6 + e1, dyes + e2, final + e3)), X); };
        int s6 = p.events[9].start + d6, sy = p.events[10].start + dyes;
        Grid g;
        g.X = X;
        g.D = D;
        int w = g.width();
        g.dv.assign(w * w * w, 0);
        auto c = std::make_unique<VcCore>();
        boot(*c, rom, sram, rtc);
        c->runTo(shiftedPlan(0, 0, 0), s6 + X - 4);
        auto s0 = std::make_unique<VcCore::State>(c->snapshot());
        auto s1 = std::make_unique<VcCore::State>();
        auto s2 = std::make_unique<VcCore::State>();
        for (int e1 = -D; e1 <= D; e1++)
        {
            c->restore(*s0);
            c->runTo(shiftedPlan(e1, 0, 0), sy + X - 4);
            *s1 = c->snapshot();
            for (int e2 = -D; e2 <= D; e2++)
            {
                c->restore(*s1);
                c->runTo(shiftedPlan(e1, e2, 0), final + X - 4);
                *s2 = c->snapshot();
                for (int e3 = -D; e3 <= D; e3++)
                {
                    c->restore(*s2);
                    Bytes a = shiftedPlan(e1, e2, e3);
                    std::uint8_t o[8] = {};
                    c->runPlan(a, 1500, CELEBI, o);
                    g.dv[((e1 + D) * w + e2 + D) * w + e3 + D] = static_cast<std::uint16_t>((o[6] << 8) | o[7]);
                }
            }
        }
        return g;
    }

    Grid fgridJob(const RomPtr &rom, const Bytes &sram, const Plan &p, int d6, int dyes, int final, const Rtc &rtc, int F, int D, int X)
    {
        CrystalSave s(sram);
        s.wr8(Sym::wGameTimeFrames, static_cast<std::uint8_t>(F));
        return gridX(rom, s.bytes(), rtc, p, d6, dyes, final, X, D);
    }

    std::vector<std::optional<Enemy>> scanFinal(const RomPtr &rom, const Bytes &sram, const Rtc &rtc, const Events &ev, int lo, int hi, int hold)
    {
        Bytes j = build(ev);
        auto c = std::make_unique<VcCore>();
        boot(*c, rom, sram, rtc);
        c->runTo(j, lo - 1);
        auto cur = std::make_unique<VcCore::State>(c->snapshot());
        std::vector<std::optional<Enemy>> out;
        Bytes jj;
        for (int f = lo; f < hi; f++)
        {
            c->restore(*cur);
            jj = j;
            for (int k = f; k < f + hold; k++)
            {
                jj[k] |= BTN_A;
            }
            RunResult r = run(*c, jj, 1500);
            out.push_back(r.ret > 0 ? r.enemy : std::nullopt);
            c->restore(*cur);
            c->runTo(j, f);
            *cur = c->snapshot();
        }
        return out;
    }

    Coverage coverage(const RomPtr &rom, const Bytes &sram, const Plan &p, int d6, int dyes, int final, const Rtc &rtc)
    {
        Bytes j = build(planEvents(p, d6, dyes, final));
        Coverage out;
        auto c = std::make_unique<VcCore>();
        for (auto [vec, addr] : { std::pair { &out.F, Sym::wGameTimeFrames }, std::pair { &out.s, Sym::wGameTimeSeconds } })
        {
            for (int v = 0; v < 60; v++)
            {
                CrystalSave s(sram);
                s.wr8(addr, static_cast<std::uint8_t>(v));
                boot(*c, rom, s.bytes(), rtc);
                RunResult r = run(*c, j, TIMELINE);
                if (r.ret > 0 && isShiny(*r.enemy))
                {
                    vec->push_back(v);
                }
            }
        }
        return out;
    }

    std::vector<FsJob> fsJobs(const Plan &p, const std::vector<Rtc> &rtcs, int D, int off, int W, int chunks)
    {
        std::vector<std::pair<int, int>> pairs;
        for (int a = -D; a <= D; a++)
        {
            for (int b = -D; b <= D; b++)
            {
                pairs.emplace_back(a, b);
            }
        }
        std::stable_sort(pairs.begin(), pairs.end(), [](const auto &x, const auto &y) {
            int dx = std::abs(x.first) + std::abs(x.second), dy = std::abs(y.first) + std::abs(y.second);
            return dx != dy ? dx < dy : x < y;
        });
        std::vector<FsJob> jobs;
        for (auto [d6, dy] : pairs)
        {
            for (int c = 0; c < chunks; c++)
            {
                int lo = p.finalMin + off + std::max({ d6, dy, 0 }) + W * c;
                for (const auto &rtc : rtcs)
                {
                    char key[64];
                    std::snprintf(key, sizeof key, "fs_%+d_%+d_%04d_%02d%02d", d6, dy, lo, rtc[2], rtc[1]);
                    jobs.push_back({ key, d6, dy, lo, lo + W, rtc });
                }
            }
        }
        return jobs;
    }

    std::vector<Hit> fsJob(const RomPtr &rom, const Bytes &sram, const Plan &p, const FsJob &job)
    {
        std::vector<Hit> hits;
        auto res = scanFinal(rom, sram, job.rtc, planEvents(p, job.d6, job.dyes), job.lo, job.hi);
        for (int i = 0; i < static_cast<int>(res.size()); i++)
        {
            if (res[i] && isShiny(*res[i]))
            {
                int f = job.lo + i;
                Coverage cv = coverage(rom, sram, p, job.d6, job.dyes, f, job.rtc);
                hits.push_back({ job.d6, job.dyes, f, job.rtc, *res[i], cv.F, cv.s });
            }
        }
        return hits;
    }

    std::pair<std::optional<std::size_t>, std::optional<std::size_t>> pick(const std::vector<Hit> &hits)
    {
        if (hits.empty())
        {
            return { std::nullopt, std::nullopt };
        }
        // Python max(): the first element with the largest key
        std::size_t p1 = 0;
        for (std::size_t i = 1; i < hits.size(); i++)
        {
            auto k = std::pair(hits[i].fOk.size(), hits[i].sOk.size()), b = std::pair(hits[p1].fOk.size(), hits[p1].sOk.size());
            if (k > b)
            {
                p1 = i;
            }
        }
        std::set<int> in1(hits[p1].fOk.begin(), hits[p1].fOk.end());
        auto missed = [&](const Hit &h) {
            return static_cast<std::size_t>(std::count_if(h.fOk.begin(), h.fOk.end(), [&](int F) { return !in1.count(F); }));
        };
        std::optional<std::size_t> p2;
        for (std::size_t i = 0; i < hits.size(); i++)
        {
            if (i == p1)
            {
                continue;
            }
            if (!p2 || std::pair(missed(hits[i]), hits[i].fOk.size()) > std::pair(missed(hits[*p2]), hits[*p2].fOk.size()))
            {
                p2 = i;
            }
        }
        return { p1, p2 };
    }

    std::vector<FgridJob> fgridJobs(int nplans, int D, int X)
    {
        std::vector<FgridJob> out;
        for (int i = 0; i < nplans; i++)
        {
            for (int F = 0; F < 60; F++)
            {
                char key[32];
                if (X == 0)
                {
                    std::snprintf(key, sizeof key, "f_%d_%02d", i, F);
                }
                else
                {
                    std::snprintf(key, sizeof key, "f_%d_%02d_x%+d", i, F, X);
                }
                out.push_back({ key, i, F, D, X });
            }
        }
        return out;
    }

    void parallelFor(int n, int threads, const std::function<void(int)> &fn)
    {
        int nt = threads > 0 ? threads : static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
        nt = std::min(nt, std::max(n, 1));
        std::atomic<int> next { 0 };
        std::vector<std::thread> pool;
        for (int t = 0; t < nt; t++)
        {
            pool.emplace_back([&] {
                for (int i; (i = next++) < n;)
                {
                    fn(i);
                }
            });
        }
        for (auto &th : pool)
        {
            th.join();
        }
    }

    std::vector<std::optional<Enemy>> sweep(const RomPtr &rom, const Bytes &sram, const Events &ev, int threads)
    {
        Bytes j = build(ev);
        std::vector<std::optional<Enemy>> res(1440);
        parallelFor(1440, threads, [&](int t) {
            auto c = std::make_unique<VcCore>();
            boot(*c, rom, sram, { 30, t % 60, t / 60, 117, 0 });
            RunResult r = run(*c, j, TIMELINE);
            res[t] = r.ret > 0 ? r.enemy : std::nullopt;
        });
        return res;
    }

    std::vector<std::string> windows(const std::vector<std::optional<Enemy>> &res, int offsetMin,
                                     const std::function<bool(const std::optional<Enemy> &)> &pred)
    {
        auto P = pred ? pred : [](const std::optional<Enemy> &e) { return e.has_value() && isShiny(*e); };
        std::vector<std::pair<int, bool>> ing;
        for (int t = 0; t < static_cast<int>(res.size()); t++)
        {
            ing.emplace_back((t + offsetMin) % 1440, P(res[t]));
        }
        std::sort(ing.begin(), ing.end());
        std::vector<std::pair<int, int>> out;
        std::optional<std::pair<int, int>> cur;
        for (auto [t, s] : ing)
        {
            if (s && !cur)
            {
                cur = std::pair(t, t);
            }
            else if (s)
            {
                cur->second = t;
            }
            else if (cur)
            {
                out.push_back(*cur);
                cur.reset();
            }
        }
        if (cur)
        {
            out.push_back(*cur);
        }
        if (out.size() > 1 && out.front().first == 0 && out.back().second == 1439)
        {
            std::vector<std::pair<int, int>> m { { out.back().first, out.front().second } };
            m.insert(m.end(), out.begin() + 1, out.end() - 1);
            out = m;
        }
        std::vector<std::string> s;
        for (auto [a, b] : out)
        {
            char buf[16];
            std::snprintf(buf, sizeof buf, "%02d:%02d-%02d:%02d", a / 60, a % 60, b / 60, b % 60);
            s.push_back(buf);
        }
        return s;
    }
}
