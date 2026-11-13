/*
 * ORAS Trainer ID (see OrasTid.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 *
 * Method and rules: BZL's 6genTidSearch, ORAS tab, used with permission, https://blog.bzl-web.com/entry/2020/08/24/225424
 */

#include "OrasTid.hpp"

namespace OrasTid
{
    u32 idOutput(u32 seed, u32 no)
    {
        XYTid::Tiny mt = XYTid::Tiny::fromSeed(seed);
        mt.advance(ADVANCES_BEFORE_ID + no);
        return mt.next();
    }

    std::optional<BaseMatch> matchBase(u32 base, std::span<const Launch> launches, u32 tolerance)
    {
        if (launches.empty() || tid(base) != launches[0].tid)
        {
            return std::nullopt;
        }
        BaseMatch match { base, { 0 } };
        for (size_t k = 1; k < launches.size(); k++)
        {
            // 6genTidSearch: start = seconds * 1000 - tolerance, then 2 * tolerance seeds (unsigned arithmetic); the first
            // seed with the launch's TID is kept, and a launch with none rejects the base seed
            u32 start = base + launches[k].ms - tolerance;
            bool found = false;
            for (u32 j = 0; j < 2 * tolerance; j++)
            {
                if (tid(start + j) == launches[k].tid)
                {
                    match.offsets.push_back(static_cast<s32>(j) - static_cast<s32>(tolerance));
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                return std::nullopt;
            }
        }
        return match;
    }

    std::vector<Identified> identify(u32 expected, u16 tidSeen, u32 no, u32 tolerance)
    {
        std::vector<Identified> result;
        u32 seed = expected - tolerance;
        for (u64 i = 0; i <= 2ULL * tolerance; i++, seed++)
        {
            u32 r = idOutput(seed, no);
            if (static_cast<u16>(r) == tidSeen)
            {
                result.emplace_back(Identified { seed, static_cast<s32>(seed - expected), XYTid::ids(r) });
            }
        }
        return result;
    }
}
