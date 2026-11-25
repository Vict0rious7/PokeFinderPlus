/*
 * GS Ball Celebi: session state and re-save (see CelebiSession.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CelebiSession.hpp"
#include <Core/Gen2/CelebiSwitchRule.hpp>
#include <fstream>

namespace Gen2
{
    using json = nlohmann::json;

    namespace
    {
        template <class T>
        json opt(const std::optional<T> &v)
        {
            return v ? json(*v) : json(nullptr);
        }

        template <class T>
        std::optional<T> getOpt(const json &j, const char *k)
        {
            if (!j.contains(k) || j[k].is_null())
            {
                return std::nullopt;
            }
            return j[k].get<T>();
        }
    }

    json Session::toJson() const
    {
        json hist = json::array();
        for (const auto &c : classHistory)
        {
            hist.push_back(opt(c));
        }
        json j = { { "version", SESSION_VERSION },   { "resaves", resaves },      { "class_history", hist },
                   { "kenji_class", opt(kenjiClass) }, { "X", opt(X) },             { "timer_ms", opt(timerMs) },
                   { "reset_rule", resetRule },       { "k_plan", opt(kPlan) },    { "needs_calibration", needsCalibration },
                   { "log", log } };
        if (accuracy)
        {
            j["accuracy"] = *accuracy;
        }
        if (lang)
        {
            j["lang"] = *lang;
        }
        if (pBand)
        {
            j["p_band"] = *pBand;
        }
        return j;
    }

    Session Session::fromJson(const json &j)
    {
        Session s;
        s.resaves = j.value("resaves", 0);
        if (j.contains("class_history"))
        {
            for (const auto &c : j["class_history"])
            {
                s.classHistory.push_back(c.is_null() ? std::nullopt : std::optional<int>(c.get<int>()));
            }
        }
        s.kenjiClass = getOpt<int>(j, "kenji_class");
        s.X = getOpt<int>(j, "X");
        s.timerMs = getOpt<int>(j, "timer_ms");
        s.resetRule = j.value("reset_rule", std::string("A"));
        s.kPlan = getOpt<int>(j, "k_plan");
        s.needsCalibration = j.value("needs_calibration", true);
        s.accuracy = getOpt<int>(j, "accuracy");
        s.lang = getOpt<std::string>(j, "lang");
        s.pBand = getOpt<double>(j, "p_band");
        s.log = j.value("log", json::array());
        return s;
    }

    Session sessionLoad(const std::string &path)
    {
        if (!path.empty())
        {
            std::ifstream f(path);
            if (f)
            {
                return Session::fromJson(json::parse(f));
            }
        }
        return Session();
    }

    void sessionSave(const std::string &path, const Session &s)
    {
        if (!path.empty())
        {
            std::ofstream(path) << s.toJson().dump(1);
        }
    }

    std::vector<std::string> resaveStep(Session &s)
    {
        std::optional<int> cls = s.kenjiClass;
        s.resaves += 1;
        s.classHistory.push_back(cls);
        std::vector<std::string> out
            = { "Re-save " + std::to_string(s.resaves)
                    + " recorded. Your F and play-time seconds are new: start a new attempts file; the switch rule starts again "
                      "(uniform F, timer residual prior as at the start).",
                "Hunt on a LATER in-game day (Pokegear weekday) than the re-save: on the same day the game skips its daily reset." };
        if (cls && (*cls == 1 || *cls == 0))
        {
            s.kenjiClass = 2;
            s.needsCalibration = false;
            out.push_back("Kenji class was 1: the re-save wrote 3-6, so your save is now class >=2. No new calibration attempts and "
                          "no timer change. Re-plan for class >=2: makesave with your new visible values and --kenji 2, then fsearch + pick "
                          "+ switchrule grids in NEW directories.");
        }
        else
        {
            s.kenjiClass.reset();
            s.needsCalibration = true;
            out.push_back("Kenji class was >=2: the re-save wrote one less, which is class 1 if it was 2 (the tool cannot see it). "
                          "Calibrate again: 6 attempts with your CURRENT timer (start value = what you use now), calibrate --kenji 1,2 "
                          "in a NEW directory, then re-plan for the class found.");
        }
        if (s.resaves >= 2)
        {
            out.push_back(RESAVE_HINT);
        }
        s.log.push_back({ { "event", "resave" }, { "n", s.resaves }, { "class_before", opt(cls) }, { "class_after", opt(s.kenjiClass) } });
        return out;
    }

    void sessionAfterCalibration(Session &s, const Verdict &v, const std::optional<BandAdvice> &band, int N, std::optional<int> timerMs,
                                 int accuracy, const std::string &lang)
    {
        s.kenjiClass = v.best.kenji;
        s.X = v.best.X;
        s.needsCalibration = false;
        s.timerMs = timerMs;
        s.accuracy = accuracy;
        s.lang = lang;
        if (band)
        {
            s.resetRule = std::string(1, band->rule);
            s.kPlan = band->kPlan;
            s.pBand = std::round(band->pBand * 1000) / 1000;
        }
        s.log.push_back({ { "event", "calibration" }, { "attempts", N }, { "kenji", v.best.kenji }, { "X", v.best.X }, { "rule", s.resetRule } });
    }

    Bytes coreResave(const RomPtr &rom, const Bytes &sram, const Plan &p, const Rtc &rtc, int vb)
    {
        CrystalSave sv(sram);
        Bytes start = sv.bytes();
        auto c = std::make_unique<VcCore>();
        boot(*c, rom, start, rtc);
        c->runTo(build(planEvents(p, 0, 0, 1933)), vb);
        CrystalSave t(start);
        // the saved WRAM segments (savetool.SEG), all in WRAM bank 1 by address
        const std::pair<int, int> seg[] = { { 0xd47b, 0xdca5 }, { 0xdca5, 0xdcd7 }, { 0xdcd7, 0xdff5 } };
        for (auto [lo, hi] : seg)
        {
            std::vector<std::uint8_t> b;
            for (int a = lo; a < hi; a++)
            {
                b.push_back(c->wramAt(a));
            }
            t.wr(lo, b);
        }
        return t.bytes();
    }
}
