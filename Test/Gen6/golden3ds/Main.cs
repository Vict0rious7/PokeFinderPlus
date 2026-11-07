// Golden-file generator: runs 3DSRNGTool's own Gen 6 stationary code (Search6_Normal path, no UI)
// and dumps every result, so the PokeFinderPlus C++ port can be checked against it.
using System;
using System.IO;
using System.Linq;
using System.Text;
using Pk3DSRNGTool;
using Pk3DSRNGTool.Core;
using Pk3DSRNGTool.RNG;

static class GoldenMain
{
    static readonly string[] Games = { "X", "Y", "OR", "AS" };

    static void Main(string[] args)
    {
        string outPath = args.Length > 0 ? args[0] : "stationary6_golden.json";
        uint[] seeds = { 0x12345678, 0xDEADBEEF };
        // (sync nature or 0xFF, shiny charm, TSV, TRV, consider delay)
        var setups = new (byte sync, bool charm, int tsv, byte trv, bool delay, bool tinySync)[]
        {
            (0xFF, false, 1234, 0, false, false),
            (5,    true,  3000, 7, true, false),
            (17,   false, 0,    15, true, true),
        };
        var frameRanges = new (int min, int max)[] { (0, 29), (2000, 2004) };

        var sb = new StringBuilder();
        sb.Append("{\"source\":\"3DSRNGTool 1.0.6 (wwwwwwzx, MIT), Stationary6 via RNGPool.Generate6\",\"cases\":[\n");
        int cases = 0, rows = 0;
        for (int gv = 0; gv < 4; gv++)
        {
            var groups = Pokemon.getCategoryList(gv, 0);
            for (int gi = 1; gi < groups.Length; gi++)
            {
                int emitted = -1;
                foreach (var pm in Pokemon.getSpecFormList(gv, gi, 0))
                {
                    var pm6 = (PKM6)pm;
                    if (pm6.Conceptual || pm6.Bank)
                        continue;
                    emitted++;
                    foreach (var su in setups)
                    foreach (var seed in seeds)
                    foreach (var fr in frameRanges)
                    {
                        RNGPool.Clear();
                        RNGPool.PM = pm;
                        var setting = new Stationary6 { Synchro_Stat = su.sync, TSV = su.tsv, TRV = su.trv, ShinyCharm = su.charm };
                        setting.UseTemplate(pm);
                        RNGPool.igenerator = setting;
                        RNGPool.TinySynced = su.tinySync;
                        RNGPool.DelayTime = pm.Delay;
                        RNGPool.Considerdelay = su.delay;
                        int buffersize = 150 + (su.delay ? Math.Max(0, (int)pm.Delay) : 0);

                        var rng = new MersenneTwister(seed);
                        for (int i = 0; i < fr.min; i++)
                            rng.Next();
                        RNGPool.CreateBuffer(rng, buffersize);

                        if (cases++ > 0) sb.Append(",\n");
                        sb.Append($"{{\"game\":\"{Games[gv]}\",\"group\":\"{groups[gi].Text}\",\"category\":{gi - 1},\"pokemon\":{emitted},\"species\":{pm.Species},\"forme\":{pm.Forme},\"level\":{setting.Level}");
                        sb.Append($",\"delay\":{pm.Delay},\"syncNature\":{su.sync},\"shinyCharm\":{(su.charm ? "true" : "false")},\"tsv\":{su.tsv},\"trv\":{su.trv},\"considerDelay\":{(su.delay ? "true" : "false")},\"assumeSynced\":{(su.tinySync ? "true" : "false")}");
                        sb.Append($",\"seed\":{seed},\"min\":{fr.min},\"max\":{fr.max},\"results\":[");
                        for (int i = fr.min; i <= fr.max; i++, RNGPool.AddNext(rng))
                        {
                            var r = (Result6)RNGPool.Generate6();
                            if (i > fr.min) sb.Append(',');
                            // [frame, EC, PID, iv0..iv5, ability, nature, gender, shiny(0/1/2 square), synchronized]
                            sb.Append($"[{i},{r.EC},{r.PID},{string.Join(",", r.IVs)},{r.Ability},{r.Nature},{r.Gender},{(r.SquareShiny ? 2 : r.Shiny ? 1 : 0)},{(r.Synchronize ? 1 : 0)}]");
                            rows++;
                        }
                        sb.Append("]}");
                    }
                }
            }
        }
        sb.Append("\n]}\n");
        File.WriteAllText(outPath, sb.ToString());
        var tb = new StringBuilder("[\n");
        int n = 0;
        for (int gv = 0; gv < 4; gv++)
        {
            var groups = Pokemon.getCategoryList(gv, 0);
            for (int gi = 1; gi < groups.Length; gi++)
            {
                int idx = 0;
                foreach (var pm in Pokemon.getSpecFormList(gv, gi, 0))
                {
                    var pm6 = (PKM6)pm;
                    if (pm6.Conceptual || pm6.Bank) { idx++; continue; }
                    var st = new Stationary6 { Synchro_Stat = 0xFF };
                    st.UseTemplate(pm);
                    if (n++ > 0) tb.Append(",\n");
                    tb.Append($"{{\"game\":\"{Games[gv]}\",\"group\":\"{groups[gi].Text}\",\"groupIndex\":{gi},\"species\":{pm.Species},\"forme\":{pm.Forme},\"level\":{st.Level},\"alwaysSync\":{(st.AlwaysSync?1:0)},\"iv3\":{(st.IV3?1:0)},\"shinyLocked\":{(st.IsShinyLocked?1:0)},\"ability\":{st.Ability},\"gender\":{st.Gender},\"randomGender\":{(st.RandomGender?1:0)},\"nature\":{st.Synchro_Stat},\"ivs\":[{string.Join(",", st.IVs)}],\"delay\":{pm.Delay},\"instantSync\":{(pm6.InstantSync?1:0)},\"ottsv\":{(pm6.OTTSV.HasValue ? pm6.OTTSV.Value.ToString() : "-1")},\"gift\":{(pm.Gift?1:0)},\"egg\":{(pm.Egg?1:0)},\"unstable\":{(pm.Unstable?1:0)},\"syncable\":{(pm.Syncable?1:0)}}}");
                    idx++;
                }
            }
        }
        tb.Append("\n]\n");
        File.WriteAllText(Path.ChangeExtension(outPath, ".templates.json"), tb.ToString());
        Console.WriteLine($"{cases} cases, {rows} results -> {outPath}");
    }
}
