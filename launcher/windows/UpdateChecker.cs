using System;
using System.IO;
using System.Net;
using System.Reflection;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;

namespace JfgLauncher
{
    // Runs only in a separate, launcher-owned helper process. No credentials or
    // profile data are sent; the response never supplies an executable or URL.
    internal static class UpdateChecker
    {
        internal const string Endpoint = "https://api.github.com/repos/TK22-26/JetForceGemini-Recomp/releases/latest";
        internal const int MaximumBytes = 1024 * 1024;
        [DataContract]
        private sealed class Release
        {
            [DataMember(Name = "tag_name", IsRequired = true)] public string Tag { get; set; }
            [DataMember(Name = "draft", IsRequired = true)] public bool Draft { get; set; }
            [DataMember(Name = "prerelease", IsRequired = true)] public bool Prerelease { get; set; }
        }
        internal sealed class Result
        {
            internal string State, Tag, Message;
            internal Result(string state, string tag, string message) { State = state; Tag = tag; Message = message; }
            internal string Wire() { return State + "\n" + Tag + "\n" + Message + "\n"; }
        }
        internal static string InstalledVersion
        {
            get { return ((AssemblyInformationalVersionAttribute)Attribute.GetCustomAttribute(
                typeof(UpdateChecker).Assembly, typeof(AssemblyInformationalVersionAttribute))).InformationalVersion; }
        }
        private static Version ParseVersion(string value, bool allowPrerelease)
        {
            if (value == null || value.Length > 100) throw new InvalidDataException("Invalid release version.");
            var match = Regex.Match(value, @"^v?((?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*))(-[0-9A-Za-z.-]+)?(\+[0-9A-Za-z.-]+)?\z");
            Version version;
            if (!match.Success || (!allowPrerelease && match.Groups[2].Success) ||
                !Version.TryParse(match.Groups[1].Value, out version))
                throw new InvalidDataException("Invalid release version.");
            return version;
        }
        internal static Result Interpret(byte[] bytes, string installed)
        {
            if (bytes == null || bytes.Length == 0 || bytes.Length > MaximumBytes)
                throw new InvalidDataException("Invalid release response size.");
            Release release;
            using (var input = new MemoryStream(bytes, false))
                release = (Release)new DataContractJsonSerializer(typeof(Release)).ReadObject(input);
            if (release == null || release.Draft || release.Prerelease)
                throw new InvalidDataException("No stable release was returned.");
            var latest = ParseVersion(release.Tag, false);
            var current = ParseVersion(installed, true);
            bool newer = latest > current || (latest == current && installed.Split('+')[0].Contains("-"));
            return new Result(newer ? "available" : "current", release.Tag,
                newer ? "A newer stable release is ready to download." :
                "You are up to date. No newer stable release is available.");
        }
        internal static byte[] ReadBounded(Stream stream)
        {
            using (var output = new MemoryStream())
            {
                byte[] buffer = new byte[8192];
                int count;
                while ((count = stream.Read(buffer, 0, buffer.Length)) != 0)
                {
                    if (output.Length + count > MaximumBytes) throw new InvalidDataException("Release response is too large.");
                    output.Write(buffer, 0, count);
                }
                return output.ToArray();
            }
        }
        internal static byte[] Fetch()
        {
            ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;
            var request = (HttpWebRequest)WebRequest.Create(Endpoint);
            request.UserAgent = "JFG-Recomp-Launcher/" + InstalledVersion;
            request.Accept = "application/vnd.github+json";
            request.Headers["X-GitHub-Api-Version"] = "2022-11-28";
            request.Timeout = 10000;
            request.ReadWriteTimeout = 10000;
            request.AllowAutoRedirect = false;
            // Cap the whole request, including slow/chunked responses.
            using (var deadline = new Timer(delegate { request.Abort(); }, null, 20000, Timeout.Infinite))
            using (var response = (HttpWebResponse)request.GetResponse())
            {
                if (response.StatusCode != HttpStatusCode.OK) throw new InvalidDataException("Unexpected release response.");
                if (response.ContentLength > MaximumBytes) throw new InvalidDataException("Release response is too large.");
                using (var stream = response.GetResponseStream()) return ReadBounded(stream);
            }
        }
        internal static Result Check(string installed, Func<byte[]> fetch)
        {
            try { return Interpret(fetch(), installed); }
            catch (WebException error)
            {
                using (var response = error.Response as HttpWebResponse)
                {
                    if (response != null && ((int)response.StatusCode == 403 || (int)response.StatusCode == 429))
                        return new Result("error", "", "GitHub is limiting update checks. Try again later or view releases.");
                    if (response != null && response.StatusCode == HttpStatusCode.NotFound)
                        return new Result("error", "", "No published stable release could be found. Try again later or view releases.");
                }
                return new Result("error", "", "Could not reach GitHub. Check your connection and try again. You can keep playing offline.");
            }
            catch (Exception error)
            {
                if (!(error is InvalidDataException || error is IOException || error is SerializationException || error is ArgumentException || error is InvalidOperationException)) throw;
                return new Result("error", "", "Could not read the latest release information. Try again later or view releases.");
            }
        }
    }
}
