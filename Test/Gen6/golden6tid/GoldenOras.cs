// Golden data for PokeFinderPlus "IDs (OR / AS)", produced by running 6genTidSearch's own ORAS code (BZL) under Mono.
// Base seed search: the real search body (OrAsID.<>c__DisplayClass4_0.<KIZYUN_CALC_Click>b__6) on a reduced seed range.
// Offset check (ズレ確認): the real OrAsID.button1_Click.
// Usage: xvfb-run mono GoldenOras.exe 6genTidSearch.exe out.json
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Windows.Forms;

class GoldenOras
{
    const BindingFlags F = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static;
    static Assembly asm;
    static Type tTiny, tOras, tDc;
    static object oras;
    static Random rnd = new Random(20261113);

    static object Field(object o, string f) => o.GetType().GetField(f, F).GetValue(o);
    static void SetField(object o, string f, object v)
    {
        var fi = o.GetType().GetField(f, F);
        fi.SetValue(o, v is Array ? v : Convert.ChangeType(v, fi.FieldType));
    }

    static uint Id(uint seed, uint no)
    {
        var ctor = tTiny.GetConstructors(F).First(k => k.GetParameters().Length == 1 && k.GetParameters()[0].ParameterType.IsValueType);
        object mt = ctor.Invoke(new object[] { Convert.ChangeType(0, ctor.GetParameters()[0].ParameterType) });
        tTiny.GetMethods(F).First(m => m.Name == "init" && m.GetParameters()[0].ParameterType == typeof(uint)).Invoke(mt, new object[] { seed });
        var next = tTiny.GetMethods(F).First(m => m.Name == "Next" && m.GetParameters().Length == 1);
        next.Invoke(mt, new object[] { Convert.ChangeType(13u + no, next.GetParameters()[0].ParameterType) });
        return (uint)tTiny.GetMethod("Nextuint", F).Invoke(mt, null);
    }

    static void SetBox(string name, ulong v)
    {
        object box = Field(oras, name);
        box.GetType().GetProperty("Value", F).SetValue(box, v);
    }

    static List<string[]> Rows(string grid)
    {
        var dgv = (DataGridView)Field(oras, grid);
        var rows = new List<string[]>();
        foreach (DataGridViewRow r in dgv.Rows)
        {
            if (r.IsNewRow) continue;
            rows.Add(r.Cells.Cast<DataGridViewCell>().Select(c => c.Value == null ? "" : c.Value.ToString()).ToArray());
        }
        return rows;
    }

    static string J(IEnumerable<string[]> rows) =>
        "[" + string.Join(",", rows.Select(r => "[" + string.Join(",", r.Select(c => "\"" + c + "\"")) + "]")) + "]";

    // ---- base seed search, real body on seeds [0, range) ----
    static List<string[]> Search(uint range, uint[] tids, uint[] secs, uint gosa)
    {
        ((DataGridView)Field(oras, "KIZYUN_DGV")).Rows.Clear();
        object dc = Activator.CreateInstance(tDc, true);
        SetField(dc, "<>4__this", oras);
        SetField(dc, "proc", 1);
        var tms = Array.CreateInstance(tTiny, 1);
        SetField(dc, "tms", tms);
        SetField(dc, "per", new uint[1]);
        SetField(dc, "ps", 0);
        SetField(dc, "seedRnage", range);
        SetField(dc, "gosa", gosa);
        SetField(dc, "gosa2", gosa * 2);
        SetField(dc, "tid1", tids[0]); SetField(dc, "tid2", tids[1]); SetField(dc, "tid3", tids[2]); SetField(dc, "tid4", tids[3]);
        // KIZYUN_CALC_Click: time = seconds * 1000 - gosa (unsigned arithmetic)
        SetField(dc, "time2", unchecked(secs[1] * 1000u - gosa));
        SetField(dc, "time3", unchecked(secs[2] * 1000u - gosa));
        SetField(dc, "time4", unchecked(secs[3] * 1000u - gosa));
        var body = tDc.GetMethods(F).First(m => m.Name == "<KIZYUN_CALC_Click>b__6");
        body.Invoke(dc, new object[] { 0 }); // on the UI thread: Control.Invoke runs synchronously
        return Rows("KIZYUN_DGV");
    }

    // ---- ズレ確認, real handler ----
    static List<string[]> Offset(uint seed, uint tid, uint no, uint gosa)
    {
        SetBox("ZURE_KIZYUN", seed);
        SetBox("ZURE_SYUTUGEN", tid);
        SetBox("ZURE_SYOUHI", no);
        SetBox("ZURE_GOSA", gosa);
        tOras.GetMethod("button1_Click", F).Invoke(oras, new object[] { null, EventArgs.Empty });
        return Rows("ZURE_DGV");
    }

    static int Main(string[] args)
    {
        asm = Assembly.LoadFrom(args[0]);
        tTiny = asm.GetType("_6genTidSearch.TinyMT");
        tOras = asm.GetType("OrAsTidSearch.OrAsID");
        tDc = asm.GetType("OrAsTidSearch.OrAsID+<>c__DisplayClass4_0");
        oras = Activator.CreateInstance(tOras);
        ((Control)oras).CreateControl();
        var h = ((Control)oras).Handle;
        var zk = Field(oras, "ZURE_KIZYUN");
        Console.Error.WriteLine("ZURE_KIZYUN hex: " + zk.GetType().GetProperty("UseHexValue", F).GetValue(zk));

        var sb = new StringBuilder();
        sb.Append("{\"source\":\"6genTidSearch v1.02 OrAsID (BZL), run by reflection under Mono\",");

        // ID of a seed (TinyMT check of the model: 13 + No advances, then the ID)
        sb.Append("\"ids\":[");
        var idc = new List<string>();
        for (int c = 0; c < 200; c++)
        {
            uint s = (uint)rnd.Next() ^ ((uint)rnd.Next() << 16); uint no = (uint)rnd.Next(0, 300);
            idc.Add($"[{s},{no},{Id(s, no)}]");
        }
        sb.Append(string.Join(",", idc)).Append("],");

        // base seed search
        const uint RANGE = 1u << 18;
        sb.Append("\"searchRange\":" + RANGE + ",\"search\":[");
        var sc = new List<string>();
        for (int c = 0; c < 60; c++)
        {
            uint gosa = (uint)(c % 6 == 0 ? rnd.Next(0, 3) : rnd.Next(1, 1500));
            uint b = (uint)rnd.Next(0, (int)RANGE);
            uint[] secs = { 0, (uint)rnd.Next(1, 400), 0, 0 };
            secs[2] = secs[1] + (uint)rnd.Next(1, 400); secs[3] = secs[2] + (uint)rnd.Next(1, 400);
            if (c == 7) { secs[1] = 0; secs[2] = 0; secs[3] = 1; } // degenerate times
            var tids = new uint[4];
            var errs = new int[4];
            tids[0] = Id(b, 0) & 0xffff;
            for (int k = 1; k < 4; k++)
            {
                int g = (int)gosa;
                int e;
                int kind = rnd.Next(0, 10);
                if (kind == 0) e = -g; else if (kind == 1) e = g - 1; else if (kind == 2) e = g; else if (kind == 3) e = -g - 1;
                else e = g == 0 ? 0 : rnd.Next(-g, g);
                errs[k] = e;
                tids[k] = Id(unchecked((uint)(b + secs[k] * 1000 + e)), 0) & 0xffff;
            }
            if (c % 10 == 9) tids[rnd.Next(1, 4)] = (uint)rnd.Next(0, 65536); // a wrong TID
            var rows = Search(RANGE, tids, secs, gosa);
            sc.Add($"{{\"planted\":{b},\"errors\":[{string.Join(",", errs)}],\"tids\":[{string.Join(",", tids)}],\"seconds\":[{string.Join(",", secs)}],\"tolerance\":{gosa},\"rows\":{J(rows)}}}");
            Console.Error.WriteLine($"search {c}: {rows.Count} rows");
        }
        sb.Append(string.Join(",", sc)).Append("],");

        // offset check
        sb.Append("\"offset\":[");
        var oc = new List<string>();
        for (int c = 0; c < 200; c++)
        {
            uint s = (uint)rnd.Next() ^ ((uint)rnd.Next() << 16);
            if (c % 20 == 0) s = (uint)rnd.Next(0, 3000);          // wrap below 0
            if (c % 20 == 1) s = 0xffffffffu - (uint)rnd.Next(0, 3000); // wrap above 2^32
            uint gosa = (uint)(c % 7 == 0 ? rnd.Next(0, 3) : rnd.Next(1, 3000));
            uint no = (uint)rnd.Next(0, 60);
            int e = gosa == 0 ? 0 : rnd.Next(-(int)gosa, (int)gosa + 1);
            uint tid = c % 5 == 4 ? (uint)rnd.Next(0, 65536) : Id(unchecked((uint)(s + e)), no) & 0xffff;
            var rows = Offset(s, tid, no, gosa);
            oc.Add($"{{\"seed\":{s},\"tid\":{tid},\"no\":{no},\"tolerance\":{gosa},\"rows\":{J(rows)}}}");
        }
        sb.Append(string.Join(",", oc)).Append("]}");
        System.IO.File.WriteAllText(args[1], sb.ToString());
        return 0;
    }
}
