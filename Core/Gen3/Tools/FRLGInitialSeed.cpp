/*
 * This file is part of PokeFinderPlus, an unofficial fork of PokéFinder
 * Copyright (C) 2026 by Vict0rious7
 *
 * Port of Ten Lines' FRLG initial seed lookup (src/wasm/src/initial_seed.{hpp,cpp} and src/tenLines/index.ts)
 * by Lincoln-LM, https://github.com/Lincoln-LM/ten-lines, GPL-3.0.
 * FRLG seeds farmed by blisy, po, HunarPG, 10Ben, Real96, ColdStoneSys, Papa Jefé, and トノ.
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
 */

#include "FRLGInitialSeed.hpp"
#include <Core/RNG/LCRNG.hpp>
#include <Core/Resources/FRLGSeeds.hpp>
#include <Core/Util/Utilities.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <mutex>

namespace
{
    struct HeldButtonOffset
    {
        char button_mode;
        std::string held_button;
        s16 offset;
    };

    // Copied from Ten Lines' initial_seed.hpp (EU = ENG table + these offsets)
    const std::map<std::string, std::vector<HeldButtonOffset>> HELD_BUTTON_OFFSETS = {
        { "fr",
          {
              { 'a', "startup_select", -1 },
              { 'a', "startup_a", -8 },
              { 'a', "blackout_r", -23 },
              { 'a', "blackout_a", -31 },
              { 'a', "blackout_l", 2 },
              { 'a', "blackout_al", -33 },
              { 'a', "none", 0 },

              { 'h', "startup_select", 7 },
              { 'h', "startup_a", 3 },
              { 'h', "blackout_r", -23 },
              { 'h', "blackout_a", -23 },
              { 'h', "none", 0 },

              { 'r', "startup_select", -1 },
              { 'r', "startup_a", -18 },
              { 'r', "blackout_r", -23 },
              { 'r', "blackout_a", -39 },
              { 'r', "none", 0 },
          } },

        { "lg",
          {
              { 'a', "startup_select", -1 },
              { 'a', "startup_a", -8 },
              { 'a', "blackout_r", -23 },
              { 'a', "blackout_a", -31 },
              { 'a', "blackout_l", 2 },
              { 'a', "blackout_al", -33 },
              { 'a', "none", 0 },

              { 'h', "startup_select", 7 },
              { 'h', "startup_a", 3 },
              { 'h', "blackout_r", -23 },
              { 'h', "blackout_a", -23 },
              { 'h', "none", 0 },

              { 'r', "startup_select", -1 },
              { 'r', "startup_a", -18 },
              { 'r', "blackout_r", -23 },
              { 'r', "blackout_a", -39 },
              { 'r', "none", 0 },
          } },

        { "fr_eu",
          {
              { 'a', "startup_select", 8 },
              { 'a', "none", -1 },

              { 'h', "startup_select", 7 },
              { 'h', "none", 0 },

              { 'r', "startup_select", -9 },
              { 'r', "none", -8 },
          } },

        { "lg_eu",
          {
              { 'a', "startup_select", 8 },
              { 'a', "none", -1 },

              { 'h', "startup_select", 7 },
              { 'h', "none", 0 },

              { 'r', "startup_select", -9 },
              { 'r', "none", -8 },
          } },

        { "fr_jpn_1_0",
          {
              { 'a', "startup_select", -1 },
              { 'a', "startup_a", 1 },
              { 'a', "blackout_r", -10 },
              { 'a', "blackout_a", -18 },
              { 'a', "blackout_l", -3 },
              { 'a', "none", 0 },

              { 'h', "startup_select", 7 },
              { 'h', "startup_a", 3 },
              { 'h', "blackout_r", -27 },
              { 'h', "blackout_a", -24 },
              { 'h', "none", 0 },

              { 'r', "startup_select", 0 },
              { 'r', "startup_a", -18 },
              { 'r', "blackout_r", -23 },
              { 'r', "blackout_a", -40 },
              { 'r', "none", 0 },
          } },

        { "fr_jpn_1_1",
          {
              { 'a', "startup_select", 10 },
              { 'a', "startup_a", -9 },
              { 'a', "blackout_r", -23 },
              { 'a', "blackout_a", -31 },
              { 'a', "blackout_l", -6 },
              { 'a', "none", 0 },

              { 'h', "startup_select", -7 },
              { 'h', "startup_a", -19 },
              { 'h', "blackout_r", -21 },
              { 'h', "blackout_a", -29 },
              { 'h', "none", 0 },

              { 'r', "startup_select", -7 },
              { 'r', "startup_a", -4 },
              { 'r', "blackout_r", -29 },
              { 'r', "blackout_a", -38 },
              { 'r', "none", 0 },
          } },

        { "lg_jpn",
          {
              { 'a', "startup_select", -1 },
              { 'a', "startup_a", -9 },
              { 'a', "blackout_r", -22 },
              { 'a', "blackout_a", -40 },
              { 'a', "blackout_l", -7 },
              { 'a', "none", 0 },

              { 'h', "startup_select", -1 },
              { 'h', "startup_a", -18 },
              { 'h', "blackout_r", -23 },
              { 'h', "blackout_a", -31 },
              { 'h', "none", 0 },

              { 'r', "startup_select", -1 },
              { 'r', "startup_a", -23 },
              { 'r', "blackout_r", -23 },
              { 'r', "blackout_a", -39 },
              { 'r', "none", 0 },
          } },

        { "fr_mgba",
          {
              { 'a', "none", 0 },

              { 'h', "none", 0 },

              { 'r', "none", 0 },
          } },

        { "lg_mgba",
          {
              { 'a', "none", 0 },

              { 'h', "none", 0 },

              { 'r', "none", 0 },
          } },

        { "fr_nx",
          {
              { 'a', "none", 0 },

              { 'h', "none", 0 },
              { 'h', "blackout_r", -36 },
              { 'h', "blackout_l", -36 },

              { 'r', "none", 0 },
          } },

        { "fr_jpn_nx",
          {
              { 'a', "none", 0 },

              { 'h', "none", 0 },

              { 'r', "none", 0 },
          } },

        { "lg_nx",
          {
              { 'a', "none", 0 },

              { 'h', "none", 0 },
              { 'h', "blackout_r", -36 },
              { 'h', "blackout_l", -36 },

              { 'r', "none", 0 },
          } },

        { "lg_jpn_nx",
          {
              { 'a', "none", 0 },

              { 'h', "none", 0 },

              { 'r', "none", 0 },
          } },
    };

    struct FRLGSeedEntry
    {
        const std::string *key;
        char button_mode;
        u32 seedTime;
        u16 initialSeed;
    };

    // Ten Lines' FRLGSeedDataStore (seed_map only)
    struct FRLGSeedDataStore
    {
        std::vector<std::string> keys;
        std::map<u16, std::vector<FRLGSeedEntry>> seed_map;
        std::map<std::string, std::vector<FRLGSeedEntry>> contiguous_seeds;

        FRLGSeedDataStore(const std::vector<u8> &data, bool is_nx_format)
        {
            size_t ptr = 0;
            std::vector<u16> seed_times;
            auto read16 = [&](size_t p) { return static_cast<u16>(data[p] | (data[p + 1] << 8)); };
            auto read32
                = [&](size_t p) { return static_cast<u32>(data[p] | (data[p + 1] << 8) | (data[p + 2] << 16) | (data[p + 3] << 24)); };
            if (is_nx_format)
            {
                u32 seed_times_count = read32(ptr);
                ptr += 4;
                for (u32 i = 0; i < seed_times_count; i++, ptr += 2)
                {
                    seed_times.emplace_back(read16(ptr));
                }
            }

            // First pass: keys, so entries can point at stable strings
            struct Block
            {
                size_t start;
                u16 starting_frame;
                u8 frame_size;
                u32 count;
            };
            std::vector<Block> blocks;
            while (ptr < data.size())
            {
                const char *key = reinterpret_cast<const char *>(&data[ptr]);
                keys.emplace_back(key);
                ptr += std::strlen(key) + 1;
                Block block { 0, 0, 1, 0 };
                if (!is_nx_format)
                {
                    block.starting_frame = read16(ptr);
                    ptr += 2;
                    block.frame_size = data[ptr];
                    ptr += 1;
                }
                block.count = read32(ptr);
                ptr += 4;
                block.start = ptr;
                ptr += block.count * 3;
                blocks.emplace_back(block);
            }

            for (size_t k = 0; k < blocks.size(); k++)
            {
                const auto &block = blocks[k];
                const std::string *key = &keys[k];
                char button_mode = key->at(key->find('_') + 1);
                bool has_last = false;
                u16 last = 0;
                for (u32 i = 0; i < block.count; i++)
                {
                    size_t p = block.start + i * 3;
                    u16 seed = read16(p);
                    if (data[p + 2])
                    {
                        continue;
                    }
                    // seeds appearing consecutively can reasonably be condensed into their first entry
                    if (has_last && last == seed)
                    {
                        continue;
                    }
                    has_last = true;
                    last = seed;
                    u32 seed_time
                        = is_nx_format ? static_cast<u32>(seed_times[i]) + 5737 : (block.starting_frame + i / block.frame_size) * 16;
                    seed_map[seed].emplace_back(FRLGSeedEntry { key, button_mode, seed_time, seed });
                    contiguous_seeds[*key].emplace_back(FRLGSeedEntry { key, button_mode, seed_time, seed });
                }
            }
        }
    };

    std::string fileForVersion(const std::string &gameVersion)
    {
        static const std::map<std::string, std::string> files
            = { { "fr", "fr_eng" },           { "fr_eu", "fr_eng" },        { "lg", "lg_eng" },       { "lg_eu", "lg_eng" },
                { "fr_mgba", "fr_eng_mgba" }, { "lg_mgba", "lg_eng_mgba" }, { "fr_nx", "fr_eng_nx" }, { "lg_nx", "lg_eng_nx" } };
        auto it = files.find(gameVersion);
        return it == files.end() ? gameVersion : it->second;
    }

    const FRLGSeedDataStore *store(const std::string &gameVersion)
    {
        static std::mutex mutex;
        static std::map<std::string, FRLGSeedDataStore> stores;

        std::lock_guard<std::mutex> lock(mutex);
        std::string file = fileForVersion(gameVersion);
        auto it = stores.find(file);
        if (it != stores.end())
        {
            return &it->second;
        }

        for (const auto &embedded : FRLG_SEED_FILES)
        {
            if (file == embedded.name && embedded.size != 0)
            {
                u32 size;
                const u8 *raw = Utilities::decompress<u8>(embedded.data, embedded.size, size);
                std::vector<u8> data(raw, raw + size);
                delete[] raw;
                return &stores.emplace(file, FRLGSeedDataStore(data, gameVersion.ends_with("nx"))).first->second;
            }
        }
        return nullptr;
    }

    // Ten Lines' sorted_initial_seeds: (distance(seed, 0), seed) for every 16-bit seed, sorted by distance
    const std::vector<std::pair<u32, u16>> &sortedInitialSeeds()
    {
        static const std::vector<std::pair<u32, u16>> seeds = [] {
            std::vector<std::pair<u32, u16>> s;
            s.reserve(0x10000);
            for (u32 seed = 0; seed < 0x10000; seed++)
            {
                s.emplace_back(PokeRNG::distance(seed, 0), static_cast<u16>(seed));
            }
            std::sort(s.begin(), s.end());
            return s;
        }();
        return seeds;
    }

    u32 findClosestInitialSeedIndex(u32 target_seed)
    {
        const auto &sorted_initial_seeds = sortedInitialSeeds();
        u32 distance_from_base = PokeRNG::distance(0, target_seed);
        u32 target = 0xFFFFFFFF - distance_from_base;
        u32 left = 0;
        u32 right = sorted_initial_seeds.size() - 1;
        while (left < right)
        {
            u32 middle = (left + right) / 2;
            if (sorted_initial_seeds[middle].first <= target)
            {
                left = middle + 1;
            }
            else
            {
                right = middle;
            }
        }
        if (sorted_initial_seeds[left].first > target)
        {
            return left;
        }
        return 0;
    }

    bool matches(const std::string &key, const std::string &held, const FRLGSettingsFilter &filter)
    {
        // key = sound_buttonmode_button
        auto first = key.find('_');
        auto second = key.find('_', first + 1);
        return (filter.sound.empty() || key.compare(0, first, filter.sound) == 0 && filter.sound.size() == first)
            && (filter.buttonMode.empty() || key.substr(first + 1, second - first - 1) == filter.buttonMode)
            && (filter.button.empty() || key.substr(second + 1) == filter.button) && (filter.held.empty() || held == filter.held);
    }
}

namespace FRLGInitialSeed
{
    const std::vector<std::string> &gameVersions()
    {
        static const std::vector<std::string> versions = { "fr", "fr_eu", "fr_jpn_1_0", "fr_jpn_1_1", "fr_nx",     "fr_jpn_nx", "fr_mgba",
                                                           "lg", "lg_eu", "lg_jpn",     "lg_nx",      "lg_jpn_nx", "lg_mgba" };
        return versions;
    }

    const std::vector<std::string> &consoles()
    {
        static const std::vector<std::string> list = { "GBA", "GBP", "NDS", "3DS", "NX", "NX2" };
        return list;
    }

    bool hasData(const std::string &gameVersion)
    {
        return store(gameVersion) != nullptr;
    }

    std::string dataTimestamp()
    {
        return FRLG_SEEDS_TIMESTAMP;
    }

    std::vector<FRLGInitialSeedResult> find(u32 target, u16 count, u32 offset, const std::string &gameVersion, u32 ttvFramesOut,
                                            const FRLGSettingsFilter &filter)
    {
        std::vector<FRLGInitialSeedResult> results;
        const auto *seed_store = store(gameVersion);
        auto offsets = HELD_BUTTON_OFFSETS.find(gameVersion);
        if (seed_store == nullptr || offsets == HELD_BUTTON_OFFSETS.end() || count == 0)
        {
            return results;
        }

        // target_seed = PokeRNG(target_seed, -offset).getSeed()
        u32 target_seed = PokeRNG(target, static_cast<u32>(-static_cast<s64>(offset))).getSeed();
        const auto &sorted_initial_seeds = sortedInitialSeeds();
        u32 distance_from_base = PokeRNG::distance(0, target_seed);
        u32 result_index = findClosestInitialSeedIndex(target_seed);
        // PokeFinderPlus: bounded to one pass, since a settings filter can exclude everything
        for (u32 i = 0; i < 0x10000 && results.size() < count; i++)
        {
            auto [offset_advances, seed] = sorted_initial_seeds[(result_index + i) % sorted_initial_seeds.size()];
            u32 advances = offset_advances + distance_from_base;
            // if the advances required to reach this seed are less than the required frames in the overworld
            // then the target is unreachable
            if (advances < ttvFramesOut)
            {
                continue;
            }
            for (const auto &held_button_offset : offsets->second)
            {
                u16 unoffset_seed = seed - held_button_offset.offset;
                auto it = seed_store->seed_map.find(unoffset_seed);
                // no known way to achieve this seed in frlg
                if (it == seed_store->seed_map.end())
                {
                    continue;
                }
                for (const auto &entry : it->second)
                {
                    // the offset needed to achieve this seed only happens with a different button mode
                    if (entry.button_mode != held_button_offset.button_mode || !matches(*entry.key, held_button_offset.held_button, filter))
                    {
                        continue;
                    }
                    results.emplace_back(
                        FRLGInitialSeedResult { advances, entry.seedTime, seed, *entry.key + "_" + held_button_offset.held_button });
                    if (results.size() >= count)
                    {
                        break;
                    }
                }
                if (results.size() >= count)
                {
                    break;
                }
            }
        }
        return results;
    }

    std::vector<FRLGSeedTime> contiguousSeeds(const std::string &gameVersion, const std::string &settings)
    {
        // Ten Lines' get_contiguous_seed_list: the farmed seeds of one setting in frame order, shifted by the extra button's offset
        std::vector<FRLGSeedTime> entries;
        const auto *seed_store = store(gameVersion);
        auto offsets = HELD_BUTTON_OFFSETS.find(gameVersion);
        size_t first = settings.find('_');
        size_t second = first == std::string::npos ? first : settings.find('_', first + 1);
        size_t third = second == std::string::npos ? second : settings.find('_', second + 1);
        if (seed_store == nullptr || offsets == HELD_BUTTON_OFFSETS.end() || third == std::string::npos)
        {
            return entries;
        }
        std::string setting_key = settings.substr(0, third);
        std::string held_button = settings.substr(third + 1);
        char button_mode = settings[first + 1];

        auto seeds_iter = seed_store->contiguous_seeds.find(setting_key);
        if (seeds_iter == seed_store->contiguous_seeds.end())
        {
            return entries;
        }
        auto held_button_offset = std::find_if(offsets->second.begin(), offsets->second.end(), [&](const HeldButtonOffset &offset) {
            return offset.held_button == held_button && offset.button_mode == button_mode;
        });
        if (held_button_offset == offsets->second.end())
        {
            return entries;
        }
        for (const auto &seed : seeds_iter->second)
        {
            entries.emplace_back(FRLGSeedTime { static_cast<u16>(seed.initialSeed + held_button_offset->offset), seed.seedTime });
        }
        return entries;
    }

    std::vector<FRLGSeedTime> neighbourSeeds(const std::string &gameVersion, const std::string &settings, u16 seed, u32 leeway)
    {
        // Ten Lines' calibration: the target seed and up to leeway seeds before and after it in the farmed order
        auto list = contiguousSeeds(gameVersion, settings);
        auto it = std::find_if(list.begin(), list.end(), [seed](const FRLGSeedTime &entry) { return entry.initialSeed == seed; });
        if (it == list.end())
        {
            return { };
        }
        size_t index = std::distance(list.begin(), it);
        size_t start = index > leeway ? index - leeway : 0;
        size_t end = std::min(list.size(), index + leeway + 1);
        return std::vector<FRLGSeedTime>(list.begin() + start, list.begin() + end);
    }

    int frameToMS(double frame, const std::string &console)
    {
        struct Timing
        {
            double frame_rate;
            int offset_ms;
        };
        static const std::map<std::string, Timing> timings
            = { { "Generic", { 16777216.0 / 280896, 0 } }, { "GBA", { 16777216.0 / 280896, -260 } },
                { "GBP", { 16777216.0 / 280896, 200 } },   { "NDS", { 16756991.0 / 280896, 788 } },
                { "3DS", { 16756991.0 / 280896, 1558 } },  { "NX", { 16777216.0 / 280896, 0 } },
                { "NX2", { 16777216.0 / 280896, -750 } } };
        auto it = timings.find(console);
        const Timing &timing = it == timings.end() ? timings.at("Generic") : it->second;
        return static_cast<int>(std::floor(frame / timing.frame_rate * 1000)) + timing.offset_ms;
    }

    void teachyTVConversion(u32 advances, u32 minimumOut, u32 &ttvAdvances, u32 &regularAdvances)
    {
        s64 target_advances_via_ttv = static_cast<s64>(advances) - minimumOut;
        s64 ttv = static_cast<s64>(std::floor(target_advances_via_ttv / 313.0));
        ttvAdvances = static_cast<u32>(ttv);
        regularAdvances = static_cast<u32>(advances - ttv * 313);
    }

    std::string humanizeSettings(const std::string &settings)
    {
        static const std::map<std::string, std::string> terms
            = { { "stereo", "Stereo" }, { "mono", "Mono" }, { "start", "Start" },     { "select", "Select" },     { "a", "A" },
                { "l", "L" },           { "r", "R" },       { "startup", "Startup" }, { "blackout", "Blackout" }, { "al", "A+L" },
                { "none", "None" } };
        static const std::map<std::string, std::string> modes = { { "a", "L=A" }, { "h", "Help" }, { "r", "LR" } };

        std::vector<std::string> parts;
        size_t start = 0;
        while (true)
        {
            size_t end = settings.find('_', start);
            parts.emplace_back(settings.substr(start, end == std::string::npos ? std::string::npos : end - start));
            if (end == std::string::npos)
            {
                break;
            }
            start = end + 1;
        }
        parts.resize(5);
        auto term = [&](const std::string &s) {
            auto it = terms.find(s);
            return it == terms.end() ? std::string() : it->second;
        };
        auto mode = modes.find(parts[1]);
        return term(parts[0]) + " | " + (mode == modes.end() ? std::string() : mode->second) + " | Seed Button: " + term(parts[2])
            + " | Extra Button: " + term(parts[3]) + " " + term(parts[4]);
    }
}
