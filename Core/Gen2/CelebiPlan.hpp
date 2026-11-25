/*
 * GS Ball Celebi (Crystal VC): joypad timelines, plan discovery (the press sequence from power-on for a language + save)
 * and the PokeTimerPlus phases of a plan.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 *
 * Port of the research prototype's celebi_tool.py v0.6 (build, plan_events, Discover / discover, timer). Timelines are
 * keyed by VBlank count (Crystal's VBlank handler at 0x02c5). Checked against the prototype (Test/Gen2).
 */

#ifndef CELEBIPLAN_HPP
#define CELEBIPLAN_HPP

#include <Core/Gen2/VcCore.hpp>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Gen2
{
    using Bytes = std::vector<std::uint8_t>;
    using RomPtr = std::shared_ptr<const Bytes>;

    constexpr std::uint8_t BTN_A = 1, BTN_B = 2;
    constexpr double FPS = 59.8312;
    constexpr int LEAD = 12; // the beep is this many frames before the press (reaction time)
    constexpr int TIMELINE = 4000; // VBlanks in a timeline
    constexpr int CELEBI = 0xfb; // species index

    /// Clock chip value (seconds, minutes, hours, day low, day high).
    using Rtc = std::array<int, 5>;

    /// Joypad mask held on VBlank counts [start, end).
    struct Event
    {
        int start, end;
        std::uint8_t mask;
        bool operator==(const Event &) const = default;
    };
    using Events = std::vector<Event>;

    struct Press
    {
        int earliest; // first VBlank where an A press changes the future state
        std::optional<int> press; // prompts: the planned press (earliest + margin)
        std::optional<int> finalMin; // the final A: first VBlank that reaches the encounter
        bool operator==(const Press &) const = default;
    };

    /// A discovered plan: events 0-3 (boot A, CONTINUE, info box A, hold B), 4-9 the six prompts, 10 YES; then the final A.
    struct Plan
    {
        std::string lang;
        Events events;
        std::vector<Press> presses;
        int finalMin = 0;
    };

    /// Joypad by VBlank count: OR of the event masks over [max(start, 0), min(end, n)).
    Bytes build(const Events &events, int n = TIMELINE);
    /// The plan with the 6th prompt moved by d6, YES by dyes, and the final A at final (if given), held 5 VBlanks.
    Events planEvents(const Plan &p, int d6 = 0, int dyes = 0, std::optional<int> final = std::nullopt);
    /// The whole timeline shifted by X VBlanks (timer offset): j'[k] = j[k - X], 0 before the start, the last value after.
    Bytes shifted(const Bytes &j, int X);

    /// "hh:mm" -> minutes.
    int hm(const std::string &s);
    /// Chip clock for an in-game minute and a clock offset in minutes (the prototype's chip_rtc).
    Rtc chipRtc(int ingameMin, int offsetMin, int sec = 30, int day = 117);

    /// Power-on with the VC reset phases (the prototype's g2lib.boot).
    void boot(VcCore &c, const RomPtr &rom, const Bytes &sram, const Rtc &rtc, int divcd0 = 25, int counter0 = 88);
    struct RunResult
    {
        std::optional<std::array<std::uint8_t, 8>> enemy; // empty: no encounter within maxf
        int ret;
    };
    /// The prototype's g2lib.run: run a timeline until Celebi (+2 frames) or maxf frames.
    RunResult run(VcCore &c, const Bytes &j, int maxf = TIMELINE, int species = CELEBI);

    /// Discovers the press sequence (6 prompts + YES + final). Throws std::runtime_error when it can't.
    Plan discover(const RomPtr &rom, const Bytes &sram, const Rtc &rtc, int cont = 384, int info = 664);

    struct TimerRow
    {
        std::string press;
        int vbc, gbFrame, beep, phase;
    };
    /// GB frame (0-based, including frames without VBlank) at which each timed press is read, its beep and phase.
    std::vector<TimerRow> timer(const RomPtr &rom, const Bytes &sram, const Plan &p, int d6, int dyes, int final,
                                const Rtc &rtc = { 30, 12, 18, 117, 0 });
}

#endif // CELEBIPLAN_HPP
