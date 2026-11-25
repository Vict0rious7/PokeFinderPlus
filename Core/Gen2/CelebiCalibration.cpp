/*
 * GS Ball Celebi: calibration and reset band (see CelebiCalibration.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CelebiCalibration.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <stdexcept>

namespace Gen2
{
    int stat(int base, int dv, int level, bool hp)
    {
        return (2 * (base + dv)) * level / 100 + (hp ? level + 10 : 5);
    }

    namespace
    {
        const std::map<int, std::vector<int>> &stinv()
        {
            static const std::map<int, std::vector<int>> m = [] {
                std::map<int, std::vector<int>> r;
                for (int d = 0; d < 16; d++)
                {
                    r[stat(100, d)].push_back(d);
                }
                return r;
            }();
            return m;
        }

        const std::vector<int> &inv(int s)
        {
            static const std::vector<int> none;
            auto it = stinv().find(s);
            return it == stinv().end() ? none : it->second;
        }
    }

    const std::set<std::uint16_t> &dvCands(const std::array<int, 6> &s)
    {
        static std::map<std::array<int, 6>, std::set<std::uint16_t>> cache;
        static std::mutex m;
        std::lock_guard<std::mutex> lock(m);
        auto it = cache.find(s);
        if (it != cache.end())
        {
            return it->second;
        }
        auto [H, Atk, Def, SA, SD, Spe] = s;
        std::set<std::uint16_t> out;
        if (SA == SD)
        {
            for (int a : inv(Atk))
            {
                for (int d : inv(Def))
                {
                    for (int sp : inv(Spe))
                    {
                        for (int sc : inv(SA))
                        {
                            int h = (a & 1) * 8 + (d & 1) * 4 + (sp & 1) * 2 + (sc & 1);
                            if (stat(100, h, 30, true) == H)
                            {
                                out.insert(static_cast<std::uint16_t>((((a << 4) | d) << 8) | ((sp << 4) | sc)));
                            }
                        }
                    }
                }
            }
        }
        return cache[s] = out;
    }

    std::array<int, 6> dvStats(std::uint16_t dv)
    {
        int a = (dv >> 12) & 15, d = (dv >> 8) & 15, sp = (dv >> 4) & 15, sc = dv & 15;
        int h = (a & 1) * 8 + (d & 1) * 4 + (sp & 1) * 2 + (sc & 1);
        return { stat(100, h, 30, true), stat(100, a), stat(100, d), stat(100, sc), stat(100, sc), stat(100, sp) };
    }

    const std::set<std::uint16_t> &dvCandsOf(std::uint16_t dv)
    {
        return dvCands(dvStats(dv));
    }

    CarryRep carryRep(int ih, int im, int k, int isec)
    {
        const int *want = CARRY[k];
        int Os = want[0] ? 40 : 0;
        std::vector<int> hs { want[2] ? 22 : 0 }, ms { want[1] ? 30 : 0 };
        for (int i = 0; i < 24; i++)
        {
            hs.push_back(i);
        }
        for (int i = 0; i < 60; i++)
        {
            ms.push_back(i);
        }
        for (int Oh : hs)
        {
            for (int Om : ms)
            {
                int t = ((ih * 3600 + im * 60 + isec - (Oh * 3600 + Om * 60 + Os)) % 86400 + 86400) % 86400;
                int cs = t % 60, cm = (t / 60) % 60, ch = t / 3600;
                int c_s = cs + Os >= 60, c_m = cm + Om + c_s >= 60, c_h = ch + Oh + c_m >= 24;
                if (c_s == want[0] && c_m == want[1] && c_h == want[2])
                {
                    return { { cs, cm, ch, 117, 0 }, { Oh, Om, Os } };
                }
            }
        }
        throw std::runtime_error("no representative clock for this in-game time and carry count");
    }

    std::vector<CalJob> calJobs(const CrystalSave &tpl, const Minimal &mini, const std::vector<Attempt> &attempts,
                                const std::vector<int> &kenjis, int xlo, int xhi, int D)
    {
        std::map<int, std::vector<int>> byHour;
        for (const auto &a : attempts)
        {
            int t = hm(a.ingame);
            byHour[t / 60].push_back(t % 60);
        }
        std::map<int, int> hours; // hour -> lower-median minute of that hour
        for (auto &[h, ms] : byHour)
        {
            std::sort(ms.begin(), ms.end());
            hours[h] = ms[(ms.size() - 1) / 2];
        }
        std::vector<CalJob> jobs;
        for (int ke : kenjis)
        {
            for (int k = 0; k < 4; k++)
            {
                for (auto [h, m] : hours)
                {
                    CarryRep cr = carryRep(h, m, k);
                    Minimal mm = mini;
                    mm.start = { mini.start[0], cr.offset[0], cr.offset[1], cr.offset[2] };
                    mm.kenji = ke;
                    mm.setKenji = true;
                    CrystalSave s = applyMinimal(tpl, mm);
                    Bytes sram = s.bytes();
                    for (int X = xlo; X <= xhi; X++)
                    {
                        char key[32];
                        std::snprintf(key, sizeof key, "g_%d_%d_%02d_%+03d", ke, k, h, X);
                        jobs.push_back({ key, ke, k, h, X, D, cr.rtc, sram });
                    }
                }
            }
        }
        return jobs;
    }

    void calIndexAdd(CalIndex &inv, int kenji, int k, int hour, const Grid &g)
    {
        int D = g.D;
        for (int e1 = -D; e1 <= D; e1++)
        {
            for (int e2 = -D; e2 <= D; e2++)
            {
                for (int e3 = -D; e3 <= D; e3++)
                {
                    inv[{ kenji, k, hour, g.at(e1, e2, e3) }].push_back({ g.X, e1, e2, e3 });
                }
            }
        }
    }

    namespace
    {
        int maxAbsErr(const Leaf &x)
        {
            return std::max({ std::abs(x[1]), std::abs(x[2]), std::abs(x[3]) });
        }

        /// leaves explaining attempt a under (Kenji, k), errors within D (the prototype's ex())
        std::vector<Leaf> ex(const Attempt &a, const CalIndex &inv, int ke, int k, int D)
        {
            std::vector<Leaf> out;
            int h = hm(a.ingame) / 60;
            for (auto c : dvCands(a.stats))
            {
                auto it = inv.find({ ke, k, h, c });
                if (it == inv.end())
                {
                    continue;
                }
                for (const auto &x : it->second)
                {
                    if (maxAbsErr(x) <= D)
                    {
                        out.push_back(x);
                    }
                }
            }
            return out;
        }

        int pyRound(double v)
        {
            return static_cast<int>(std::nearbyint(v)); // round half to even, as Python's round()
        }
    }

    std::vector<Fit> calRank(const std::vector<Attempt> &attempts, const CalIndex &inv, const std::vector<int> &kenjis, int D)
    {
        std::vector<Fit> out;
        for (int ke : kenjis)
        {
            for (const auto &kset : KSETS)
            {
                std::vector<int> ks { kset[0] };
                if (kset[1] >= 0)
                {
                    ks.push_back(kset[1]);
                }
                std::vector<std::vector<Leaf>> E;
                std::set<int> xs;
                for (const auto &a : attempts)
                {
                    std::vector<Leaf> e;
                    for (int k : ks)
                    {
                        auto v = ex(a, inv, ke, k, D);
                        e.insert(e.end(), v.begin(), v.end());
                    }
                    for (const auto &x : e)
                    {
                        xs.insert(x[0]);
                    }
                    E.push_back(std::move(e));
                }
                if (xs.empty())
                {
                    xs.insert(0);
                }
                auto sc = [&](int X) {
                    std::array<int, 3> r {};
                    const int ws[3] = { 2, 0, 1 };
                    for (int i = 0; i < 3; i++)
                    {
                        for (const auto &e : E)
                        {
                            r[i] += std::any_of(e.begin(), e.end(), [&](const Leaf &x) { return std::abs(x[0] - X) <= ws[i]; });
                        }
                    }
                    return r;
                };
                // Python max(): first X (ascending) with the largest (score, -|X|)
                int bestX = *xs.begin() - 2;
                auto bestK = std::pair(sc(bestX), -std::abs(bestX));
                for (int X = *xs.begin() - 1; X <= *xs.rbegin() + 2; X++)
                {
                    auto k = std::pair(sc(X), -std::abs(X));
                    if (k > bestK)
                    {
                        bestK = k;
                        bestX = X;
                    }
                }
                Fit f;
                f.explained = bestK.first[0];
                f.kenji = ke;
                f.carries = ks;
                f.X = bestX;
                f.ms = pyRound(bestX * 1000 / FPS);
                for (const auto &e : E)
                {
                    std::set<std::array<int, 3>> at;
                    for (const auto &x : e)
                    {
                        if (x[0] == bestX)
                        {
                            at.insert({ x[1], x[2], x[3] });
                        }
                    }
                    f.errors.emplace_back(at.begin(), at.end());
                    if (!at.empty())
                    {
                        f.errorNote.emplace_back();
                    }
                    else if (e.empty())
                    {
                        f.errorNote.emplace_back("none");
                    }
                    else
                    {
                        // nearest X explaining the attempt; ties: the lower X (the prototype's tie order is not fixed)
                        int nx = e[0][0];
                        for (const auto &x : e)
                        {
                            if (std::abs(x[0] - bestX) < std::abs(nx - bestX) || (std::abs(x[0] - bestX) == std::abs(nx - bestX) && x[0] < nx))
                            {
                                nx = x[0];
                            }
                        }
                        char buf[16];
                        std::snprintf(buf, sizeof buf, "X%+d", nx - bestX);
                        f.errorNote.emplace_back(buf);
                    }
                }
                out.push_back(std::move(f));
            }
        }
        std::stable_sort(out.begin(), out.end(), [](const Fit &a, const Fit &b) {
            return a.explained != b.explained ? a.explained > b.explained : a.carries.size() < b.carries.size();
        });
        return out;
    }

    Verdict calVerdict(const std::vector<Fit> &rank, int N)
    {
        const Fit &b = rank[0];
        std::optional<Fit> rival;
        for (std::size_t i = 1; i < rank.size(); i++)
        {
            if (rank[i].kenji != b.kenji || std::abs(rank[i].X - b.X) > 2)
            {
                rival = rank[i];
                break;
            }
        }
        Verdict v;
        v.accepted = N >= MIN_ATT && b.explained >= N - 2 && (!rival || rival->explained <= N - 3);
        v.best = b;
        v.runnerUp = rival;
        for (const auto &o : rank)
        {
            if (o.kenji == b.kenji && std::abs(o.X - b.X) <= 2)
            {
                if (o.explained == b.explained)
                {
                    v.carriesBest.push_back(o.carries);
                }
                else if (o.explained == b.explained - 1)
                {
                    v.carriesNext.push_back(o.carries);
                }
            }
        }
        return v;
    }

    CalState calState(const Verdict &v, int N)
    {
        if (v.accepted)
        {
            return CalState::Accepted;
        }
        if (N < MIN_ATT)
        {
            return CalState::NeedMore;
        }
        return N < MAX_ATT ? CalState::AddTwo : CalState::ShiftTimer;
    }

    double resetWait(char rule)
    {
        return rule == 'B' ? 0.0 : 20.0;
    }

    std::pair<double, double> mlSeconds(char rule, double timerMs)
    {
        double w = resetWait(rule) + timerMs / 1000.0;
        return { w + ML_DELAY[0] + REACTION[0], w + ML_DELAY[1] + REACTION[1] };
    }

    double pCarry(double Os, double lo, double hi)
    {
        if (Os <= lo)
        {
            return 0.0;
        }
        if (Os >= hi)
        {
            return 1.0;
        }
        return (Os - lo) / (hi - lo);
    }

    std::vector<int> bandOf(char rule, double timerMs, double eps)
    {
        auto [lo, hi] = mlSeconds(rule, timerMs);
        std::vector<int> out;
        for (int o = 0; o < 60; o++)
        {
            double p = pCarry(o, lo, hi);
            if (eps < p && p < 1 - eps)
            {
                out.push_back(o);
            }
        }
        return out;
    }

    std::vector<std::set<int>> carryEvidence(const std::vector<Attempt> &att, const CalIndex &inv, int kenji, int X, int D)
    {
        std::vector<std::set<int>> out;
        for (const auto &a : att)
        {
            int h = hm(a.ingame) / 60;
            std::set<int> ks;
            for (int k = 0; k < 4; k++)
            {
                for (auto c : dvCands(a.stats))
                {
                    auto it = inv.find({ kenji, k, h, c });
                    if (it != inv.end()
                        && std::any_of(it->second.begin(), it->second.end(),
                                       [&](const Leaf &x) { return std::abs(x[0] - X) <= 2 && maxAbsErr(x) <= D; }))
                    {
                        ks.insert(k);
                    }
                }
            }
            out.push_back(ks);
        }
        return out;
    }

    std::vector<double> bandPosterior(const std::vector<BandObs> &obs, double timerMs, double eps)
    {
        // joint posterior over offset seconds Os (60) x offset minutes Om (60) x constant hour carry h0 (2):
        // count = c_s + [minute < Om + c_s] + h0
        std::array<std::array<double, 60>, 2> pv {};
        for (int r = 0; r < 2; r++)
        {
            auto [lo, hi] = mlSeconds(r ? 'B' : 'A', timerMs);
            for (int o = 0; o < 60; o++)
            {
                pv[r][o] = pCarry(o, lo, hi);
            }
        }
        std::vector<double> post(60 * 60 * 2, 1.0);
        for (const auto &ob : obs)
        {
            if (ob.ks.empty() || ob.ks.size() == 4)
            {
                continue;
            }
            const auto &p = pv[ob.rule == 'B' ? 1 : 0];
            double extra = eps * static_cast<double>(ob.ks.size()) / 4;
            for (int o = 0; o < 60; o++)
            {
                for (int om = 0; om < 60; om++)
                {
                    int c0 = ob.minute < om, c1 = ob.minute < om + 1;
                    for (int h = 0; h < 2; h++)
                    {
                        double lk = 0;
                        for (int y : ob.ks)
                        {
                            lk += p[o] * (1 + c1 + h == y) + (1 - p[o]) * (c0 + h == y);
                        }
                        post[(o * 60 + om) * 2 + h] *= (1 - eps) * lk + extra;
                    }
                }
            }
        }
        double z = 0;
        for (double v : post)
        {
            z += v;
        }
        std::vector<double> m(60, 1.0 / 60);
        if (z > 0)
        {
            for (int o = 0; o < 60; o++)
            {
                double s = 0;
                for (int i = 0; i < 120; i++)
                {
                    s += post[o * 120 + i] / z;
                }
                m[o] = s;
            }
        }
        return m;
    }

    BandAdvice bandAdvice(const std::vector<BandObs> &obs, double timerMs, double margin, double vmin)
    {
        auto post = bandPosterior(obs, timerMs);
        auto [aLo, aHi] = mlSeconds('A', timerMs);
        auto [bLo, bHi] = mlSeconds('B', timerMs);
        std::vector<int> singleA, singleB;
        std::set<int> seen;
        for (const auto &o : obs)
        {
            if (o.ks.size() == 1)
            {
                (o.rule == 'B' ? singleB : singleA).push_back(*o.ks.begin());
                seen.insert(*o.ks.begin());
            }
        }
        double vA = 0, vB = 0;
        for (int o = 0; o < 60; o++)
        {
            double pA = pCarry(o, aLo, aHi), pB = pCarry(o, bLo, bHi);
            vA += post[o] * std::max(pA, 1 - pA);
            vB += post[o] * (singleB.empty() ? pB : std::max(pB, 1 - pB));
        }
        BandAdvice a;
        a.bandA = bandOf('A', timerMs);
        a.bandB = bandOf('B', timerMs);
        a.pBand = 0;
        for (int o : a.bandA)
        {
            a.pBand += post[o];
        }
        // Counter.most_common(1): the highest count, ties to the first value seen
        auto mode = [](const std::vector<int> &v) -> std::optional<int> {
            if (v.empty())
            {
                return std::nullopt;
            }
            std::vector<std::pair<int, int>> cnt; // value, count in first-seen order
            for (int x : v)
            {
                auto it = std::find_if(cnt.begin(), cnt.end(), [&](const auto &c) { return c.first == x; });
                if (it == cnt.end())
                {
                    cnt.emplace_back(x, 1);
                }
                else
                {
                    it->second++;
                }
            }
            auto best = cnt.begin();
            for (auto it = cnt.begin(); it != cnt.end(); ++it)
            {
                if (it->second > best->second)
                {
                    best = it;
                }
            }
            return best->first;
        };
        if (vB > vA + margin)
        {
            a.rule = 'B';
            a.value = vB;
            a.kPlan = !singleB.empty() ? mode(singleB) : (!seen.empty() ? std::optional<int>(*seen.rbegin()) : std::nullopt);
        }
        else
        {
            a.rule = 'A';
            a.value = vA;
            a.kPlan = mode(singleA);
        }
        a.confirm = a.rule == 'B' && singleB.size() < 2;
        a.more = a.value < vmin || a.confirm;
        a.mixed = std::set<int>(singleA.begin(), singleA.end()).size() > 1;
        return a;
    }
}
