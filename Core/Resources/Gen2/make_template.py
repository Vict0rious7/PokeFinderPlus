#!/usr/bin/env python3
"""PokeFinderPlus Gen 2: builds the Celebi save template (crystal_template.sav) from a Crystal shrine save
(international layout: FR, DE, IT, ES, EN). Every name and the trainer ID are replaced or cleared; the values that
change Celebi (CrystalSave::readMinimal / applyMinimal) are left as they are and set per player at run time.

usage: python3 make_template.py <shrine save (sav.dat, 32 KB + optional 16-byte VC footer)> <out.sav>

What is changed (pokecrystal symbols; both save copies, then both checksums):
- player / mom / rival names, box names, the party's OT names and nicknames, the trainer ID;
- every box (current box and boxes 1-14): OT names and nicknames;
- party mail, mailbox (and their backups), link battle records, Hall of Fame, Mystery Gift partner name and trainer.
"""
import sys

SIZE = 0x8000
# saved WRAM segments: [lo, hi) -> offset of the main / backup copy (CrystalSave.cpp SEG)
SEG = ((0xd47b, 0xdca5, 0x2009, 0x1209), (0xdca5, 0xdcd7, 0x2833, 0x1a33), (0xdcd7, 0xdff5, 0x2865, 0x1a65))
CHARS = {c: 0x80 + i for i, c in enumerate('ABCDEFGHIJKLMNOPQRSTUVWXYZ')}
TERM = 0x50


def text(s, n):
    b = [CHARS[c] for c in s] + [TERM]
    return bytes(b + [TERM] * (n - len(b)))


def sram(bank, addr):
    return bank * 0x2000 + addr - 0xa000


def wram(b, addr, data):
    for lo, hi, main, backup in SEG:
        if lo <= addr < hi:
            for base in (main, backup):
                o = base + addr - lo
                b[o:o + len(data)] = data
            return
    raise ValueError(hex(addr))


def box(b, off, slots=20):
    """Gen 2 box: count, species[slots + 1], mons[slots x 32], OTs[slots x 11], nicknames[slots x 11]"""
    ot = off + 1 + slots + 1 + slots * 32
    for i in range(slots):
        b[ot + 11 * i:ot + 11 * (i + 1)] = text('PLAYER', 11)
        b[ot + 11 * slots + 11 * i:ot + 11 * slots + 11 * (i + 1)] = text('MON', 11)


def checksum(b, lo, hi, at):
    s = sum(b[lo:hi]) & 0xffff
    b[at] = s & 0xff
    b[at + 1] = s >> 8


def main():
    b = bytearray(open(sys.argv[1], 'rb').read()[:SIZE])
    assert len(b) == SIZE
    wram(b, 0xd47b, bytes(2))                      # wPlayerID
    wram(b, 0xd47d, text('PLAYER', 11))            # wPlayerName
    wram(b, 0xd488, text('MOM', 11))               # wMomsName
    wram(b, 0xd493, text('RIVAL', 11))             # wRivalName
    for i in range(14):                            # wBoxNames
        wram(b, 0xdb75 + 9 * i, text('BOX', 9))
    for i in range(6):
        wram(b, 0xddff + 11 * i, text('PLAYER', 11))   # wPartyMonOTs
        wram(b, 0xde41 + 11 * i, text('MON', 11))      # wPartyMonNicknames
    box(b, sram(1, 0xad10))                        # sBox (current box)
    for bank, addrs in ((2, (0xa000, 0xa450, 0xa8a0, 0xacf0, 0xb140, 0xb590, 0xb9e0)),
                        (3, (0xa000, 0xa450, 0xa8a0, 0xacf0, 0xb140, 0xb590, 0xb9e0))):
        for a in addrs:
            box(b, sram(bank, a))
    b[sram(0, 0xa600):sram(0, 0xabe2)] = bytes(0xabe2 - 0xa600)    # party mail, mailbox, backups
    b[sram(0, 0xabfe):sram(0, 0xac09)] = bytes(0xac09 - 0xabfe)    # sMysteryGiftPartnerName
    b[sram(0, 0xac0a):sram(0, 0xac30)] = bytes(0xac30 - 0xac0a)    # sMysteryGiftTrainer
    b[sram(1, 0xb260):sram(1, 0xb2c0)] = bytes(0xb2c0 - 0xb260)    # sLinkBattleStats
    b[sram(1, 0xb2c0):sram(1, 0xbe3d)] = bytes(0xbe3d - 0xb2c0)    # sHallOfFame
    checksum(b, 0x2009, 0x2009 + 0xb7a, 0x2d0d)    # sGameData -> sChecksum (CrystalSave::fix)
    checksum(b, 0x1209, 0x1209 + 0xb7a, 0x1f0d)    # sBackupGameData -> sBackupChecksum
    open(sys.argv[2], 'wb').write(b)


if __name__ == '__main__':
    main()
