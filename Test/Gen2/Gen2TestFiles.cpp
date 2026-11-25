/*
 * PokeFinderPlus: new file. Game files for the Gen 2 tests (see Gen2TestFiles.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "Gen2TestFiles.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <cstdio>
#include <cstdlib>
#include <mutex>

namespace Gen2Test
{
    namespace
    {
        std::map<std::string, std::shared_ptr<const Bytes>> &files(const char *env, std::size_t limit)
        {
            static std::map<std::string, std::map<std::string, std::shared_ptr<const Bytes>>> cache;
            static std::mutex m;
            std::lock_guard<std::mutex> lock(m);
            auto it = cache.find(env);
            if (it != cache.end())
            {
                return it->second;
            }
            auto &out = cache[env];
            const char *dir = std::getenv(env);
            if (!dir || !*dir)
            {
                return out;
            }
            QDir d(QString::fromLocal8Bit(dir));
            for (const QFileInfo &fi : d.entryInfoList(QDir::Files))
            {
                if (fi.size() > 4 * 1024 * 1024)
                {
                    continue;
                }
                QFile f(fi.absoluteFilePath());
                if (!f.open(QIODevice::ReadOnly))
                {
                    continue;
                }
                QByteArray b = f.readAll();
                auto bytes = std::make_shared<Bytes>(b.begin(), b.end());
                std::size_t n = std::min(limit, bytes->size());
                if (limit != SIZE_MAX)
                {
                    bytes->resize(n);
                }
                out[fnv64(bytes->data(), bytes->size())] = bytes;
            }
            return out;
        }

        std::vector<std::string> &skips()
        {
            static std::vector<std::string> s;
            return s;
        }
    }

    std::string fnv64(const std::uint8_t *p, std::size_t n)
    {
        std::uint64_t h = 1469598103934665603ULL;
        for (std::size_t i = 0; i < n; i++)
        {
            h ^= p[i];
            h *= 1099511628211ULL;
        }
        char buf[17];
        std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(h));
        return buf;
    }

    const json &golden(const std::string &name)
    {
        static std::map<std::string, json> cache;
        static std::mutex m;
        std::lock_guard<std::mutex> lock(m);
        auto it = cache.find(name);
        if (it != cache.end())
        {
            return it->second;
        }
        QFile file(QString::fromStdString(":/data/" + name));
        json data;
        if (file.open(QIODevice::ReadOnly))
        {
            data = json::parse(file.readAll().toStdString());
        }
        return cache[name] = data;
    }

    std::shared_ptr<const Bytes> rom(const std::string &fnvHex)
    {
        auto &f = files("POKEFINDERPLUS_GBC_ROMS", SIZE_MAX);
        auto it = f.find(fnvHex);
        return it == f.end() ? nullptr : it->second;
    }

    std::optional<Bytes> save(const std::string &fnvHex)
    {
        auto &f = files("POKEFINDERPLUS_GEN2_SAVES", 0x8000);
        auto it = f.find(fnvHex);
        if (it == f.end())
        {
            return std::nullopt;
        }
        return *it->second;
    }

    void countSkip(const std::string &what)
    {
        skips().push_back(what);
    }

    std::string summary()
    {
        if (skips().empty())
        {
            return "Gen 2: no test skipped";
        }
        std::string s = "Gen 2: " + std::to_string(skips().size())
            + " test(s) skipped (set POKEFINDERPLUS_GBC_ROMS / POKEFINDERPLUS_GEN2_SAVES to run them):";
        for (const auto &w : skips())
        {
            s += "\n  - " + w;
        }
        return s;
    }

    Bytes joyFromRle(const json &rle, int n)
    {
        Bytes j(n, 0);
        for (const auto &r : rle)
        {
            int a = r[0].get<int>(), b = r[1].get<int>();
            auto m = r[2].get<std::uint8_t>();
            for (int k = std::max(a, 0); k < std::min(b, n); k++)
            {
                j[k] |= m;
            }
        }
        return j;
    }

    Gen2::Minimal minimalFromJson(const json &j)
    {
        Gen2::Minimal m;
        static const std::map<std::string, Gen2::TextSpeed> TS = { { "instant", Gen2::TextSpeed::Instant },
                                                                   { "fast", Gen2::TextSpeed::Fast },
                                                                   { "mid", Gen2::TextSpeed::Mid },
                                                                   { "slow", Gen2::TextSpeed::Slow } };
        m.textSpeed = TS.at(j["textspeed"].get<std::string>());
        m.stereo = j["sound"].get<std::string>() == "stereo";
        m.frame = j["frame"].get<int>();
        m.raikou = j["raikou"].get<bool>();
        m.entei = j["entei"].get<bool>();
        m.lass = j["lass"].get<bool>();
        m.swarm = j["swarm"].get<bool>();
        m.F = j["F"].get<int>();
        m.playSec = j["playsec"].get<int>();
        for (int i = 0; i < 4; i++)
        {
            m.start[i] = j["start"][i].get<int>();
        }
        m.setKenji = j.contains("kenji");
        m.kenji = m.setKenji ? j["kenji"].get<int>() : 2;
        return m;
    }

    json minimalToJson(const Gen2::Minimal &m)
    {
        json j = { { "textspeed", Gen2::textSpeedName(m.textSpeed) },
                   { "sound", m.stereo ? "stereo" : "mono" },
                   { "frame", m.frame },
                   { "raikou", m.raikou },
                   { "entei", m.entei },
                   { "lass", m.lass },
                   { "swarm", m.swarm },
                   { "F", m.F },
                   { "playsec", m.playSec },
                   { "start", m.start } };
        if (m.setKenji)
        {
            j["kenji"] = m.kenji;
        }
        return j;
    }

    Gen2::Plan planFromJson(const json &j)
    {
        Gen2::Plan p;
        p.lang = j.value("lang", "");
        for (const auto &e : j["events"])
        {
            p.events.push_back({ e[0].get<int>(), e[1].get<int>(), e[2].get<std::uint8_t>() });
        }
        for (const auto &q : j["presses"])
        {
            Gen2::Press pr { q["earliest"].get<int>(), std::nullopt, std::nullopt };
            if (q.contains("press"))
            {
                pr.press = q["press"].get<int>();
            }
            if (q.contains("final_min"))
            {
                pr.finalMin = q["final_min"].get<int>();
            }
            p.presses.push_back(pr);
        }
        p.finalMin = j["final_min"].get<int>();
        return p;
    }

    json planToJson(const Gen2::Plan &p)
    {
        json j = { { "lang", p.lang }, { "final_min", p.finalMin }, { "events", json::array() }, { "presses", json::array() } };
        for (const auto &e : p.events)
        {
            j["events"].push_back({ e.start, e.end, e.mask });
        }
        for (const auto &q : p.presses)
        {
            json o = { { "earliest", q.earliest } };
            if (q.press)
            {
                o["press"] = *q.press;
            }
            if (q.finalMin)
            {
                o["final_min"] = *q.finalMin;
            }
            j["presses"].push_back(o);
        }
        return j;
    }

    Gen2::Rtc rtcFromJson(const json &r)
    {
        return { r[0].get<int>(), r[1].get<int>(), r[2].get<int>(), r[3].get<int>(), r[4].get<int>() };
    }

    Gen2::RomPtr langRom(const std::string &lang)
    {
        return rom(golden("gen2_core.json")["roms"][lang].get<std::string>());
    }

    std::optional<Bytes> namedSave(const std::string &name)
    {
        return save(golden("gen2_core.json")["saves"][name].get<std::string>());
    }
}
