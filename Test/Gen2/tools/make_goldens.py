#!/usr/bin/env python3
"""Generate the Gen 2 (Crystal VC Celebi) golden data from the research prototype (celebi_tool.py v0.6 + libvc).

The goldens hold outputs only (enemy bytes, state hashes, VBlank counts, DVs, verdicts, posteriors) and the inputs that
produced them (joypad timelines, clocks, minimal save values). No ROM bytes, no save bytes, no patch data.

Needs the prototype workspace (v17 + v18..v25 add-ons, paths rewritten, libvc built, nin/roms/<lang>_vc.gbc and the
saves in place) in $CELEBI_WS (default ~/celebi-ws). Run with python3 (numpy needed):

    CELEBI_WS=~/celebi-ws python3 Test/Gen2/tools/make_goldens.py core [...]

Sections: core (step 1).
"""
import hashlib, json, os, random, sys

WS = os.path.expanduser(os.environ.get('CELEBI_WS', '~/celebi-ws'))
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(HERE), 'golden2')
sys.path.insert(0, os.path.join(WS, 'vcore'))
os.chdir(os.path.join(WS, 'vcore'))
import celebi_tool as ct  # noqa: E402
import g2lib  # noqa: E402
from g2lib import L  # noqa: E402

LANGS = ('fr', 'de', 'es', 'en', 'it')
VICTOR_SAVE = os.path.join(WS, 'mnt/user-data/outputs/mb/sav.dat')
EN_TEST_SAVE = os.path.expanduser(os.environ.get(
    'CELEBI_EN_TEST_SAVE', '~/Documents/Personnal/Games/Pokemon/Data/Saves/EN-PKHeX-milestones/sav_EN-PKHeX_04_Ilex_shrine.dat'))
# plan 1 of record per language (SPEC v0.6 section 3.2): (d6, dyes, final)
PLAN1 = dict(fr=(-3, 0, 1933), de=(0, 2, 1489), es=(1, 0, 1505), en=(-3, 0, 1933), it=(2, 0, 2597))


def fnv64(b):
    h = 1469598103934665603
    for x in b:
        h ^= x
        h = (h * 1099511628211) & 0xffffffffffffffff
    return '%016x' % h


def rle(j):
    out, s = [], 0
    for k in range(1, len(j) + 1):
        if k == len(j) or j[k] != j[s]:
            if j[s]:
                out.append([s, k, j[s]])
            s = k
    return out


def shifted(j, X):
    n = len(j)
    return [j[k - X] if 0 <= k - X < n else (0 if k - X < 0 else j[-1]) for k in range(n)]


def dump(name, data):
    os.makedirs(OUT, exist_ok=True)
    p = os.path.join(OUT, name)
    json.dump(data, open(p, 'w'), separators=(',', ':'))
    print('wrote', p, os.path.getsize(p), 'bytes')


def sec_core():
    """Step 1: random timelines (presses +-3, timer X +-3, random clocks) -> enemy bytes, VBlank count, state hash."""
    R = random.Random(20261125)
    saves = dict(victor=ct.sram_of(VICTOR_SAVE), en_test=ct.sram_of(EN_TEST_SAVE))
    roms = {l: g2lib.rom(l) for l in LANGS}
    cases = []
    for lang in LANGS:
        sets = [('victor', 'plan_%s_lp.json' % lang, PLAN1[lang])]
        if lang == 'en':
            sets.append(('en_test', '../np/plan_en_np.json', (0, -2, 1518)))
        for save, planf, (d6, dy, fin) in sets:
            P = json.load(open(planf))
            for _ in range(200):
                e1, e2, e3, X = (R.randint(-3, 3) for _ in range(4))
                rtc = [R.randrange(60), R.randrange(60), R.randrange(24), R.choice((117, 117, 118, R.randrange(100, 140))), 0]
                j = shifted(ct.build(ct.plan_events(P, d6 + e1, dy + e2, fin + e3)), X)
                g2lib.boot(lang, saves[save], tuple(rtc))
                h, r = g2lib.run(j, 4000)
                cases.append(dict(lang=lang, save=save, rtc=rtc, joy=rle(j), n=len(j), enemy=h if r > 0 else None, ret=r,
                                  hash='%016x' % L.vc_state_hash(), frames=L.vc_frames()))
    # r8_13: the first harness run (Azahar libretro, FR title, Victor's save), replayed frame by frame with its logged
    # joypad and clock reads (g3.Rep2 with the VC reset phases 25 / 88)
    os.environ['AZLOG_DIR'] = os.path.join(WS, 'hlog') + '/'
    import repair  # noqa: E402  (sets up g3.Rep2 with AZLOG_DIR)
    r = repair.Rep2('r8_13_hz', 25, 88)
    last = max(r.LOG); frames = []
    init_rtc = list(r.rtc)
    while r.hits < last + 3 and len(frames) < 4000:
        rt = r.rtc_at(r.hits + 1)
        joy = r.JD.get(r.hits + 1, 0)
        frames.append([joy, list(rt) if (rt[2] | rt[3] | rt[1]) else None])
        r.step()
    enemy = bytes(L.vc_wram(1, 0x206 + i) for i in range(8)).hex()
    r813 = dict(rom='fr', save='victor', init_rtc=init_rtc, frames=frames, enemy=enemy, expect='fb00d7dbf6f82aaa',
                hash='%016x' % L.vc_state_hash())
    print('r8_13 replay enemy', enemy)
    dump('core.json', dict(about='step 1 VcCore goldens (prototype vccore.c via libvc)', divcd0=25, counter0=88,
                           roms={l: fnv64(roms[l]) for l in LANGS}, saves={k: fnv64(v) for k, v in saves.items()},
                           cases=cases, r8_13=r813))


SECTIONS = dict(core=sec_core)
if __name__ == '__main__':
    for s in sys.argv[1:] or SECTIONS:
        SECTIONS[s]()
