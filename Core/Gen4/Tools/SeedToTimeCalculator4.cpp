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
 * Modified 2026-11-02 by Vict0rious7 for PokeFinderPlus (unofficial fork):
 * added the "Today" seed picker (parseSeedList, calculateTimesToday, rankToday); calculateTimes() now loops over
 * calculateDay(), its unchanged per-day body, so Today can scan one day.
 */

#include "SeedToTimeCalculator4.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>

namespace SeedToTimeCalculator4
{
    // PokeFinderPlus: the body of calculateTimes() for one day, so the Today picker can scan one day instead of a year
    static void calculateDay(std::vector<SeedTime4> &results, u32 seed, u16 year, u8 month, u8 day, bool forceSecond, u8 forcedSecond)
    {
        u8 ab = seed >> 24;
        u8 cd = (seed >> 16) & 0xFF;
        u32 efgh = seed & 0xFFFF;

        // Allow overflow seeds by setting hour to 23 and adjusting for delay
        u8 hour = cd > 23 ? 23 : cd;
        u32 delay = cd > 23 ? (efgh + (2000 - year)) + ((cd - 23) * 0x10000) : efgh + (2000 - year);

        for (u8 minute = 0; minute < 60; minute++)
        {
            for (u8 second = 0; second < 60; second++)
            {
                if (ab == ((month * day + minute + second) & 0xFF))
                {
                    if (!forceSecond || second == forcedSecond)
                    {
                        results.emplace_back(DateTime(year, month, day, hour, minute, second), delay);
                    }
                }
            }
        }
    }

    std::vector<SeedTime4> calculateTimes(u32 seed, u16 year, bool forceSecond, u8 forcedSecond)
    {
        std::vector<SeedTime4> results;
        for (u8 month = 1; month <= 12; month++)
        {
            u8 maxDays = Date::daysInMonth(year, month);
            for (u8 day = 1; day <= maxDays; day++)
            {
                calculateDay(results, seed, year, month, day, forceSecond, forcedSecond);
            }
        }
        return results;
    }

    std::vector<SeedTimeCalibrate4> calibrate(int delayCalibration, int secondCalibration, const SeedTime4 &target)
    {
        DateTime time = target.getDateTime();
        u32 delay = target.getDelay();

        std::vector<SeedTimeCalibrate4> results;
        results.reserve((2 * delayCalibration + 1) * (2 * secondCalibration + 1));
        for (int secondOffset = -secondCalibration; secondOffset <= secondCalibration; secondOffset++)
        {
            DateTime offset = time.addSeconds(secondOffset);
            if (!offset.valid())
            {
                continue;
            }

            for (int delayOffset = -delayCalibration; delayOffset <= delayCalibration; delayOffset++)
            {
                results.emplace_back(offset, delay + delayOffset);
            }
        }

        return results;
    }

    std::vector<SeedTimeCalibrate4> calibrate(int delayCalibration, int secondCalibration, u8 playerRoute,
                                              const std::array<bool, 3> &roamers, std::array<u8, 3> &routes, const SeedTime4 &target)
    {
        DateTime time = target.getDateTime();
        u32 delay = target.getDelay();

        std::vector<SeedTimeCalibrate4> results;
        results.reserve((2 * delayCalibration + 1) * (2 * secondCalibration + 1));
        for (int secondOffset = -secondCalibration; secondOffset <= secondCalibration; secondOffset++)
        {
            DateTime offset = time.addSeconds(secondOffset);
            if (!offset.valid())
            {
                continue;
            }

            for (int delayOffset = -delayCalibration; delayOffset <= delayCalibration; delayOffset++)
            {
                results.emplace_back(offset, delay + delayOffset, playerRoute, roamers, routes);
            }
        }

        return results;
    }

    // PokeFinderPlus: "Today" seed picker. Only filters and sorts what calculateDay() (calculateTimes() for one day) returns; no new seed
    // maths.
    std::vector<u32> parseSeedList(const std::string &text, u32 &rejected)
    {
        rejected = 0;
        std::vector<u32> seeds;
        std::unordered_set<u32> seen;

        std::istringstream lines(text);
        std::string line;
        while (std::getline(lines, line))
        {
            // First value on the line: up to a tab, space, comma or semicolon
            auto start = line.find_first_not_of(" \t\r,;");
            if (start == std::string::npos)
            {
                continue;
            }
            auto end = line.find_first_of(" \t\r,;", start);
            std::string token = line.substr(start, end == std::string::npos ? std::string::npos : end - start);

            if (token.size() > 2 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X'))
            {
                token = token.substr(2);
            }

            if (token.empty() || token.size() > 8
                || !std::all_of(token.begin(), token.end(), [](unsigned char c) { return std::isxdigit(c); }))
            {
                rejected++;
                continue;
            }

            u32 seed = std::stoul(token, nullptr, 16);
            if (seen.insert(seed).second)
            {
                seeds.emplace_back(seed);
            }
        }
        return seeds;
    }

    std::vector<SeedTime4> calculateTimesToday(u32 seed, const DateTime &now)
    {
        Date today = now.getDate();
        Time time = now.getTime();

        std::vector<SeedTime4> results;
        // calculateTimes() keeps the seed's hour (clamped to 23), so skip seeds whose hour has already passed
        u8 hour = std::min<u8>((seed >> 16) & 0xFF, 23);
        if (hour < time.hour())
        {
            return results;
        }

        std::vector<SeedTime4> day;
        calculateDay(day, seed, today.year(), today.month(), today.day(), false, 0);
        for (const auto &result : day)
        {
            Time t = result.getDateTime().getTime();
            if (t.hour() > time.hour() || (t.hour() == time.hour() && t.minute() > time.minute()))
            {
                results.emplace_back(result);
            }
        }
        return results;
    }

    std::vector<TodaySeed4> rankToday(const std::vector<u32> &seeds, const DateTime &now, int parity)
    {
        std::vector<TodaySeed4> ranked;
        for (u32 seed : seeds)
        {
            auto times = calculateTimesToday(seed, now);
            if (times.empty())
            {
                continue;
            }
            if (parity != -1 && static_cast<int>(times.front().getDelay() % 2) != parity)
            {
                continue;
            }
            ranked.emplace_back(TodaySeed4 { seed, std::move(times) });
        }

        std::stable_sort(ranked.begin(), ranked.end(), [](const TodaySeed4 &left, const TodaySeed4 &right) {
            u8 leftHour = left.times.front().getDateTime().getTime().hour();
            u8 rightHour = right.times.front().getDateTime().getTime().hour();
            if (leftHour != rightHour)
            {
                return leftHour < rightHour;
            }
            if (left.times.size() != right.times.size())
            {
                return left.times.size() > right.times.size();
            }
            return left.times.front().getDelay() < right.times.front().getDelay();
        });
        return ranked;
    }
}
