/*
 * ORAS Trainer ID: base seed from a few new games launched at known times, then the SID / TSV of a new game.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 *
 * Method and rules come from BZL's 6genTidSearch (2020), ORAS tab, used with his permission:
 * BZL (なるほどえたきちのブログじゃねーの), https://blog.bzl-web.com/entry/2020/08/24/225424
 * The code follows 6genTidSearch's OrAsID (基準seed検索 and ズレ確認) and is checked against it (golden data produced by
 * running 6genTidSearch's own code, Test/Gen6/orastid_6gentidsearch.json).
 *
 * The model (BZL's article): with no save data, the TinyMT seed of a launch = a fixed value + the launch time in ms, so
 * two launches t ms apart have seeds t apart. TinyMT then advances 13 times, plus once per "No" at the name confirmation,
 * and the next output is the ID: TID = low 16 bits, SID = high 16 bits.
 */

#ifndef ORASTID_HPP
#define ORASTID_HPP

#include <Core/Gen6/XYTid.hpp>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace OrasTid
{
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;
    using s32 = std::int32_t;

    /// TinyMT advances before the ID with no "No" at the name confirmation (6genTidSearch: tidGen = 13)
    constexpr u32 ADVANCES_BEFORE_ID = 13;
    /// 6genTidSearch's default launch tolerance (許容誤差, ± ms)
    constexpr u32 DEFAULT_TOLERANCE = 500;

    /// TinyMT output that becomes the ID for a launch seed and a number of "No" at the name confirmation
    u32 idOutput(u32 seed, u32 no = 0);

    /// TID of a launch seed (no "No")
    inline u16 tid(u32 seed)
    {
        return static_cast<u16>(idOutput(seed));
    }

    /// One new game: when it was launched (ms after the first launch) and the TID it gave
    struct Launch
    {
        u32 ms;
        u16 tid;
    };

    /// One base seed that explains every launch, with the offset (ms) each launch needed
    struct BaseMatch
    {
        u32 seed;
        std::vector<s32> offsets; ///< one per launch, the first is always 0
    };

    /**
     * @brief Does \p base explain the launches? As 6genTidSearch's 基準seed検索.
     *
     * Launch 0 defines the base: its seed is \p base exactly. Launch k (k ≥ 1) matches if its TID comes from
     * base + ms_k + e with -tolerance ≤ e < tolerance (2 × tolerance values starting at -tolerance, so +tolerance itself is
     * out). Each launch's offset is the first e that matches.
     */
    std::optional<BaseMatch> matchBase(u32 base, std::span<const Launch> launches, u32 tolerance);

    /// One seed near the expected one that gives the TID seen
    struct Identified
    {
        u32 seed;
        s32 offset; ///< ms from the expected seed
        XYTid::Ids ids;
    };

    /**
     * @brief 6genTidSearch's ズレ確認: the seeds from expected - tolerance to expected + tolerance (both included) whose ID,
     * after \p no "No", has the TID seen. Each gives the SID, TSV and TRV.
     */
    std::vector<Identified> identify(u32 expected, u16 tidSeen, u32 no, u32 tolerance);
}

#endif // ORASTID_HPP
