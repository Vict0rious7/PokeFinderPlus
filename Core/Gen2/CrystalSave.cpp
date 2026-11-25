/*
 * Pokémon Crystal save tools (see CrystalSave.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CrystalSave.hpp"
#include <algorithm>
#include <stdexcept>

namespace Gen2
{
    namespace
    {
        struct Seg
        {
            int lo, hi, main, backup;
        };
        // saved WRAM segments: [lo, hi) -> offset of the main / backup copy in the save
        constexpr Seg SEG[] = { { 0xd47b, 0xdca5, 0x2009, 0x1209 }, { 0xdca5, 0xdcd7, 0x2833, 0x1a33 }, { 0xdcd7, 0xdff5, 0x2865, 0x1a65 } };
        constexpr int OPT_MAIN = 0x2000, OPT_BAK = 0x1200;
    }

    CrystalSave::CrystalSave(std::span<const std::uint8_t> data)
    {
        std::copy_n(data.begin(), std::min<std::size_t>(data.size(), SIZE), b.begin());
    }

    int CrystalSave::offset(int addr, bool backup)
    {
        for (const auto &s : SEG)
        {
            if (s.lo <= addr && addr < s.hi)
            {
                return (backup ? s.backup : s.main) + addr - s.lo;
            }
        }
        throw std::out_of_range("address outside the saved WRAM segments");
    }

    std::vector<std::uint8_t> CrystalSave::rd(int addr, int n) const
    {
        int o = offset(addr);
        return std::vector<std::uint8_t>(b.begin() + o, b.begin() + o + n);
    }

    std::uint8_t CrystalSave::rd8(int addr) const
    {
        return b[offset(addr)];
    }

    void CrystalSave::wr(int addr, std::span<const std::uint8_t> data)
    {
        for (bool bk : { false, true })
        {
            int o = offset(addr, bk);
            std::copy(data.begin(), data.end(), b.begin() + o);
        }
    }

    void CrystalSave::wr8(int addr, std::uint8_t v)
    {
        wr(addr, std::span<const std::uint8_t>(&v, 1));
    }

    std::uint8_t CrystalSave::opt(int i) const
    {
        return b[OPT_MAIN + i];
    }

    void CrystalSave::setOpt(int i, std::uint8_t v)
    {
        b[OPT_MAIN + i] = v;
        b[OPT_BAK + i] = v;
    }

    CrystalSave &CrystalSave::fix()
    {
        for (auto [start, cs] : { std::pair { 0x2009, 0x2d0d }, std::pair { 0x1209, 0x1f0d } })
        {
            unsigned c = 0;
            for (int i = start; i < start + 0xb7a; i++)
            {
                c += b[i];
            }
            c &= 0xffff;
            b[cs] = static_cast<std::uint8_t>(c & 0xff);
            b[cs + 1] = static_cast<std::uint8_t>(c >> 8);
        }
        return *this;
    }

    const std::vector<std::uint8_t> &CrystalSave::bytes()
    {
        fix();
        return b;
    }

    int kenjiClass(int v)
    {
        return v == 0 ? 0 : (v == 1 ? 1 : 2);
    }

    Minimal readMinimal(const CrystalSave &s)
    {
        Minimal m;
        std::uint8_t o = s.opt(0);
        auto ff = std::vector<std::uint8_t> { 0xff, 0xff };
        bool r1 = s.rd(0xdfd1, 2) != ff && s.rd8(0xdfcf) != 0;
        bool r2 = s.rd(0xdfd8, 2) != ff && s.rd8(0xdfd6) != 0;
        int sp = o & 7;
        m.textSpeed = sp == 0 ? TextSpeed::Instant : sp <= 1 ? TextSpeed::Fast : (sp <= 3 ? TextSpeed::Mid : TextSpeed::Slow);
        m.stereo = (o & 0x20) != 0;
        m.frame = s.opt(2) + 1;
        m.raikou = r1;
        m.entei = r2;
        m.lass = s.rd8(0xd4fe) != 0;
        m.swarm = (s.rd8(0xdc20) & 0x0c) != 0;
        m.F = s.rd8(Sym::wGameTimeFrames);
        m.playSec = s.rd8(Sym::wGameTimeSeconds);
        auto st = s.rd(Sym::wStartDay, 4);
        m.start = { st[0], st[1], st[2], st[3] };
        m.kenji = kenjiClass(s.rd8(Sym::wKenjiBreakTimer));
        m.setKenji = true;
        return m;
    }

    CrystalSave applyMinimal(const CrystalSave &tpl, const Minimal &m)
    {
        CrystalSave s(tpl.raw());
        static constexpr std::uint8_t SPEED[] = { 0, 1, 3, 5 }; // instant, fast, mid, slow
        s.setOpt(0, static_cast<std::uint8_t>((s.opt(0) & ~0x27) | (m.stereo ? 0x20 : 0) | SPEED[static_cast<int>(m.textSpeed)]));
        s.setOpt(2, static_cast<std::uint8_t>(m.frame - 1));
        static constexpr std::uint8_t RAIKOU[] = { 0xf3, 40, 24, 3, 100, 0, 0 }, ENTEI[] = { 0xf4, 40, 10, 1, 100, 0, 0 };
        static constexpr std::uint8_t NONE[] = { 0, 0, 0xff, 0xff, 0, 0, 0 };
        s.wr(0xdfcf, m.raikou ? RAIKOU : NONE);
        s.wr(0xdfd6, m.entei ? ENTEI : NONE);
        std::uint8_t sw = s.rd8(0xdc20) & ~0x0c;
        s.wr8(0xdc20, static_cast<std::uint8_t>(sw | (m.swarm ? 0x04 : 0)));
        s.wr8(Sym::wGameTimeFrames, static_cast<std::uint8_t>(m.F));
        s.wr8(Sym::wGameTimeSeconds, static_cast<std::uint8_t>(m.playSec));
        std::uint8_t st[4];
        for (int i = 0; i < 4; i++)
        {
            st[i] = static_cast<std::uint8_t>(m.start[i]);
        }
        s.wr(Sym::wStartDay, st);
        if (m.setKenji)
        {
            static constexpr std::uint8_t K[] = { 0, 1, 4 };
            s.wr8(Sym::wKenjiBreakTimer, K[m.kenji]);
        }
        if (!m.lass)
        {
            std::uint8_t zero[40] = {};
            s.wr(0xd4fe, zero);
            s.wr8(0xd71e + 16 * 7, 0xff);
        }
        return s;
    }

    std::string textSpeedName(TextSpeed t)
    {
        static const char *N[] = { "instant", "fast", "mid", "slow" };
        return N[static_cast<int>(t)];
    }
}
