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


def mini_json(m):
    d = dict(m)
    d['start'] = list(d['start'])
    return d


def sec_save():
    """Step 2: apply_minimal on 50 random minimal-value sets (2 templates) -> FNV of the 32 KB; read_minimal round trip."""
    from savetool import Save
    from savebuild import apply_minimal, read_minimal
    R = random.Random(2)
    tpls = dict(victor=Save(ct.sram_of(VICTOR_SAVE)), en_test=Save(ct.sram_of(EN_TEST_SAVE)))
    out = dict(templates={k: dict(fnv=fnv64(bytes(v.b)), minimal=mini_json(read_minimal(v))) for k, v in tpls.items()}, cases=[])
    for i in range(50):
        t = ('victor', 'en_test')[i % 2]
        m = dict(textspeed=R.choice(('instant', 'fast', 'mid', 'slow')), sound=R.choice(('mono', 'stereo')), frame=R.randint(1, 8),
                 raikou=R.random() < .5, entei=R.random() < .5, lass=R.random() < .7, swarm=R.random() < .3, F=R.randrange(60),
                 playsec=R.randrange(60), start=(R.randrange(7), R.randrange(24), R.randrange(60), R.randrange(60)), kenji=R.choice((0, 1, 2)))
        if i % 10 == 9:
            del m['kenji']
        s = apply_minimal(tpls[t], m)
        b = s.bytes()
        out['cases'].append(dict(template=t, minimal=mini_json(m), fnv=fnv64(b), readback=mini_json(read_minimal(Save(b)))))
    dump('save.json', out)


# plan 2 of record per language (SPEC v0.6 section 3.2)
PLAN2 = dict(fr=(1, -1, 1933), de=(-2, 1, 1933), es=(0, 0, 1505), en=(-1, 0, 2994), it=(-1, -2, 2641))


def sec_plan():
    """Step 3: the stored plan_<lang>_lp.json (Victor's save; discovered at chip 18:12:30 = in-game 20:30 with offset 2:18,
    and the same at chip 20:30:30) and the prototype's timer rows for plans 1 and 2 (rtc 18:12:30 day 117)."""
    sram = ct.sram_of(VICTOR_SAVE)
    out = dict(rtcs=[[30, 12, 18, 117, 0], [30, 30, 20, 117, 0]], plans={}, timers=[])
    for lang in LANGS:
        P = json.load(open('plan_%s_lp.json' % lang))
        out['plans'][lang] = P
        for i, (d6, dy, fin) in enumerate((PLAN1[lang], PLAN2[lang])):
            g2lib._roms.clear()
            rows = ct.timer(lang, sram, P, d6, dy, fin)
            out['timers'].append(dict(lang=lang, plan=i + 1, d6=d6, dyes=dy, final=fin, rows=rows))
    # the EN test save, discovered by the prototype at chip 20:30:30 (np/plan_en_np.json is from another save / clock)
    P = ct.discover('en', ct.sram_of(EN_TEST_SAVE), (30, 30, 20, 117, 0))
    P['events'] = [list(e) for e in P['events']]
    out['en_test'] = dict(rtc=[30, 30, 20, 117, 0], plan=P)
    dump('plan.json', out)


FS_DIRS = dict(fr='runs/fs_fr', de='runs/fs_de', es='runs/fs_es', en='runs/fs_en', it='runs/fs_it', fr_k2='runs/t25/fs2_fr',
               it_k2='runs/t25/fs2_it', en_test='runs/t25/fs_np')


def grid_str(g, D, pre=''):
    """leaf dict -> one hex string in (e6, eYES, efinal) order, each -D..D"""
    r = range(-D, D + 1)
    return ''.join(g[pre + '%d,%d,%d' % (a, b, c)] for a in r for b in r for c in r)


def sec_search():
    """Step 4: picks of the stored fsearch directories (all hits), a recompute sample of fsearch jobs, 20 sampled jobs of
    fg3_fr / fgx_fr / cal3_fr, regr.py's 20 rows, clock sweeps (windows) of FR and IT plan 1."""
    R = random.Random(4)
    out = dict(picks={}, fsjobs=[], grids=[], regr=[], sweeps=[])
    for name, d in FS_DIRS.items():
        G = ct.load_jobs(os.path.join(WS, d))
        keys = sorted(G)
        hits = [h for k in keys for h in G[k]]
        p1, p2 = ct.pick(hits)
        i1 = next(i for i, h in enumerate(hits) if h is p1)
        i2 = next(i for i, h in enumerate(hits) if h is p2)
        out['picks'][name] = dict(dir=d, keys=keys, hits=hits, p1=i1, p2=i2, union=len(set(p1['F_ok']) | set(p2['F_ok'])))
        if name == 'en_test':
            continue          # its save (np_k2k2.sav) is not rebuilt here: pick only
        lang = name[:2]
        with_hits = [k for k in keys if any(h is p1 or h is p2 for h in G[k])]
        without = R.sample([k for k in keys if not G[k]], 2)
        for k in with_hits + without:
            out['fsjobs'].append(dict(set=name, lang=lang, key=k, save='victor', kenji=2 if name.endswith('_k2') else None,
                                      rtcs=sorted({kk.split('_')[-1] for kk in keys}), hits=G[k]))
    # grids of FR plans 1 + 2 (fs_fr pick) and the calibration grid cal3_fr (plan 0 / 0 / plan 1 final, in-game 20:30)
    for gdir, n in (('runs/fg3_fr', 20), ('runs/fgx_fr', 20), ('runs/cal3_fr', 20)):
        G = ct.load_jobs(os.path.join(WS, gdir))
        for k in R.sample(sorted(G), n):
            D = round(len(G[k]) ** (1 / 3)) // 2
            pre = '' if k.startswith('f_') else '%d,' % int(k.split('_')[4])
            out['grids'].append(dict(set=gdir.split('/')[-1], key=k, D=D, leaves=grid_str(G[k], D, pre)))
        out['grid_keys_' + gdir.split('/')[-1]] = sorted(G)
    # regr.py: leaf 0,0,0 of the D = 2 switch grid at F 7 and 26, plans 1 + 2, five languages, vs fg3_<lang>
    for l in LANGS:
        G = ct.load_jobs(os.path.join(WS, 'runs/fg3_' + l))
        for i in (0, 1):
            for F in (7, 26):
                out['regr'].append(dict(lang=l, plan=i, F=F, dv=G['f_%d_%02d' % (i, F)]['0,0,0']))
    # windows: chip-minute sweeps of FR and IT plan 1 (offset 2:18)
    sram = ct.sram_of(VICTOR_SAVE)
    for l in ('fr', 'it'):
        P = json.load(open('plan_%s_lp.json' % l))
        p1 = out['picks'][l]['hits'][out['picks'][l]['p1']]
        g2lib._roms.clear()
        res = ct.sweep(l, sram, ct.plan_events(P, p1['d6'], p1['dyes'], p1['final']), 4)
        out['sweeps'].append(dict(lang=l, d6=p1['d6'], dyes=p1['dyes'], final=p1['final'], offset='2:18',
                                  res=[res[t] for t in range(1440)], windows=ct.windows(res, ct.hm('2:18'))))
    dump('search.json', out)


SECTIONS = dict(core=sec_core, save=sec_save, plan=sec_plan, search=sec_search)
if __name__ == '__main__':
    for s in sys.argv[1:] or SECTIONS:
        SECTIONS[s]()
