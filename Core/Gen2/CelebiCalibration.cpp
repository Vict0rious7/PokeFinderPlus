/*
 * GS Ball Celebi: calibration and reset band (see CelebiCalibration.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CelebiCalibration.hpp"
#include <algorithm>
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

}
