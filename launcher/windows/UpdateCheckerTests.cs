using System;
using System.IO;
using System.Net;
using System.Text;
namespace JfgLauncher
{
    internal static class UpdateCheckerTests
    {
        internal static int Run()
        {
            int checks = 0;
            Action<bool, string> check = delegate(bool passed, string message) { if (!passed) throw new Exception(message); checks++; };
            Func<string, string, UpdateChecker.Result> parse = delegate(string tag, string current) {
                return UpdateChecker.Check(current, delegate { return Encoding.UTF8.GetBytes("{\"tag_name\":\"" + tag + "\",\"draft\":false,\"prerelease\":false}"); });
            };
            check(parse("v1.1.0", "1.1.0").State == "current", "Equal release must not notify");
            check(parse("v1.0.0", "1.1.0").State == "current", "Older release must not downgrade");
            check(parse("v1.10.0", "1.9.9").State == "available", "Compare numeric components");
            check(parse("v2.0.0", "1.99.99").State == "available", "Major version update");
            check(parse("v1.1.1", "1.1.0").State == "available", "Patch version update");
            check(parse("1.2.0", "1.1.0").State == "available", "Optional v prefix");
            check(parse("v1.1.0", "1.1.0-beta.2").State == "available", "Stable replaces matching prerelease");
            check(parse("v1.1.0+build.9", "1.1.0+local.4").State == "current", "Build metadata is not an update");
            foreach (var tag in new[] { "", "latest", "v1.2", "v01.2.0", "v1.2.3.4", "v1.2.3-beta.1", "v1.2.3<script>", "v99999999999.0.0" })
                check(parse(tag, "1.0.0").State == "error", "Reject invalid stable tag " + tag);
            foreach (var json in new[] { "null", "{}", "<html>error</html>", "{\"tag_name\":\"v9.0.0\",\"draft\":true,\"prerelease\":false}", "{\"tag_name\":\"v9.0.0\",\"draft\":false,\"prerelease\":true}", "{\"tag_name\":\"v9.0.0\",\"draft\":false}" })
                check(UpdateChecker.Check("1.1.0", delegate { return Encoding.UTF8.GetBytes(json); }).State == "error", "Reject invalid API response");
            check(parse("v2.0.0", "bad-installed-version").State == "error", "Unknown installed version is not current");
            var offline = UpdateChecker.Check("1.1.0", delegate { throw new WebException("offline", WebExceptionStatus.ConnectFailure); });
            check(offline.State == "error" && offline.Message.Contains("offline"), "Offline failure must leave gameplay available");
            check(UpdateChecker.Check("1.1.0", delegate { return new byte[UpdateChecker.MaximumBytes + 1]; }).State == "error", "Reject oversized response");
            using (var stream = new MemoryStream(new byte[UpdateChecker.MaximumBytes + 1]))
                check(UpdateChecker.Check("1.1.0", delegate { return UpdateChecker.ReadBounded(stream); }).State == "error", "Bound streamed response");
            using (var stream = new MemoryStream(new byte[] { 1, 2, 3 })) check(UpdateChecker.ReadBounded(stream).Length == 3, "Read small response");
            check(parse("v2.0.0", "1.1.0").Wire().StartsWith("available\nv2.0.0\n"), "Frontend protocol");
            return checks;
        }
        private static int Main(string[] args)
        {
            Console.WriteLine("Update checker checks: " + Run());
            if (args.Length != 0 && args[0] == "--live") {
                var result = UpdateChecker.Check(UpdateChecker.InstalledVersion, UpdateChecker.Fetch);
                Console.Write(result.Wire());
                return result.State == "error" ? 1 : 0;
            }
            return 0;
        }
    }
}
