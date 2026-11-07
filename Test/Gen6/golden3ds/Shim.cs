// Test shim: replaces 3DSRNGTool's WinForms Program and embedded resources for a console build.
namespace Pk3DSRNGTool
{
    internal static class Program { public static MainFormStub mainform = null; }
    internal class MainFormStub { public int lindex = 0; }
}
namespace Pk3DSRNGTool.Properties
{
    internal static class Resources
    {
        static string Dir => System.IO.Path.Combine(System.AppContext.BaseDirectory, "res");
        internal static byte[] personal_ao => System.IO.File.ReadAllBytes(System.IO.Path.Combine(Dir, "personal_ao"));
        internal static byte[] personal_uu => System.IO.File.ReadAllBytes(System.IO.Path.Combine(Dir, "personal_uu"));
    }
}
