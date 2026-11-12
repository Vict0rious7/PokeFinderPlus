// Golden data for PokeFinderPlus / PokeTimerPlus "XY TID", produced by running
// 6genTidSearch's own classes (BZL) by reflection under Mono.
// Usage: mono Golden.exe 6genTidSearch.exe out.json
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;

class Golden
{
    const BindingFlags F = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static;
    static Assembly asm;
    static Type tTiny, tTimer, tLine;

    static object New(Type t, params object[] a) => t.GetConstructor(F, null, a.Select(x => x.GetType()).ToArray(), null).Invoke(a);
    static object Call(object o, string m, params object[] a)
    {
        var t = o.GetType();
        var mi = t.GetMethods(F).First(x => x.Name == m && x.GetParameters().Length == a.Length
                                            && x.GetParameters().Select(p => p.ParameterType).SequenceEqual(a.Select(y => y.GetType())));
        return mi.Invoke(o, a);
    }
    static object Get(object o, string f) => o.GetType().GetField(f, F).GetValue(o);
    static void Set(object o, string f, object v) => o.GetType().GetField(f, F).SetValue(o, v);
    static string Arr(IEnumerable<uint> a) => "[" + string.Join(",", a) + "]";

    // ---------- TinyMT ----------
    static uint[] TinyOutputs(object mt, int n)
    {
        var r = new uint[n];
        for (int i = 0; i < n; i++) r[i] = (uint)Call(mt, "Nextuint");
        return r;
    }

    // ---------- Timeline emulation with 6genTidSearch's primitives ----------
    // Event order chosen (documented in PokeTimerPlus): an event due at wall frame w applies before frame w is processed.
    class Scenario
    {
        public uint[] State; public uint ElapsedMs; public bool Male; public List<int[]> Nudges = new List<int[]>();
        public uint[] Steps; public int Rows;
    }

    static string RunScenario(Scenario s, out List<uint[]> rows, out List<uint> beeps)
    {
        object line = New(tLine, (object)s.State.Clone(), (uint)1);
        Call(line, "Kill"); // stop its background thread; the deterministic primitives still work
        uint skip = (uint)(s.ElapsedMs * 59.8261 / 1000.0) + 4;
        Call(line, "StaticSkip", skip);

        uint p1 = s.Steps[0], p2 = s.Steps[1], p3 = s.Steps[2], p4 = s.Steps[3];
        uint off = s.Male ? 116u : 136u;
        uint snap = p4 + off;
        bool running = true;
        beeps = new List<uint>();
        var nud = s.Nudges.ToDictionary(n => (uint)n[0], n => n[1]);
        for (uint w = 1; w <= snap; w++)
        {
            if (w == p1 + 6) Set(line, "_npc", (uint)3);
            if (w == p2 + 28) running = false;
            if (w == p2 + 29) { Call(Get(line, "_tinyMt"), "Nextuint"); }
            if (w == p3 + 136) running = true;
            if (w == p3 + 136 + 146) Set(line, "_npc", (uint)4);
            if (w == snap)
            {
                var c = (uint[])Call(line, "GetCounter"); c[3] = 0; Call(line, "SetCounter", (object)c);
                break; // snapshot before frame `snap` is processed
            }
            int n = 0; nud.TryGetValue(w, out n);
            int frames = running ? 1 + n : 0;
            for (int k = 0; k < frames; k++)
            {
                uint before = ((uint[])Get(line, "_npcCounter"))[0];
                Call(line, "StaticSkip", (uint)1);
                uint after = ((uint[])Get(line, "_npcCounter"))[0];
                if (after > before || before == 0) beeps.Add(w); // professor drew a new interval this frame
            }
        }
        uint[] seed = (uint[])Call(line, "GetSeed");
        uint[] counter = (uint[])Call(line, "GetCounter");
        uint npcNow = (uint)Get(line, "_npc");

        // SYOUHI_CALC_Click, b__0 (row loop without the UI range filter)
        object copy = New(tLine, (object)s.State.Clone(), (uint)4);
        Call(copy, "Kill");
        Call(copy, "SetSeed", (object)seed);
        Call(copy, "SetCounter", (object)counter);
        Call(copy, "StaticSkip", (uint)660);
        uint baseTf = (uint)Call(copy, "GetTinyFrameFast", (uint)0);
        uint f = 0;
        rows = new List<uint[]>();
        for (int i = 0; i < s.Rows; i++)
        {
            uint r = (uint)Call(copy, "GetRandFast", (uint)0);
            uint tid = r & 0xFFFF, sid = r >> 16, tsv = (tid ^ sid) >> 4, trv = (tid ^ sid) & 15;
            uint f2 = f;
            uint tf = (uint)Call(copy, "GetTinyFrameFast", (uint)0);
            do { Call(copy, "StaticSkip", (uint)1); f++; } while ((uint)Call(copy, "GetRandFast", (uint)0) == r);
            uint wait = f2 == 0 ? 0 : f2 + off;
            uint window = f2 == 0 ? f + off : f - f2;
            rows.Add(new uint[] { tf - baseTf, tid, sid, tsv, trv, wait, window });
        }
        return "{\"seed\":" + Arr(seed) + ",\"counter\":" + Arr(counter.Take(4)) + ",\"npc\":" + npcNow + "}";
    }

    static void Main(string[] args)
    {
        asm = Assembly.LoadFrom(args[0]);
        Type[] ts;
        try { ts = asm.GetTypes(); } catch (ReflectionTypeLoadException e) { ts = e.Types.Where(t => t != null).ToArray(); }
        tTiny = ts.First(t => t.FullName == "_6genTidSearch.TinyMT");
        tTimer = ts.First(t => t.FullName == "_6genTidSearch.BlinkTimer");
        tLine = ts.First(t => t.FullName == "_6genTidSearch.BlinkTimeLine");
        var rnd = new Random(20261112);
        Func<uint> R32 = () => (uint)rnd.Next(1 << 16) << 16 | (uint)rnd.Next(1 << 16);
        var sb = new StringBuilder("{\n");

        // 1. TinyMT from a seed, and from a 4-word state
        var seeds = new List<uint> { 0, 1, 0x12345678, 0xFFFFFFFF, 0x80000000 };
        for (int i = 0; i < 200; i++) seeds.Add(R32());
        sb.Append("\"tinymt\":[\n");
        sb.Append(string.Join(",\n", seeds.Select(sd => "{\"seed\":" + sd + ",\"status\":" + Arr((uint[])Get(New(tTiny, sd), "status")) + ",\"out\":" + Arr(TinyOutputs(New(tTiny, sd), 64)) + "}")));
        sb.Append("],\n\"tinymt_state\":[\n");
        var st = new List<string>();
        for (int i = 0; i < 50; i++)
        {
            var mt = New(tTiny, R32()); Call(mt, "Next", (uint)rnd.Next(1000));
            var state = ((uint[])Get(mt, "status")).ToArray();
            st.Add("{\"state\":" + Arr(state) + ",\"out\":" + Arr(TinyOutputs(New(tTiny, (object)state.Clone()), 32)) + "}");
        }
        sb.Append(string.Join(",\n", st)).Append("],\n");

        // 2. Blink interval
        var timer = Activator.CreateInstance(tTimer, true);
        var cb = new List<string>();
        foreach (uint r in new uint[] { 0, 1, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF }.Concat(Enumerable.Range(0, 2000).Select(_ => R32())))
            cb.Add("[" + r + "," + (uint)Call(timer, "CalcBlink", r) + "]");
        sb.Append("\"calcblink\":[").Append(string.Join(",", cb)).Append("],\n");

        // 3. Seed search primitives (Search / HitFrame)
        var sc = new List<string>();
        for (int i = 0; i < 3000; i++)
        {
            uint seed = R32();
            uint fmin = i % 5 == 0 ? (uint)rnd.Next(0, 50) : 5u;
            uint fmax = i % 5 == 0 ? fmin + (uint)rnd.Next(0, 200) : 20u;
            int k = rnd.Next(3, 13);
            var mt = New(tTiny, seed); Call(mt, "Next", fmin);
            var outs = TinyOutputs(mt, (int)(fmax - fmin) + k + 5);
            uint start = (uint)rnd.Next(0, (int)(fmax - fmin) + 3);
            uint[] gaps = new uint[k];
            int kind = i % 4; // 0 clean, 1 jitter ±10, 2 jitter up to ±13, 3 random
            for (int j = 0; j < k; j++)
            {
                uint b = (uint)Call(timer, "CalcBlink", outs[start + j]);
                int jit = kind == 0 ? 0 : kind == 1 ? rnd.Next(-10, 11) : rnd.Next(-13, 14);
                gaps[j] = kind == 3 ? (uint)rnd.Next(180, 545) : (uint)Math.Max(0, (int)b + jit);
            }
            if (i % 7 == 0 && k > 3) gaps[rnd.Next(1, k)] = (uint)rnd.Next(180, 545); // one bad tap
            bool found = (bool)Call(timer, "Search", (object)gaps, seed, fmin, fmax);
            var hit = Call(timer, "HitFrame", (object)gaps, seed, fmin, fmax);
            var ht = hit.GetType();
            uint[] h = new uint[5];
            for (int j = 0; j < 5; j++) h[j] = (uint)ht.GetField("Item" + (j + 1)).GetValue(hit);
            sc.Add("{\"seed\":" + seed + ",\"fmin\":" + fmin + ",\"fmax\":" + fmax + ",\"gaps\":" + Arr(gaps) + ",\"found\":" + (found ? "true" : "false") + ",\"hit\":" + Arr(h) + "}");
        }
        sb.Append("\"search\":[\n").Append(string.Join(",\n", sc)).Append("],\n");

        // 4. Timeline: double-click → steps → wait list
        var tl = new List<string>();
        for (int i = 0; i < 400; i++)
        {
            var mt = New(tTiny, R32()); Call(mt, "Next", (uint)rnd.Next(5, 40));
            var sce = new Scenario
            {
                State = ((uint[])Get(mt, "status")).ToArray(),
                ElapsedMs = (uint)rnd.Next(2000, 600000),
                Male = rnd.Next(2) == 0,
                Rows = 120,
            };
            uint p1 = (uint)rnd.Next(60, 6000);
            uint p2 = p1 + (uint)rnd.Next(40, 1200);
            uint p3 = p2 + (uint)rnd.Next(60, 4000);
            uint p4 = p3 + (uint)rnd.Next(300, 1500);
            sce.Steps = new[] { p1, p2, p3, p4 };
            int nn = rnd.Next(0, 6);
            var used = new HashSet<int>();
            for (int j = 0; j < nn; j++)
            {
                int w = rnd.Next(1, (int)p1);
                if (used.Add(w)) sce.Nudges.Add(new[] { w, rnd.Next(2) == 0 ? 1 : -1 });
            }
            sce.Nudges = sce.Nudges.OrderBy(x => x[0]).ToList();
            List<uint[]> rows; List<uint> beeps;
            string snap = RunScenario(sce, out rows, out beeps);
            tl.Add("{\"state\":" + Arr(sce.State) + ",\"elapsedMs\":" + sce.ElapsedMs + ",\"male\":" + (sce.Male ? "true" : "false")
                   + ",\"nudges\":[" + string.Join(",", sce.Nudges.Select(n => "[" + n[0] + "," + n[1] + "]")) + "]"
                   + ",\"steps\":" + Arr(sce.Steps) + ",\"snapshot\":" + snap
                   + ",\"blinks\":" + Arr(beeps.Take(40))
                   + ",\"rows\":[" + string.Join(",", rows.Select(r => Arr(r))) + "]}");
        }
        sb.Append("\"timeline\":[\n").Append(string.Join(",\n", tl)).Append("]\n}\n");
        System.IO.File.WriteAllText(args[1], sb.ToString());
        Console.WriteLine("ok: tinymt " + seeds.Count + ", search " + sc.Count + ", timeline " + tl.Count);
    }
}
