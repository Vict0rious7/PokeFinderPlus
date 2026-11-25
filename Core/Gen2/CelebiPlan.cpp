/*
 * GS Ball Celebi: timelines, plan discovery, timer phases (see CelebiPlan.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CelebiPlan.hpp"
#include <algorithm>
#include <stdexcept>

namespace Gen2
{
    namespace
    {
        // Victor's margins after the earliest accepted frame of each prompt (prompts 1-5 are free, SPEC 3.1); 40 beyond
        constexpr int FR_MARGINS[] = { 33, 29, 43, 25, 31, 50, 85 };

        /// The prototype's Discover: an A press at VBlank f is "accepted" when it changes the state 90 VBlanks later.
        /// Same results as the prototype; faster because the base timeline is advanced instead of re-booted for each f
        /// (run_to stops at the first frame reaching the VBlank count, so both paths end in the same state).
        class Discover
        {
        public:
            Discover(const RomPtr &rom, const Bytes &sram, const Rtc &rtc) :
                rom(rom), sram(sram), rtc(rtc), base(std::make_unique<VcCore>()), probe(std::make_unique<VcCore>())
            {
                // restore() copies the emulated state only; each core holds its own ROM pointer
                base->loadRom(rom);
                probe->loadRom(rom);
            }

            bool accepted(const Events &events, int f, int h = 6, int H = 90)
            {
                Bytes bj = build(events);
                Events te = events;
                te.push_back({ f, f + h, BTN_A });
                Bytes tj = build(te);
                at(events, bj, f);
                probe->restore(base->snapshot());
                probe->runTo(bj, f + H);
                std::uint64_t h0 = probe->stateHash();
                probe->restore(base->snapshot());
                probe->runTo(tj, f + H);
                return h0 != probe->stateHash();
            }

            std::optional<int> firstAccept(const Events &events, int lo, int hi)
            {
                for (int f = lo; f < hi; f++)
                {
                    if (accepted(events, f))
                    {
                        return f;
                    }
                }
                return std::nullopt;
            }

            bool reaches(const Events &events)
            {
                boot(*probe, rom, sram, rtc);
                return run(*probe, build(events), TIMELINE).ret > 0;
            }

        private:
            RomPtr rom;
            Bytes sram;
            Rtc rtc;
            std::unique_ptr<VcCore> base, probe;
            Events baseEvents;
            long baseAt = -1;
            bool booted = false;

            void at(const Events &events, const Bytes &bj, int f)
            {
                if (!booted || events != baseEvents || f - 1 < baseAt)
                {
                    boot(*base, rom, sram, rtc);
                    baseEvents = events;
                    booted = true;
                }
                base->runTo(bj, f - 1);
                baseAt = f - 1;
            }
        };
    }

    Bytes build(const Events &events, int n)
    {
        Bytes j(n, 0);
        for (const auto &e : events)
        {
            for (int k = std::max(e.start, 0); k < std::min(e.end, n); k++)
            {
                j[k] |= e.mask;
            }
        }
        return j;
    }

    Events planEvents(const Plan &p, int d6, int dyes, std::optional<int> final)
    {
        Events ev = p.events;
        ev[9].start += d6;
        ev[9].end += d6;
        ev[10].start += dyes;
        ev[10].end += dyes;
        if (final)
        {
            ev.push_back({ *final, *final + 5, BTN_A });
        }
        return ev;
    }

    Bytes shifted(const Bytes &j, int X)
    {
        int n = static_cast<int>(j.size());
        Bytes out(n);
        for (int k = 0; k < n; k++)
        {
            int i = k - X;
            out[k] = (0 <= i && i < n) ? j[i] : (i < 0 ? 0 : j[n - 1]);
        }
        return out;
    }

    int hm(const std::string &s)
    {
        auto c = s.find(':');
        int h = std::stoi(s.substr(0, c));
        std::string rest = s.substr(c + 1);
        auto c2 = rest.find(':');
        int m = std::stoi(c2 == std::string::npos ? rest : rest.substr(0, c2));
        return h * 60 + m;
    }

    Rtc chipRtc(int ingameMin, int offsetMin, int sec, int day)
    {
        int t = ((ingameMin - offsetMin) % 1440 + 1440) % 1440;
        return { sec, t % 60, t / 60, day, 0 };
    }

    void boot(VcCore &c, const RomPtr &rom, const Bytes &sram, const Rtc &rtc, int divcd0, int counter0)
    {
        c.loadRom(rom);
        c.loadSram(sram);
        c.reset(divcd0, counter0);
        c.setRtc(rtc[0], rtc[1], rtc[2], rtc[3], rtc[4]);
    }

    RunResult run(VcCore &c, const Bytes &j, int maxf, int species)
    {
        std::array<std::uint8_t, 8> o {};
        int r = c.runPlan(j, maxf, species, o.data());
        RunResult res { std::nullopt, r };
        if (r > 0)
        {
            res.enemy = o;
        }
        return res;
    }

    Plan discover(const RomPtr &rom, const Bytes &sram, const Rtc &rtc, int cont, int info)
    {
        Discover D(rom, sram, rtc);
        Events ev = { { 26, 334, BTN_A }, { cont, cont + 24, BTN_A }, { info, info + 104, BTN_A }, { info + 43, 4000, BTN_B } };
        Plan p;
        int last = info + 104, i = 0;
        for (;;)
        {
            auto e = D.firstAccept(ev, last, last + 1500);
            if (!e)
            {
                throw std::runtime_error("no accepted press after VBlank " + std::to_string(last));
            }
            int m = i < 7 ? FR_MARGINS[i] : 40;
            int f = *e + m;
            Events probe = ev;
            probe.push_back({ f, f + 5, BTN_A });
            if (D.reaches(probe))
            {
                int g = *e;
                for (;;)
                {
                    Events q = ev;
                    q.push_back({ g, g + 5, BTN_A });
                    if (D.reaches(q))
                    {
                        break;
                    }
                    g++;
                }
                p.presses.push_back({ *e, std::nullopt, g });
                break;
            }
            ev.push_back({ f, f + 6, BTN_A });
            p.presses.push_back({ *e, f, std::nullopt });
            last = f + 6;
            i++;
        }
        if (p.presses.size() != 8)
        {
            throw std::runtime_error("expected 6 prompts + YES + final, got " + std::to_string(p.presses.size()) + " presses");
        }
        p.events = ev;
        p.finalMin = *p.presses.back().finalMin;
        return p;
    }

    std::vector<TimerRow> timer(const RomPtr &rom, const Bytes &sram, const Plan &p, int d6, int dyes, int final, const Rtc &rtc)
    {
        Events ev = planEvents(p, d6, dyes, final);
        Bytes j = build(ev);
        auto c = std::make_unique<VcCore>();
        boot(*c, rom, sram, rtc);
        std::vector<std::pair<std::string, int>> names
            = { { "CONTINUE", ev[1].start }, { "info", ev[2].start }, { "6th prompt", ev[9].start }, { "YES", ev[10].start }, { "final A", final } };
        int last = 0;
        for (const auto &n : names)
        {
            last = std::max(last, n.second);
        }
        std::vector<int> fr(last + 1, 0);
        for (int vb = 1; vb <= last; vb++)
        {
            c->runTo(j, vb);
            fr[vb] = static_cast<int>(c->frames() - 1);
        }
        std::vector<TimerRow> rows;
        int prev = 0;
        for (const auto &[n, vb] : names)
        {
            int gb = fr[vb], beep = gb - LEAD;
            rows.push_back({ n, vb, gb, beep, beep - prev });
            prev = beep;
        }
        return rows;
    }
}
