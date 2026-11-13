# golden6tid

Makes `Test/Gen6/xytid_6gentidsearch.json`, the reference data for `XYTidTest`, by running **6genTidSearch's own code**
(BZL, v1.02, `6genTidSearch.exe`, .NET Framework 4) by reflection: `TinyMT`, `BlinkTimer.CalcBlink / Search / HitFrame`
and `BlinkTimeLine` (`StaticSkip`, `GetRandFast`, `GetTinyFrameFast`, counters). The step events and the 計算 row loop
are driven with those primitives in the order read from the XY tab's handlers (`SYOUHI_STEP_Click`, `SYOUHI_CALC_Click`).
6genTidSearch's timeline normally runs on a stopwatch; here every event lands on an exact frame (an event due at frame w
applies before frame w is processed), the same choice as `XYTid::Session`.

6genTidSearch is not redistributed (BZL's terms); bring your own copy.

```sh
apt install mono-runtime mono-mcs          # Ubuntu; any Mono 6 works
mcs -langversion:7 Golden.cs -out:Golden.exe
mono Golden.exe /path/to/6genTidSearch.exe ../xytid_6gentidsearch.json
```

The case generator is seeded (20261112), so the file is reproducible from the same exe
(SHA-256 c5d0f61d…3ef40da, 3,853,824 bytes).

## ORAS

`GoldenOras.cs` makes `Test/Gen6/orastid_6gentidsearch.json`, the reference data for `OrasTidTest`, from 6genTidSearch's
ORAS tab (`OrasTidSearch.OrAsID`): TinyMT IDs (13 + "No" advances), the real 基準seed検索 search body
(`<KIZYUN_CALC_Click>b__6`) run on seeds 0 to 2^18 - 1 for 60 planted cases, and the real ズレ確認 handler (`button1_Click`)
for 200 cases. It needs Mono's WinForms and a display (`xvfb-run`).

```sh
apt install mono-runtime mono-mcs libmono-system-windows-forms4.0-cil xvfb
mcs -langversion:7 -r:System.Windows.Forms.dll -r:System.Drawing.dll GoldenOras.cs -out:GoldenOras.exe
xvfb-run -a mono GoldenOras.exe /path/to/6genTidSearch.exe ../orastid_6gentidsearch.json
```
