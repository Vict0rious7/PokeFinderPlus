# golden3ds (PokeFinderPlus)

Makes `Test/Gen6/static6_3dsrngtool.json`, the reference the Gen 6 static generator is tested against, by running
**3DSRNGTool 1.0.6's own code** (github.com/wwwwwwzx/3DSRNGTool, MIT) without its Windows UI:
`Stationary6` through `RNGPool.Generate6`, the path of 3DSRNGTool's "Normal" Gen 6 search.

```sh
git clone --branch 1.0.6 https://github.com/wwwwwwzx/3DSRNGTool.git /tmp/3DSRNGTool
dotnet run -p:RngTool=/tmp/3DSRNGTool/3DSRNGTool -- ../static6_3dsrngtool.json
```

- Needs the .NET 8 SDK only (no NuGet packages; `nuget.config` clears the sources). Runs on Linux and macOS.
- `Shim.cs` replaces the two WinForms-only pieces the core touches (`Program.mainform`, the embedded personal tables).
- It also writes `static6_3dsrngtool.templates.json` (every Gen 6 static template after 3DSRNGTool's `UseTemplate`),
  which `Core/Gen6/Encounters6.cpp` was generated from.
- Cases: every X, Y, OR, AS static template (Pokémon Link / Transporter excluded), 2 seeds, 3 settings
  (sync nature, shiny charm, TSV/TRV, delay considered, TinyMT sync assumed), advances 0–29 and 2000–2004.
