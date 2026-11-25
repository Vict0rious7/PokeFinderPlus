/*
 * Pokémon Crystal VC: game-file inputs (see VcInputs.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "VcInputs.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace Gen2
{
    // ---------------------------------------------------------------- SHA-1 (FIPS 180-4)
    std::string sha1Hex(const std::uint8_t *p, std::size_t n)
    {
        std::uint32_t h[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
        auto rol = [](std::uint32_t x, int s) { return (x << s) | (x >> (32 - s)); };
        std::uint64_t bits = static_cast<std::uint64_t>(n) * 8;
        std::size_t total = ((n + 8) / 64 + 1) * 64;
        std::uint8_t block[64];
        for (std::size_t off = 0; off < total; off += 64)
        {
            for (int i = 0; i < 64; i++)
            {
                std::size_t k = off + i;
                block[i] = k < n ? p[k] : (k == n ? 0x80 : 0);
            }
            if (off + 64 == total)
            {
                for (int i = 0; i < 8; i++)
                {
                    block[56 + i] = static_cast<std::uint8_t>(bits >> (56 - 8 * i));
                }
            }
            std::uint32_t w[80];
            for (int i = 0; i < 16; i++)
            {
                w[i] = (block[4 * i] << 24) | (block[4 * i + 1] << 16) | (block[4 * i + 2] << 8) | block[4 * i + 3];
            }
            for (int i = 16; i < 80; i++)
            {
                w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
            }
            std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
            for (int i = 0; i < 80; i++)
            {
                std::uint32_t f, k;
                if (i < 20)
                {
                    f = (b & c) | (~b & d);
                    k = 0x5A827999;
                }
                else if (i < 40)
                {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                }
                else if (i < 60)
                {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                }
                else
                {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }
                std::uint32_t t = rol(a, 5) + f + e + k + w[i];
                e = d;
                d = c;
                c = rol(b, 30);
                b = a;
                a = t;
            }
            h[0] += a;
            h[1] += b;
            h[2] += c;
            h[3] += d;
            h[4] += e;
        }
        char out[41];
        for (int i = 0; i < 5; i++)
        {
            std::snprintf(out + 8 * i, 9, "%08x", h[i]);
        }
        return out;
    }

    std::optional<std::pair<std::string, std::string>> identifyDump(const Bytes &rom)
    {
        static const std::pair<const char *, std::pair<const char *, const char *>> CART[] = {
            { "c055992b16b739", { "fr", "FR cartridge" } },    { "accb584293ba", { "de", "DE cartridge" } },
            { "889a06fc0bb8", { "es", "ES cartridge" } },      { "6cee05e5b95b", { "it", "IT cartridge" } },
            { "f2f52230b536", { "en", "EN Rev A cartridge" } },
        };
        std::string h = sha1Hex(rom);
        for (const auto &[pre, v] : CART)
        {
            if (h.starts_with(pre))
            {
                return std::pair(std::string(v.first), std::string(v.second));
            }
        }
        return std::nullopt;
    }

    std::string ciaErrorText(CiaError e)
    {
        switch (e)
        {
        case CiaError::None:
            return "";
        case CiaError::NotCia:
            return "This file is not a CIA.";
        case CiaError::Encrypted:
            return "This CIA is encrypted, so its files can't be read. Dump your Virtual Console title again as a decrypted CIA "
                   "(GodMode9: \"Build CIA (decrypted)\"; a \".standard\" CIA can still be encrypted), or give the ROM and the "
                   "title's patch file, or your cartridge dump.";
        case CiaError::NoRom:
            return "This CIA holds no Pokémon Crystal ROM.";
        }
        return "";
    }

    namespace
    {
        std::uint32_t u32(const Bytes &c, std::size_t o)
        {
            if (o + 4 > c.size())
            {
                throw std::out_of_range("CIA truncated");
            }
            return c[o] | (c[o + 1] << 8) | (c[o + 2] << 16) | (static_cast<std::uint32_t>(c[o + 3]) << 24);
        }

        std::uint64_t u64(const Bytes &c, std::size_t o)
        {
            return u32(c, o) | (static_cast<std::uint64_t>(u32(c, o + 4)) << 32);
        }

        std::string utf16name(const Bytes &c, std::size_t o, std::size_t len)
        {
            std::string s;
            for (std::size_t i = 0; i + 1 < len + 1 && i < len; i += 2)
            {
                std::uint16_t ch = c[o + i] | (c[o + i + 1] << 8);
                s += ch < 0x80 ? static_cast<char>(ch) : '?'; // Crystal file names are ASCII
            }
            return s;
        }

        bool crystalFile(const std::string &name)
        {
            return name.starts_with("CGBBYT") || name.starts_with("CGBBXT");
        }
    }

    bool CiaTitle::sameEmulator() const
    {
        return error == CiaError::None && codeSha1 == VcRef::CODE && codeDecSha1 == VcRef::CODE_DEC && configSha1 == VcRef::CONFIG;
    }

    std::optional<Bytes> CiaTitle::dump() const
    {
        auto it = romfs.find(romName);
        return it == romfs.end() ? std::nullopt : std::optional<Bytes>(it->second);
    }

    std::optional<Bytes> CiaTitle::patchedRom() const
    {
        auto rom = dump();
        auto it = romfs.find(romName.substr(4) + ".patch");
        if (!rom || it == romfs.end())
        {
            return std::nullopt;
        }
        return applyPatch(*rom, std::string(it->second.begin(), it->second.end()));
    }

    CiaTitle readCia(const Bytes &c)
    {
        CiaTitle t;
        static const std::uint8_t MAGIC[4] = { 'N', 'C', 'C', 'H' };
        auto pos = std::search(c.begin(), c.end(), MAGIC, MAGIC + 4);
        if (pos == c.end() || pos - c.begin() < 0x100)
        {
            // a CIA whose content is encrypted with the title key shows no NCCH header
            bool cia = c.size() > 0x2040 && u32(c, 0) == 0x2020;
            t.error = cia ? CiaError::Encrypted : CiaError::NotCia;
            return t;
        }
        std::size_t n = static_cast<std::size_t>(pos - c.begin()) - 0x100;
        try
        {
            if (!(c.at(n + 0x188 + 7) & 4))
            {
                t.error = CiaError::Encrypted; // NCCH without the NoCrypto flag
                return t;
            }
            t.titleId = u64(c, n + 0x108);
            std::size_t e = n + static_cast<std::size_t>(u32(c, n + 0x1a0)) * 0x200;
            for (int i = 0; i < 10; i++)
            {
                std::string name(reinterpret_cast<const char *>(&c.at(e + i * 16)), 8);
                name = name.c_str();
                std::uint32_t o = u32(c, e + i * 16 + 8), s = u32(c, e + i * 16 + 12);
                if (name == ".code")
                {
                    Bytes code(c.begin() + e + 0x200 + o, c.begin() + e + 0x200 + o + s);
                    t.codeSha1 = sha1Hex(code);
                    try
                    {
                        t.codeDecSha1 = sha1Hex(blzDecompress(code));
                    }
                    catch (const std::exception &)
                    {
                        t.codeDecSha1 = t.codeSha1;
                    }
                }
            }
            std::size_t ro = u32(c, n + 0x1b0), base = n + ro * 0x200;
            std::uint32_t mhs = u32(c, base + 8), l3bs = u32(c, base + 0x4c);
            std::size_t bs = std::size_t(1) << l3bs;
            std::size_t l3 = base + ((0x60 + mhs + bs - 1) / bs) * bs;
            std::uint32_t dmo = u32(c, l3 + 12), fmo = u32(c, l3 + 28), fdo = u32(c, l3 + 36);
            std::vector<std::pair<std::uint32_t, std::string>> stack { { 0, "" } };
            int guard = 0;
            while (!stack.empty() && guard++ < 100000)
            {
                auto [off, path] = stack.back();
                stack.pop_back();
                std::size_t dm = l3 + dmo + off;
                std::uint32_t child = u32(c, dm + 8), fil = u32(c, dm + 12), nl = u32(c, dm + 20);
                std::string nm = utf16name(c, dm + 0x18, nl);
                std::string d = nm.empty() ? path : path + nm + "/";
                for (std::uint32_t f = fil; f != 0xffffffff;)
                {
                    std::size_t fm = l3 + fmo + f;
                    std::uint32_t sib = u32(c, fm + 4), nl2 = u32(c, fm + 28);
                    std::uint64_t doff = u64(c, fm + 8), dl = u64(c, fm + 16);
                    std::string fnm = utf16name(c, fm + 0x20, nl2);
                    std::size_t at = l3 + fdo + doff;
                    if (at + dl > c.size())
                    {
                        throw std::out_of_range("CIA truncated");
                    }
                    if (fnm == "config.ini")
                    {
                        t.configSha1 = sha1Hex(c.data() + at, dl);
                    }
                    if (crystalFile(fnm))
                    {
                        t.romfs[d + fnm] = Bytes(c.begin() + at, c.begin() + at + dl);
                        if (d == "rom/")
                        {
                            t.romName = d + fnm;
                        }
                    }
                    f = sib;
                }
                for (std::uint32_t ch = child; ch != 0xffffffff; ch = u32(c, l3 + dmo + ch + 4))
                {
                    stack.push_back({ ch, d });
                }
            }
        }
        catch (const std::exception &)
        {
            t.error = CiaError::NotCia;
            return t;
        }
        if (t.romName.empty())
        {
            t.error = CiaError::NoRom;
        }
        return t;
    }

    Bytes blzDecompress(const Bytes &c)
    {
        std::size_t n = c.size();
        if (n < 8)
        {
            throw std::runtime_error("too short");
        }
        std::uint32_t tb = u32(c, n - 8), delta = u32(c, n - 4);
        std::size_t dn = n + delta;
        Bytes d(dn, 0);
        std::copy(c.begin(), c.end(), d.begin());
        long index = static_cast<long>(n) - ((tb >> 24) & 0xFF);
        long stop = static_cast<long>(n) - (tb & 0xFFFFFF);
        long out = static_cast<long>(dn);
        if (index < 0 || stop < 0)
        {
            throw std::runtime_error("bad footer");
        }
        while (index > stop)
        {
            index -= 1;
            std::uint8_t flags = d[index];
            for (int i = 0; i < 8; i++)
            {
                if (index <= stop)
                {
                    break;
                }
                if (flags & 0x80)
                {
                    index -= 2;
                    int v = d[index] | (d[index + 1] << 8);
                    int ln = (v >> 12) + 3, disp = (v & 0xFFF) + 3;
                    for (int k = 0; k < ln; k++)
                    {
                        out -= 1;
                        if (out < 0 || out + disp >= static_cast<long>(dn))
                        {
                            throw std::runtime_error("bad data");
                        }
                        d[out] = d[out + disp];
                    }
                }
                else
                {
                    index -= 1;
                    out -= 1;
                    if (out < 0)
                    {
                        throw std::runtime_error("bad data");
                    }
                    d[out] = d[index];
                }
                flags = static_cast<std::uint8_t>(flags << 1);
            }
        }
        return d;
    }

    Bytes applyPatch(const Bytes &rom, const std::string &text, int *applied)
    {
        Bytes out = rom;
        int nApplied = 0;
        // sections start with '[' at the beginning of a line (the prototype's re.split(r'^\[', txt, flags=re.M))
        std::vector<std::string> secs;
        std::size_t pos = 0;
        std::vector<std::size_t> starts;
        for (std::size_t i = 0; i < text.size(); i++)
        {
            if (text[i] == '[' && (i == 0 || text[i - 1] == '\n' || text[i - 1] == '\r'))
            {
                starts.push_back(i + 1);
            }
        }
        for (std::size_t k = 0; k < starts.size(); k++)
        {
            std::size_t end = k + 1 < starts.size() ? starts[k + 1] - 1 : text.size();
            secs.push_back(text.substr(starts[k], end - starts[k]));
        }
        (void)pos;
        auto trim = [](std::string s) {
            auto a = s.find_first_not_of(" \t\r\n\f\v");
            auto b = s.find_last_not_of(" \t\r\n\f\v");
            return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
        };
        for (const auto &s : secs)
        {
            std::map<std::string, std::string> body;
            std::istringstream in(s);
            std::string line;
            std::getline(in, line); // "name]..."
            while (std::getline(in, line))
            {
                line = trim(line.substr(0, line.find(';')));
                auto eq = line.find('=');
                if (eq != std::string::npos)
                {
                    body[trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
                }
            }
            if (body["Mode"] != "1")
            {
                continue;
            }
            std::size_t addr = std::stoul(body["Address"], nullptr, 16);
            std::string fc = body["Fixcode"];
            Bytes data;
            if (!fc.empty() && (fc[0] == 'a' || fc[0] == 'A'))
            {
                auto colon = fc.find(':');
                int cnt = std::stoi(fc.substr(1, colon - 1));
                std::string hx;
                for (char ch : fc.substr(colon + 1))
                {
                    if (ch != ' ')
                    {
                        hx += ch;
                    }
                }
                for (std::size_t i = 0; i + 1 < hx.size() && static_cast<int>(data.size()) < cnt; i += 2)
                {
                    data.push_back(static_cast<std::uint8_t>(std::stoi(hx.substr(i, 2), nullptr, 16)));
                }
            }
            else
            {
                data.push_back(static_cast<std::uint8_t>(std::stoul(fc, nullptr, 16)));
            }
            if (addr + data.size() > out.size())
            {
                throw std::out_of_range("patch address outside the ROM");
            }
            std::copy(data.begin(), data.end(), out.begin() + addr);
            nApplied++;
        }
        if (applied)
        {
            *applied = nApplied;
        }
        return out;
    }
}
