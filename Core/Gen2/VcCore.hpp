/*
 * Game Boy Color core whose timing is a transcription of the Nintendo 3DS Virtual Console run loop of Pokémon Crystal
 * (VC .code: run loop 0x1a8264, batch 0x1aae60, PPU 0x19e504). No rendering, no sound.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 *
 * Port of the research prototype's vccore.c (crystal-vc-celebi, 2026-11-18). Every file-scope variable of the C file is a
 * member of VcCore::State, so several cores can run in parallel threads and a snapshot is a plain copy of the state
 * (~82 KB). The ROM is held separately (read-only, shared between cores). The behaviour is checked byte for byte against
 * the prototype (Test/Gen2). No Nintendo data is in this file or anywhere in the repository: the ROM is loaded at run
 * time from the player's own files.
 */

#ifndef VCCORE_HPP
#define VCCORE_HPP

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace Gen2
{
    /**
     * @brief GBC core with the 3DS Virtual Console's instruction, DIV, timer and PPU timing.
     *
     * Joypad bits (active high): 0 A, 1 B, 2 Select, 3 Start, 4 Right, 5 Left, 6 Up, 7 Down.
     */
    class VcCore
    {
    public:
        using u8 = std::uint8_t;
        using u16 = std::uint16_t;
        using i32 = std::int32_t;

        /// Everything that changes while the core runs (the prototype's file-scope state). A snapshot is a copy.
        struct State
        {
            u8 vram[2][0x2000];
            u8 wram[8][0x1000];
            u8 sram[4][0x2000];
            u8 oam[0xa0];
            u8 io[0x100]; // io[0x80..0xfe] = HRAM, io[0xff] = IE
            int rombank, rambank, ramen, latchprev, svbk, vbk;
            u8 rtcCur[5], rtcLat[5]; // S M H DL DH
            u8 A, F, B, C, D, E, H, L;
            u16 SP, PC;
            int ime, halted;
            i32 cycles, budget; // 0x22f604 / 0x22f600
            i32 divcd; // 0x22fa50
            i32 counter; // 0x22fa48
            i32 pending; // 0x22f68c
            int hdmaActive; // 0x22f694
            i32 linecd; // PPU +0x04
            i32 M0, LINE, M2, M3, VBD; // PPU +08 +0c +10 +14 +70
            int S, P; // PPU +0x4c, +0x50
            int f68; // PPU +0x68
            int frameFlag; // 0x22f6e4
            u8 joy;
            long long tot; // M-cycles elapsed
            long long frames;
            long vbcount; // times PC reached 0x02c5 (Crystal's VBlank handler)
            int serialWarn;
            int illegal;
        };

        VcCore();

        /// ROM bytes (shared, not copied). Up to 2 MB.
        void loadRom(std::shared_ptr<const std::vector<u8>> rom);
        /// Cartridge RAM (the first 32 KB of the VC sav.dat).
        void loadSram(std::span<const u8> data);
        void setRtc(int s, int m, int h, int dl, int dh);
        void setJoy(u8 j);
        /// Power-on state. The VC's reset phases are DIV countdown 25 and timer counter 88.
        void reset(int divcd0 = 25, int counter0 = 88);

        /// One VC frame (0x1a8264). Returns -1 when the core hangs (guard), 0 otherwise.
        int frame();

        /**
         * @brief Plan runner keyed by VBlank count: the joypad for a frame is joyv[vbcount + 1] (the last value repeats).
         * Runs until wram1[0x206] == species, plus 2 frames, or maxf frames.
         * @return vbcount at the stop with the 8 enemy bytes in out; -1 = no encounter within maxf; -2 = core hang
         */
        int runPlan(std::span<const u8> joyv, int maxf, int species, u8 *out);
        /// Joypad by frame index (joys[i] for frame i, the last value repeats). Returns frames run, -1, or -2.
        int runUntilEnemy(std::span<const u8> joys, int maxf, int species, u8 *out);
        /// Runs until vbcount >= vb (the prototype's g2lib.run_to). Returns false on a core hang.
        bool runTo(std::span<const u8> joyv, long vb);

        /// FNV-1a hash of the emulated state that matters for the future (not the elapsed-time counters).
        std::uint64_t stateHash() const;

        u8 rd(u16 a) const;
        void wr(u16 a, u8 v);
        u8 wram(int bank, int off) const
        {
            return s.wram[bank & 7][off & 0xfff];
        }
        /// WRAM by address C000-DFFF (bank 1 for D000-DFFF), as the prototype's g2lib.wram.
        u8 wramAt(int addr) const
        {
            return wram(addr < 0xd000 ? 0 : 1, addr & 0xfff);
        }
        const u8 *sram() const
        {
            return &s.sram[0][0];
        }
        long vbcount() const
        {
            return s.vbcount;
        }
        long long frames() const
        {
            return s.frames;
        }
        long long tot() const
        {
            return s.tot;
        }
        int illegal() const
        {
            return s.illegal;
        }

        const State &snapshot() const
        {
            return s;
        }
        void restore(const State &st)
        {
            s = st;
        }

    private:
        std::shared_ptr<const std::vector<u8>> romData;
        const u8 *rom = nullptr;
        int romsize = 0x200000;
        State s {};

        void lycCheck();
        void hdmaBlock();
        void gdma();
        void lcdcChange(u8 old);
        void p1Update();
        void ioWr(u8 r, u8 v);
        u8 fetch();
        u16 fetch16();
        void push16(u16 v);
        u16 pop16();
        u8 getr(int i);
        void setr(int i, u8 v);
        void alu(int op, u8 v);
        int cond(int cc) const;
        int cb();
        int step();
        void batch();
        void ppu();
    };
}

#endif // VCCORE_HPP
