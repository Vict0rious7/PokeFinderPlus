/*
 * GBC core with the 3DS Virtual Console timing (see VcCore.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 *
 * Line-by-line port of the prototype's vccore.c. Integer types and evaluation order are kept on purpose (including the
 * prototype's DAA and timer arithmetic): the goal is the same emulated state as the prototype, byte for byte. The
 * prototype's tracing hooks (breakpoint trace, instruction log, VBlank log) are left out; they never change the state.
 */

#include "VcCore.hpp"
#include <algorithm>
#include <cstring>

namespace Gen2
{
    namespace
    {
        constexpr int HDMA_COST = 8; // 0x22f690
        constexpr int LCDINTONCE = 0; // PPU +0x64
        constexpr int LCDONDELAY = 0; // PPU +0x6c (not loaded for CGB)
        constexpr std::uint8_t TMASK[4] = { 0xff, 0x03, 0x0f, 0x3f };
        constexpr int TPER[4] = { 256, 4, 16, 64 };

        constexpr std::uint8_t FZ = 0x80;
        constexpr std::uint8_t FN = 0x40;
        constexpr std::uint8_t FH = 0x20;
        constexpr std::uint8_t FC = 0x10;
    }

    VcCore::VcCore()
    {
        reset();
    }

    void VcCore::loadRom(std::shared_ptr<const std::vector<u8>> r)
    {
        romData = std::move(r);
        rom = romData->data();
        romsize = static_cast<int>(romData->size());
    }

    void VcCore::loadSram(std::span<const u8> data)
    {
        std::memcpy(s.sram, data.data(), std::min<std::size_t>(data.size(), 0x8000));
    }

    void VcCore::setRtc(int sec, int m, int h, int dl, int dh)
    {
        s.rtcCur[0] = static_cast<u8>(sec);
        s.rtcCur[1] = static_cast<u8>(m);
        s.rtcCur[2] = static_cast<u8>(h);
        s.rtcCur[3] = static_cast<u8>(dl);
        s.rtcCur[4] = static_cast<u8>(dh);
    }

    void VcCore::setJoy(u8 j)
    {
        s.joy = j;
    }

    // ---------------------------------------------------------------- memory access
    VcCore::u8 VcCore::rd(u16 a) const
    {
        switch (a >> 12)
        {
        case 0:
        case 1:
        case 2:
        case 3:
            return rom[a];
        case 4:
        case 5:
        case 6:
        case 7:
            return rom[((s.rombank * 0x4000) + (a - 0x4000)) % romsize];
        case 8:
        case 9:
            return s.vram[s.vbk][a - 0x8000];
        case 0xA:
        case 0xB:
            if (s.rambank <= 3)
            {
                return s.sram[s.rambank][a - 0xA000];
            }
            if (s.rambank >= 8 && s.rambank <= 0xC)
            {
                return s.rtcCur[s.rambank - 8];
            }
            return 0xff;
        case 0xC:
            return s.wram[0][a - 0xC000];
        case 0xD:
            return s.wram[s.svbk][a - 0xD000];
        case 0xE:
            return s.wram[0][a - 0xE000];
        default:
            if (a < 0xFE00)
            {
                return s.wram[s.svbk][a - 0xF000];
            }
            if (a < 0xFEA0)
            {
                return s.oam[a - 0xFE00];
            }
            if (a < 0xFF00)
            {
                return 0xff;
            }
            return s.io[a & 0xff];
        }
    }

    void VcCore::lycCheck() // 0x169274
    {
        if (s.io[0x44] == s.io[0x45])
        {
            s.io[0x41] |= 4;
            if (s.io[0x41] & 0x40)
            {
                s.f68 = LCDINTONCE;
                s.io[0x0f] |= 2;
            }
        }
        else
        {
            s.f68 = 0;
            s.io[0x41] &= ~4;
        }
    }

    void VcCore::hdmaBlock() // 0x16a084: one HBlank block
    {
        u16 src = static_cast<u16>((s.io[0x51] << 8) | s.io[0x52]), dst = static_cast<u16>((s.io[0x53] << 8) | s.io[0x54]);
        src &= 0xfff0;
        dst = static_cast<u16>(dst & 0x1ff0);
        for (int i = 0; i < 16; i++)
        {
            s.vram[s.vbk][(dst + i) & 0x1fff] = rd(static_cast<u16>(src + i));
        }
        s.pending = HDMA_COST + 0 + 1;
        if ((s.io[0x55] & 0x7f) == 0)
        {
            s.io[0x51] = 0xff;
            s.io[0x52] = 0xf0;
            s.io[0x53] = 0x9f;
            s.io[0x54] = 0xf0;
            s.io[0x55] = 0xff;
            s.hdmaActive = 0;
        }
        else
        {
            u16 s2 = static_cast<u16>(src + 16), d2 = static_cast<u16>(dst + 16);
            if (s2 == 0x8000)
            {
                s2 = 0xa000;
            }
            if (d2 == 0x2000)
            {
                d2 = 0;
            }
            s.io[0x51] = static_cast<u8>(s2 >> 8);
            s.io[0x52] = static_cast<u8>(s2 & 0xf0);
            s.io[0x53] = static_cast<u8>((d2 >> 8) & 0x1f);
            s.io[0x54] = static_cast<u8>(d2 & 0xf0);
            s.io[0x55] = static_cast<u8>((s.io[0x55] & 0x80) | ((s.io[0x55] - 1) & 0x7f));
        }
    }

    void VcCore::gdma() // 0x169f1c
    {
        u16 src = static_cast<u16>(((s.io[0x51] << 8) | s.io[0x52]) & 0xfff0);
        u16 dst = static_cast<u16>(((s.io[0x53] << 8) | s.io[0x54]) & 0x1ff0);
        int n = (s.io[0x55] & 0x7f) + 1, len = n * 16;
        for (int i = 0; i < len; i++)
        {
            s.vram[s.vbk][(dst + i) & 0x1fff] = rd(static_cast<u16>(src + i));
        }
        s.pending = 1 + 0 + n * HDMA_COST;
        u16 s2 = static_cast<u16>(src + len), d2 = static_cast<u16>(dst + len);
        if (s2 == 0x8000)
        {
            s2 = 0xa000;
        }
        if (d2 == 0x2000)
        {
            d2 = 0;
        }
        s.io[0x51] = static_cast<u8>(s2 >> 8);
        s.io[0x52] = static_cast<u8>(s2 & 0xf0);
        s.io[0x53] = static_cast<u8>((d2 >> 8) & 0x1f);
        s.io[0x54] = static_cast<u8>(d2 & 0xf0);
        s.io[0x55] = 0xff;
        s.hdmaActive = 0;
    }

    void VcCore::lcdcChange(u8 old) // 0x16a43c
    {
        u8 x = old ^ s.io[0x40];
        if (x & 0x80)
        {
            if (s.io[0x40] & 0x80) // LCD on
            {
                s.io[0x44] = 0;
                s.S = LCDONDELAY + 3;
                s.io[0x41] = static_cast<u8>((s.io[0x41] & ~3) | 2);
                s.linecd = s.M2;
                if (s.io[0x41] & 0x20)
                {
                    s.io[0x0f] |= 2;
                }
                lycCheck();
            }
            else // LCD off
            {
                s.S = 2;
                s.f68 = 0;
                s.io[0x41] &= ~3;
                s.linecd = s.LINE;
            }
        }
    }

    void VcCore::p1Update() // 0x196090
    {
        u8 v = s.io[0];
        u8 btn = static_cast<u8>((~s.joy) & 0x0f), dir = static_cast<u8>(((~s.joy) >> 4) & 0x0f);
        if (!(v & 0x20))
        {
            v = static_cast<u8>((v & 0xf0) | btn);
        }
        else if (!(v & 0x10))
        {
            v = static_cast<u8>((v & 0xf0) | dir);
        }
        else
        {
            v |= 0x0f;
        }
        s.io[0] = v;
    }

    void VcCore::ioWr(u8 r, u8 v)
    {
        u8 old = s.io[r];
        switch (r)
        {
        case 0x00:
            s.io[0] = static_cast<u8>((v & 0x30) | 0xc0);
            p1Update();
            return;
        case 0x02:
            s.io[2] = v;
            if (v & 0x80)
            {
                s.serialWarn++;
            }
            return;
        case 0x04:
            s.io[4] = 0;
            s.divcd = 0x40;
            s.counter = 0xff;
            return;
        case 0x07:
            s.io[7] = v & 7;
            if (v & 4)
            {
                i32 t = (s.counter & TMASK[v & 3]) + 1;
                if (s.budget > t)
                {
                    s.budget = t;
                }
            }
            return;
        case 0x0f:
            s.io[0x0f] = v & 0x1f;
            return;
        case 0x40:
            s.io[0x40] = v;
            if (v != old)
            {
                lcdcChange(old);
            }
            return;
        case 0x41:
            s.io[0x41] = static_cast<u8>((old & 0x87) | (v & 0x78));
            s.io[0x41] &= ~4;
            return;
        case 0x44:
            return;
        case 0x45:
            s.io[0x45] = v;
            if (v != old && (s.io[0x40] & 0x80))
            {
                if (s.io[0x44] != s.io[0x45])
                {
                    s.f68 = 0;
                    s.io[0x41] &= ~4;
                }
                else
                {
                    s.io[0x41] |= 4;
                    if (s.io[0x41] & 0x40)
                    {
                        s.f68 = 1;
                        s.io[0x0f] |= 2;
                    }
                }
            }
            return;
        case 0x46:
            s.io[0x46] = v;
            for (int i = 0; i < 0xa0; i++)
            {
                s.oam[i] = rd(static_cast<u16>((v << 8) + i));
            }
            return;
        case 0x4d:
            s.io[0x4d] = static_cast<u8>((old & 0x80) | (v & 1));
            return;
        case 0x4f:
            s.io[0x4f] = v | 0xfe;
            s.vbk = v & 1;
            return;
        case 0x51:
            s.io[0x51] = v;
            return;
        case 0x52:
            s.io[0x52] = v & 0xf0;
            return;
        case 0x53:
            s.io[0x53] = v & 0x1f;
            return;
        case 0x54:
            s.io[0x54] = v & 0xf0;
            return;
        case 0x55:
            if (!s.hdmaActive)
            {
                if (v & 0x80)
                {
                    s.io[0x55] = v & 0x7f;
                    s.hdmaActive = 1;
                    if ((s.io[0x41] & 3) == 0)
                    {
                        hdmaBlock();
                    }
                }
                else
                {
                    s.io[0x55] = v;
                    gdma();
                }
            }
            else
            {
                if (v & 0x80)
                {
                    s.io[0x55] = v & 0x7f;
                }
                else
                {
                    s.io[0x55] = 0xff;
                    s.hdmaActive = 0;
                }
            }
            return;
        case 0x70:
            s.io[0x70] = v | 0xf8;
            s.svbk = (v & 7) ? (v & 7) : 1;
            return;
        case 0xff:
            s.io[0xff] = v & 0x1f;
            s.budget = 0;
            return;
        default:
            s.io[r] = v;
            return;
        }
    }

    void VcCore::wr(u16 a, u8 v)
    {
        switch (a >> 12)
        {
        case 0:
        case 1:
            s.ramen = ((v & 0x0a) == 0x0a);
            return;
        case 2:
        case 3:
            s.rombank = v & 0x7f;
            if (!s.rombank)
            {
                s.rombank = 1;
            }
            return;
        case 4:
        case 5:
            s.rambank = v & 0x0f;
            return;
        case 6:
        case 7:
            if (s.latchprev == 0 && (v & 1))
            {
                std::memcpy(s.rtcLat, s.rtcCur, 5);
            }
            s.latchprev = v & 1;
            return;
        case 8:
        case 9:
            s.vram[s.vbk][a - 0x8000] = v;
            return;
        case 0xA:
        case 0xB:
            if (s.rambank <= 3)
            {
                s.sram[s.rambank][a - 0xA000] = v;
            }
            else if (s.rambank >= 8 && s.rambank <= 0xC)
            {
                s.rtcCur[s.rambank - 8] = v;
                s.rtcLat[s.rambank - 8] = v;
            }
            return;
        case 0xC:
            s.wram[0][a - 0xC000] = v;
            return;
        case 0xD:
            s.wram[s.svbk][a - 0xD000] = v;
            return;
        case 0xE:
            s.wram[0][a - 0xE000] = v;
            return;
        default:
            if (a < 0xFE00)
            {
                s.wram[s.svbk][a - 0xF000] = v;
                return;
            }
            if (a < 0xFEA0)
            {
                s.oam[a - 0xFE00] = v;
                return;
            }
            if (a < 0xFF00)
            {
                return;
            }
            if (a >= 0xFF80 && a != 0xFFFF)
            {
                s.io[a & 0xff] = v;
                return;
            }
            ioWr(static_cast<u8>(a & 0xff), v);
            return;
        }
    }

    // ---------------------------------------------------------------- CPU core (SM83, VC/GB M-cycle counts)
    VcCore::u8 VcCore::fetch()
    {
        return rd(s.PC++);
    }

    VcCore::u16 VcCore::fetch16()
    {
        u8 lo = fetch();
        u8 hi = fetch();
        return static_cast<u16>(lo | (hi << 8));
    }

    void VcCore::push16(u16 v)
    {
        s.SP--;
        wr(s.SP, static_cast<u8>(v >> 8));
        s.SP--;
        wr(s.SP, static_cast<u8>(v & 0xff));
    }

    VcCore::u16 VcCore::pop16()
    {
        u8 lo = rd(s.SP++);
        u8 hi = rd(s.SP++);
        return static_cast<u16>(lo | (hi << 8));
    }

#define BC ((s.B << 8) | s.C)
#define DE ((s.D << 8) | s.E)
#define HL ((s.H << 8) | s.L)
#define SETBC(v)                                                                                                                           \
    do                                                                                                                                     \
    {                                                                                                                                      \
        u16 _v = static_cast<u16>(v);                                                                                                      \
        s.B = static_cast<u8>(_v >> 8);                                                                                                    \
        s.C = static_cast<u8>(_v & 0xff);                                                                                                  \
    } while (0)
#define SETDE(v)                                                                                                                           \
    do                                                                                                                                     \
    {                                                                                                                                      \
        u16 _v = static_cast<u16>(v);                                                                                                      \
        s.D = static_cast<u8>(_v >> 8);                                                                                                    \
        s.E = static_cast<u8>(_v & 0xff);                                                                                                  \
    } while (0)
#define SETHL(v)                                                                                                                           \
    do                                                                                                                                     \
    {                                                                                                                                      \
        u16 _v = static_cast<u16>(v);                                                                                                      \
        s.H = static_cast<u8>(_v >> 8);                                                                                                    \
        s.L = static_cast<u8>(_v & 0xff);                                                                                                  \
    } while (0)

    VcCore::u8 VcCore::getr(int i)
    {
        switch (i)
        {
        case 0:
            return s.B;
        case 1:
            return s.C;
        case 2:
            return s.D;
        case 3:
            return s.E;
        case 4:
            return s.H;
        case 5:
            return s.L;
        case 6:
            return rd(static_cast<u16>(HL));
        default:
            return s.A;
        }
    }

    void VcCore::setr(int i, u8 v)
    {
        switch (i)
        {
        case 0:
            s.B = v;
            break;
        case 1:
            s.C = v;
            break;
        case 2:
            s.D = v;
            break;
        case 3:
            s.E = v;
            break;
        case 4:
            s.H = v;
            break;
        case 5:
            s.L = v;
            break;
        case 6:
            wr(static_cast<u16>(HL), v);
            break;
        default:
            s.A = v;
        }
    }

    void VcCore::alu(int op, u8 v)
    {
        int r;
        u8 c = (s.F & FC) ? 1 : 0;
        u8 &A = s.A;
        u8 &F = s.F;
        switch (op)
        {
        case 0:
            r = A + v;
            F = static_cast<u8>(((r & 0xff) ? 0 : FZ) | (((A & 0xf) + (v & 0xf)) > 0xf ? FH : 0) | (r > 0xff ? FC : 0));
            A = static_cast<u8>(r);
            break;
        case 1:
            r = A + v + c;
            F = static_cast<u8>(((r & 0xff) ? 0 : FZ) | (((A & 0xf) + (v & 0xf) + c) > 0xf ? FH : 0) | (r > 0xff ? FC : 0));
            A = static_cast<u8>(r);
            break;
        case 2:
            r = A - v;
            F = static_cast<u8>(FN | ((r & 0xff) ? 0 : FZ) | (((A & 0xf) < (v & 0xf)) ? FH : 0) | (r < 0 ? FC : 0));
            A = static_cast<u8>(r);
            break;
        case 3:
            r = A - v - c;
            F = static_cast<u8>(FN | ((r & 0xff) ? 0 : FZ) | (((A & 0xf) - (v & 0xf) - c) < 0 ? FH : 0) | (r < 0 ? FC : 0));
            A = static_cast<u8>(r);
            break;
        case 4:
            A &= v;
            F = static_cast<u8>((A ? 0 : FZ) | FH);
            break;
        case 5:
            A ^= v;
            F = (A ? 0 : FZ);
            break;
        case 6:
            A |= v;
            F = (A ? 0 : FZ);
            break;
        case 7:
            r = A - v;
            F = static_cast<u8>(FN | ((r & 0xff) ? 0 : FZ) | (((A & 0xf) < (v & 0xf)) ? FH : 0) | (r < 0 ? FC : 0));
            break;
        }
    }

    int VcCore::cond(int cc) const
    {
        switch (cc)
        {
        case 0:
            return !(s.F & FZ);
        case 1:
            return (s.F & FZ) != 0;
        case 2:
            return !(s.F & FC);
        default:
            return (s.F & FC) != 0;
        }
    }

    int VcCore::cb()
    {
        u8 op = fetch();
        int r = op & 7;
        u8 v = getr(r);
        int x = op >> 6, y = (op >> 3) & 7;
        u8 res = v;
        if (x == 0)
        {
            u8 c;
            switch (y)
            {
            case 0:
                c = v >> 7;
                res = static_cast<u8>((v << 1) | c);
                break;
            case 1:
                c = v & 1;
                res = static_cast<u8>((v >> 1) | (c << 7));
                break;
            case 2:
                c = v >> 7;
                res = static_cast<u8>((v << 1) | ((s.F & FC) ? 1 : 0));
                break;
            case 3:
                c = v & 1;
                res = static_cast<u8>((v >> 1) | ((s.F & FC) ? 0x80 : 0));
                break;
            case 4:
                c = v >> 7;
                res = static_cast<u8>(v << 1);
                break;
            case 5:
                c = v & 1;
                res = static_cast<u8>((v >> 1) | (v & 0x80));
                break;
            case 6:
                c = 0;
                res = static_cast<u8>((v >> 4) | (v << 4));
                break;
            default:
                c = v & 1;
                res = v >> 1;
                break;
            }
            s.F = static_cast<u8>((res ? 0 : FZ) | (c ? FC : 0));
            setr(r, res);
            return r == 6 ? 4 : 2;
        }
        else if (x == 1)
        {
            s.F = static_cast<u8>((s.F & FC) | FH | ((v >> y) & 1 ? 0 : FZ));
            return r == 6 ? 3 : 2;
        }
        else if (x == 2)
        {
            setr(r, static_cast<u8>(v & ~(1 << y)));
            return r == 6 ? 4 : 2;
        }
        else
        {
            setr(r, static_cast<u8>(v | (1 << y)));
            return r == 6 ? 4 : 2;
        }
    }

    // executes one instruction; returns M-cycles to add (or -1 when the handler already set cycles)
    int VcCore::step()
    {
        if (s.PC == 0x02c5)
        {
            s.vbcount++;
        }
        u8 op = fetch();
        u8 t8;
        u16 t16;
        int r;
        u8 &A = s.A;
        u8 &F = s.F;
        switch (op)
        {
        case 0x00:
            return 1;
        case 0x01:
            SETBC(fetch16());
            return 3;
        case 0x11:
            SETDE(fetch16());
            return 3;
        case 0x21:
            SETHL(fetch16());
            return 3;
        case 0x31:
            s.SP = fetch16();
            return 3;
        case 0x02:
            wr(static_cast<u16>(BC), A);
            return 2;
        case 0x12:
            wr(static_cast<u16>(DE), A);
            return 2;
        case 0x22:
            wr(static_cast<u16>(HL), A);
            SETHL(HL + 1);
            return 2;
        case 0x32:
            wr(static_cast<u16>(HL), A);
            SETHL(HL - 1);
            return 2;
        case 0x0A:
            A = rd(static_cast<u16>(BC));
            return 2;
        case 0x1A:
            A = rd(static_cast<u16>(DE));
            return 2;
        case 0x2A:
            A = rd(static_cast<u16>(HL));
            SETHL(HL + 1);
            return 2;
        case 0x3A:
            A = rd(static_cast<u16>(HL));
            SETHL(HL - 1);
            return 2;
        case 0x03:
            SETBC(BC + 1);
            return 2;
        case 0x13:
            SETDE(DE + 1);
            return 2;
        case 0x23:
            SETHL(HL + 1);
            return 2;
        case 0x33:
            s.SP++;
            return 2;
        case 0x0B:
            SETBC(BC - 1);
            return 2;
        case 0x1B:
            SETDE(DE - 1);
            return 2;
        case 0x2B:
            SETHL(HL - 1);
            return 2;
        case 0x3B:
            s.SP--;
            return 2;
        case 0x04:
        case 0x0C:
        case 0x14:
        case 0x1C:
        case 0x24:
        case 0x2C:
        case 0x34:
        case 0x3C:
            r = (op >> 3) & 7;
            t8 = static_cast<u8>(getr(r) + 1);
            setr(r, t8);
            F = static_cast<u8>((F & FC) | (t8 ? 0 : FZ) | ((t8 & 0xf) == 0 ? FH : 0));
            return r == 6 ? 3 : 1;
        case 0x05:
        case 0x0D:
        case 0x15:
        case 0x1D:
        case 0x25:
        case 0x2D:
        case 0x35:
        case 0x3D:
            r = (op >> 3) & 7;
            t8 = static_cast<u8>(getr(r) - 1);
            setr(r, t8);
            F = static_cast<u8>((F & FC) | FN | (t8 ? 0 : FZ) | ((t8 & 0xf) == 0xf ? FH : 0));
            return r == 6 ? 3 : 1;
        case 0x06:
        case 0x0E:
        case 0x16:
        case 0x1E:
        case 0x26:
        case 0x2E:
        case 0x36:
        case 0x3E:
            r = (op >> 3) & 7;
            t8 = fetch();
            setr(r, t8);
            return r == 6 ? 3 : 2;
        case 0x07:
            t8 = A >> 7;
            A = static_cast<u8>((A << 1) | t8);
            F = t8 ? FC : 0;
            return 1;
        case 0x0F:
            t8 = A & 1;
            A = static_cast<u8>((A >> 1) | (t8 << 7));
            F = t8 ? FC : 0;
            return 1;
        case 0x17:
            t8 = A >> 7;
            A = static_cast<u8>((A << 1) | ((F & FC) ? 1 : 0));
            F = t8 ? FC : 0;
            return 1;
        case 0x1F:
            t8 = A & 1;
            A = static_cast<u8>((A >> 1) | ((F & FC) ? 0x80 : 0));
            F = t8 ? FC : 0;
            return 1;
        case 0x08:
            t16 = fetch16();
            wr(t16, static_cast<u8>(s.SP & 0xff));
            wr(static_cast<u16>(t16 + 1), static_cast<u8>(s.SP >> 8));
            return 5;
        case 0x09:
        case 0x19:
        case 0x29:
        case 0x39:
        {
            u16 v = static_cast<u16>(op == 0x09 ? BC : op == 0x19 ? DE : op == 0x29 ? HL : s.SP);
            int res = HL + v;
            F = static_cast<u8>((F & FZ) | (((HL & 0xfff) + (v & 0xfff)) > 0xfff ? FH : 0) | (res > 0xffff ? FC : 0));
            SETHL(res);
            return 2;
        }
        case 0x10:
            s.PC++;
            return 1;
        case 0x18:
        {
            auto e = static_cast<std::int8_t>(fetch());
            s.PC = static_cast<u16>(s.PC + e);
            return 3;
        }
        case 0x20:
        case 0x28:
        case 0x30:
        case 0x38:
        {
            auto e = static_cast<std::int8_t>(fetch());
            if (cond((op >> 3) & 3))
            {
                s.PC = static_cast<u16>(s.PC + e);
                return 3;
            }
            return 2;
        }
        case 0x27:
        {
            int a = A;
            if (!(F & FN))
            {
                if ((F & FH) || (a & 0xf) > 9)
                {
                    a += 6;
                }
                if ((F & FC) || a > 0x9f)
                {
                    a += 0x60;
                }
            }
            else
            {
                if (F & FH)
                {
                    a = (a - 6) & 0xff;
                }
                if (F & FC)
                {
                    a -= 0x60;
                }
            }
            F &= ~(FH | FZ);
            if ((a & 0x100) == 0x100)
            {
                F |= FC;
            }
            A = static_cast<u8>(a & 0xff);
            if (!A)
            {
                F |= FZ;
            }
            return 1;
        }
        case 0x2F:
            A = static_cast<u8>(~A);
            F |= FN | FH;
            return 1;
        case 0x37:
            F = static_cast<u8>((F & FZ) | FC);
            return 1;
        case 0x3F:
            F = static_cast<u8>((F & FZ) | ((F & FC) ? 0 : FC));
            return 1;
        case 0x76: // HALT (0x1ac1a0)
            if (!s.ime || !s.io[0xff])
            {
                return 1;
            }
            s.halted = 1;
            s.cycles = s.budget;
            return -1;
        case 0xC0:
        case 0xC8:
        case 0xD0:
        case 0xD8:
            if (cond((op >> 3) & 3))
            {
                s.PC = pop16();
                return 5;
            }
            return 2;
        case 0xC9:
            s.PC = pop16();
            return 4;
        case 0xD9: // RETI
            s.PC = pop16();
            s.ime = 1;
            s.cycles += 4;
            s.budget = s.cycles;
            return -1;
        case 0xC1:
            SETBC(pop16());
            return 3;
        case 0xD1:
            SETDE(pop16());
            return 3;
        case 0xE1:
            SETHL(pop16());
            return 3;
        case 0xF1:
            t16 = pop16();
            A = static_cast<u8>(t16 >> 8);
            F = static_cast<u8>(t16 & 0xf0);
            return 3;
        case 0xC5:
            push16(static_cast<u16>(BC));
            return 4;
        case 0xD5:
            push16(static_cast<u16>(DE));
            return 4;
        case 0xE5:
            push16(static_cast<u16>(HL));
            return 4;
        case 0xF5:
            push16(static_cast<u16>((A << 8) | F));
            return 4;
        case 0xC2:
        case 0xCA:
        case 0xD2:
        case 0xDA:
            t16 = fetch16();
            if (cond((op >> 3) & 3))
            {
                s.PC = t16;
                return 4;
            }
            return 3;
        case 0xC3:
            s.PC = fetch16();
            return 4;
        case 0xC4:
        case 0xCC:
        case 0xD4:
        case 0xDC:
            t16 = fetch16();
            if (cond((op >> 3) & 3))
            {
                push16(s.PC);
                s.PC = t16;
                return 6;
            }
            return 3;
        case 0xCD:
            t16 = fetch16();
            push16(s.PC);
            s.PC = t16;
            return 6;
        case 0xC7:
        case 0xCF:
        case 0xD7:
        case 0xDF:
        case 0xE7:
        case 0xEF:
        case 0xF7:
        case 0xFF:
            push16(s.PC);
            s.PC = op & 0x38;
            return 4;
        case 0xC6:
        case 0xCE:
        case 0xD6:
        case 0xDE:
        case 0xE6:
        case 0xEE:
        case 0xF6:
        case 0xFE:
            alu((op >> 3) & 7, fetch());
            return 2;
        case 0xCB:
            return cb();
        case 0xE0:
            t8 = fetch();
            wr(static_cast<u16>(0xff00 + t8), A);
            return 3;
        case 0xF0:
            t8 = fetch();
            A = rd(static_cast<u16>(0xff00 + t8));
            return 3;
        case 0xE2:
            wr(static_cast<u16>(0xff00 + s.C), A);
            return 2;
        case 0xF2:
            A = rd(static_cast<u16>(0xff00 + s.C));
            return 2;
        case 0xEA:
            wr(fetch16(), A);
            return 4;
        case 0xFA:
            A = rd(fetch16());
            return 4;
        case 0xE8:
        {
            auto e = static_cast<std::int8_t>(fetch());
            int res = s.SP + e;
            F = static_cast<u8>((((s.SP & 0xf) + (e & 0xf)) > 0xf ? FH : 0) | (((s.SP & 0xff) + (e & 0xff)) > 0xff ? FC : 0));
            s.SP = static_cast<u16>(res);
            return 4;
        }
        case 0xF8:
        {
            auto e = static_cast<std::int8_t>(fetch());
            int res = s.SP + e;
            F = static_cast<u8>((((s.SP & 0xf) + (e & 0xf)) > 0xf ? FH : 0) | (((s.SP & 0xff) + (e & 0xff)) > 0xff ? FC : 0));
            SETHL(res);
            return 3;
        }
        case 0xE9:
            s.PC = static_cast<u16>(HL);
            return 1;
        case 0xF9:
            s.SP = static_cast<u16>(HL);
            return 2;
        case 0xF3:
            s.ime = 0;
            return 1;
        case 0xFB: // EI (0x1af378)
            s.ime = 1;
            s.cycles += 1;
            s.budget = s.cycles;
            return -1;
        default:
            if (op >= 0x40 && op < 0x80)
            {
                int d = (op >> 3) & 7, src = op & 7;
                setr(d, getr(src));
                return (d == 6 || src == 6) ? 2 : 1;
            }
            if (op >= 0x80 && op < 0xC0)
            {
                alu((op >> 3) & 7, getr(op & 7));
                return (op & 7) == 6 ? 2 : 1;
            }
            s.illegal++; // illegal: VC adds no cycles (0x1af458) and would hang; counted
            return 1;
        }
    }

#undef BC
#undef DE
#undef HL
#undef SETBC
#undef SETDE
#undef SETHL

    void VcCore::batch() // 0x1aae60
    {
        if (s.ime)
        {
            u8 m = s.io[0x0f] & s.io[0xff];
            if (m)
            {
                for (int k = 0; k < 5; k++)
                {
                    if (m & (1 << k))
                    {
                        s.io[0x0f] &= static_cast<u8>(~(1 << k));
                        push16(s.PC);
                        s.PC = static_cast<u16>(0x40 + 8 * k);
                        break;
                    }
                }
                s.halted = 0;
                s.ime = 0;
            }
        }
        if (s.halted)
        {
            s.cycles = s.budget;
            return;
        }
        s.cycles = 0;
        do
        {
            int c = step();
            if (c >= 0)
            {
                s.cycles += c;
            }
        } while (s.cycles < s.budget);
    }

    void VcCore::ppu() // 0x19e504
    {
        u8 *ly = &s.io[0x44], *st = &s.io[0x41], *iff = &s.io[0x0f];
        if (s.S == 0 || s.S == 2) // LCD off: virtual LY
        {
            *ly = static_cast<u8>((*ly + 1) & 0xff);
            if (*ly == 0x90)
            {
                s.frameFlag = 1;
            }
            else if (*ly == 0x9a)
            {
                if (s.S == 2)
                {
                    s.S = 0;
                }
                *ly = 0;
            }
            s.linecd += s.LINE;
            return;
        }
        int mode = *st & 3;
        if (mode == 0)
        {
            (*ly)++;
            lycCheck();
            if (*ly < 0x90)
            {
                *st = static_cast<u8>((*st & ~3) | 2);
                s.linecd += s.M2;
                if (*st & 0x20)
                {
                    *iff |= 2;
                }
            }
            else
            {
                *st = static_cast<u8>((*st & ~3) | 1);
                s.linecd += s.VBD;
                s.P = 3;
                if (*st & 0x10)
                {
                    *iff |= 2;
                }
                if (s.S != 1)
                {
                    s.S = (s.S == 3) ? 1 : s.S - 1;
                }
            }
        }
        else if (mode == 1)
        {
            switch (s.P)
            {
            case 0:
                (*ly)++;
                lycCheck();
                if (*ly == 0x99)
                {
                    s.P = 1;
                    s.linecd += 1;
                }
                else
                {
                    s.linecd += s.LINE;
                }
                break;
            case 1:
                *ly = 0;
                s.P = 2;
                s.linecd += s.LINE - 1;
                lycCheck();
                break;
            case 2:
                s.P = 3;
                *st = static_cast<u8>((*st & ~3) | 2);
                s.linecd += s.M2;
                if (*st & 0x20)
                {
                    *iff |= 2;
                }
                break;
            case 3:
                s.P = 0;
                *iff |= 1;
                s.linecd += s.LINE - s.VBD;
                s.frameFlag = 1;
                break;
            }
        }
        else if (mode == 2)
        {
            *st |= 3;
            s.linecd += s.M3;
        }
        else
        {
            *st &= ~3;
            s.linecd += s.M0;
            if ((*st & 8) && s.f68 == 0)
            {
                *iff |= 2;
            }
            if (s.hdmaActive)
            {
                hdmaBlock();
            }
        }
    }

    // ---------------------------------------------------------------- run loop: one VC frame (0x1a8264)
    int VcCore::frame()
    {
        s.frameFlag = 0;
        p1Update();
        long guard = 0;
        for (;;)
        {
            if (++guard > 20000000)
            {
                return -1;
            }
            if (s.pending)
            {
                if (s.pending <= s.budget)
                {
                    s.budget -= s.pending;
                    s.pending = 0;
                }
                else
                {
                    s.pending -= s.budget;
                    s.budget = 0;
                }
            }
            batch();
            s.budget = 0xffff;
            s.tot += s.cycles;
            s.divcd -= s.cycles;
            if (s.divcd <= 0)
            {
                s.io[4]++;
                s.divcd += 0x40;
            }
            if (s.budget > s.divcd)
            {
                s.budget = s.divcd;
            }
            s.linecd -= s.cycles;
            if (s.linecd <= 0)
            {
                ppu();
            }
            if (s.budget > s.linecd)
            {
                s.budget = s.linecd;
            }
            if (s.io[7] & 4)
            {
                int k = s.io[7] & 3;
                i32 t = (s.counter & TMASK[k]) - s.cycles + 1;
                while (t <= 0)
                {
                    if (s.io[5] == 0xff)
                    {
                        s.io[5] = s.io[6];
                        s.io[0x0f] |= 4;
                    }
                    else
                    {
                        s.io[5]++;
                    }
                    t += TPER[k];
                }
                if (s.budget > t)
                {
                    s.budget = t;
                }
            }
            s.counter -= s.cycles;
            if (0xff - s.counter > 0xff)
            {
                s.counter += (0xff - s.counter) & ~0xff;
            }
            if (s.frameFlag)
            {
                break;
            }
        }
        s.frames++;
        return 0;
    }

    // ---------------------------------------------------------------- API
    void VcCore::reset(int divcd0, int counter0)
    {
        std::memset(s.vram, 0, sizeof s.vram);
        std::memset(s.wram, 0, sizeof s.wram);
        std::memset(s.oam, 0, sizeof s.oam);
        std::memset(s.io, 0, sizeof s.io);
        s.rombank = 1;
        s.rambank = 0;
        s.ramen = 0;
        s.latchprev = 0;
        s.svbk = 1;
        s.vbk = 0;
        // 0x16a854 IO reset (CGB branch)
        u8 *io = s.io;
        io[0x00] = 0xff;
        io[0x0f] = 1;
        io[0x10] = 0x80;
        io[0x11] = 0xbf;
        io[0x12] = 0xf3;
        io[0x14] = 0xbf;
        io[0x16] = 0x3f;
        io[0x19] = 0xbf;
        io[0x1a] = 0x7f;
        io[0x1c] = 0x9f;
        io[0x1e] = 0xbf;
        io[0x23] = 0xbf;
        io[0x24] = 0x77;
        io[0x25] = 0xf3;
        io[0x26] = 0xf1;
        io[0x40] = 0x91;
        io[0x47] = 0xfc;
        io[0x4d] = 0x7e;
        io[0x75] = 0x8f;
        io[0x02] = 0x7c;
        io[0x04] = 0x1e;
        io[0x41] = 0x81;
        io[0x44] = 0x90;
        io[0x53] = 0x9f;
        io[0x56] = 0x3e;
        io[0x70] = 0xf8;
        io[0x4f] = 0xfe;
        io[0x52] = 0xf0;
        io[0x54] = 0xf0;
        io[0x68] = 0xc0;
        io[0x6a] = 0xc0;
        io[0x6c] = 0xfe;
        io[0x51] = 0xff;
        io[0x55] = 0xff;
        // 0x15c44c CPU reset (CGB)
        s.A = 0x11;
        s.F = 0x80;
        s.B = 0;
        s.C = 0;
        s.D = 0xff;
        s.E = 0x56;
        s.H = 0x00;
        s.L = 0x0d;
        s.SP = 0xfffe;
        s.PC = 0x100;
        s.ime = 0;
        s.halted = 0;
        s.cycles = 0;
        s.budget = 0;
        // 0x16b998 PPU init
        s.linecd = 0x72;
        s.S = 1;
        s.P = 0;
        s.f68 = 0;
        s.M0 = 51;
        s.LINE = 114;
        s.M2 = 20;
        s.M3 = 43;
        s.VBD = 1;
        s.divcd = divcd0;
        s.counter = counter0;
        s.pending = 0;
        s.hdmaActive = 0;
        s.frameFlag = 0;
        s.tot = 0;
        s.frames = 0;
        s.serialWarn = 0;
        s.vbcount = 0;
    }

    int VcCore::runUntilEnemy(std::span<const u8> joys, int maxf, int species, u8 *out)
    {
        int seen = -1;
        int njoy = static_cast<int>(joys.size());
        for (int i = 0; i < maxf; i++)
        {
            s.joy = joys[i < njoy ? i : njoy - 1];
            if (frame() < 0)
            {
                return -2;
            }
            if (seen < 0 && s.wram[1][0x206] == species)
            {
                seen = i;
            }
            if (seen >= 0 && i >= seen + 2)
            {
                std::memcpy(out, &s.wram[1][0x206], 8);
                return i + 1;
            }
        }
        return -1;
    }

    int VcCore::runPlan(std::span<const u8> joyv, int maxf, int species, u8 *out)
    {
        int seen = -1;
        long nj = static_cast<long>(joyv.size());
        for (int i = 0; i < maxf; i++)
        {
            long k = s.vbcount + 1;
            s.joy = joyv[k < nj ? k : nj - 1];
            if (frame() < 0)
            {
                return -2;
            }
            if (seen < 0 && s.wram[1][0x206] == species)
            {
                seen = i;
            }
            if (seen >= 0 && i >= seen + 2)
            {
                std::memcpy(out, &s.wram[1][0x206], 8);
                return static_cast<int>(s.vbcount);
            }
        }
        return -1;
    }

    bool VcCore::runTo(std::span<const u8> joyv, long vb)
    {
        u8 o[8];
        while (s.vbcount < vb)
        {
            if (runPlan(joyv, 1, 0x1ff, o) == -2)
            {
                return false;
            }
        }
        return true;
    }

    std::uint64_t VcCore::stateHash() const
    {
        std::uint64_t h = 1469598103934665603ULL;
        auto hb = [&h](const void *p, std::size_t n) {
            const auto *b = static_cast<const u8 *>(p);
            for (std::size_t i = 0; i < n; i++)
            {
                h ^= b[i];
                h *= 1099511628211ULL;
            }
        };
        hb(s.wram, sizeof s.wram);
        hb(s.vram, sizeof s.vram);
        hb(s.oam, sizeof s.oam);
        hb(s.io, sizeof s.io);
        hb(s.sram, sizeof s.sram);
        // same layout as the prototype's int v[]: 26 native ints
        int v[] = { s.rombank, s.rambank, s.svbk, s.vbk,     s.A,     s.F,       s.B,       s.C,      s.D,          s.E,
                    s.H,       s.L,       s.SP,   s.PC,      s.ime,   s.halted,  s.cycles,  s.budget, s.divcd,      s.counter,
                    s.pending, s.hdmaActive, s.linecd, s.S, s.P, s.f68 };
        hb(v, sizeof v);
        return h;
    }
}
