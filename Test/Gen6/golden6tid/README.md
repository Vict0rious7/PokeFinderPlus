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
