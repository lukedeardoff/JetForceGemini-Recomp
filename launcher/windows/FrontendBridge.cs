using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace JfgLauncher
{
// Setup and diagnostic services for the native frontend. No launcher window.
internal static class FrontendBridge
{
    private sealed class Owner : IWin32Window
    {
        public IntPtr Handle { get; private set; }
        internal Owner(IntPtr h)
        {
            Handle = h;
        }
    }
    private static string root;
    private static void State(string message)
    {
        // Several independent tool processes may report status. Each write is atomic.
        try
        {
            Write("frontend-status.txt", message.Replace("\r", "").Replace("\n", " ") + "\n");
        }
        catch (IOException)
        {
        }
        catch (UnauthorizedAccessException)
        {
        }
    }
    private static string Read(string name)
    {
        string path = Path.Combine(root, name);
        if (!File.Exists(path))
            return "";
        if (new FileInfo(path).Length > 65536)
            throw new InvalidDataException("The saved frontend setting is too large.");
        return File.ReadAllText(path, Encoding.UTF8).TrimEnd('\r', '\n');
    }
    private static void Write(string name, string value)
    {
        string path = Path.Combine(root, name),
               temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            File.WriteAllText(temporary, value, new UTF8Encoding(false));
            if (File.Exists(path))
                File.Replace(temporary, path, null);
            else
                File.Move(temporary, path);
        }
        finally
        {
            if (File.Exists(temporary))
                File.Delete(temporary);
        }
    }
    internal static void CheckRuntimeProtocol(string game, int timeout)
    {
        using (var check = new Process())
        {
            check.StartInfo = new ProcessStartInfo(
                game, "--frontend-version") { UseShellExecute = false, CreateNoWindow = true,
                                              RedirectStandardOutput = true, RedirectStandardError = true };
            check.Start();
            var stdout = check.StandardOutput.ReadToEndAsync();
            var stderr = check.StandardError.ReadToEndAsync();
            if (!check.WaitForExit(timeout))
            {
                try
                {
                    check.Kill();
                }
                catch (InvalidOperationException)
                {
                }
                throw new InvalidDataException(
                    "The bundled game did not answer the launcher. Extract a fresh copy of the release ZIP.");
            }
            if (!Task.WaitAll(new Task[] { stdout, stderr }, timeout) || check.ExitCode != 0 ||
                stdout.Result.Trim() != "jfg-frontend-1")
                throw new InvalidDataException(
                    "The bundled game is incompatible with this launcher. Extract the complete release ZIP.");
        }
    }
    internal static string MapDirectory(string profile) {
        if(!InventorySession.IsActive(profile))return Path.Combine(profile,"no-active-live-session");
        string marker=Path.Combine(profile,"frontend-map-session.txt");
        if(File.Exists(marker)) {
            string current=File.ReadAllText(marker,Encoding.UTF8).Trim();
            // An ordinary game run must never display an older mod session as current.
            return current=="disabled"?Path.Combine(profile,"no-live-map-export"):current;
        }
        return File.Exists(Path.Combine(profile,"live.json"))?profile:Path.Combine(profile,"no-active-live-session");
    }
    private static void PrepareBackground() {
        try {InventoryImages.ExportShips(Read("frontend-rom.txt"),Path.Combine(root,"frontend-ships.bin"));}
        catch(InvalidDataException){}catch(IOException){}catch(UnauthorizedAccessException){}catch(ArgumentException){}
    }
    internal static void SaveDirectShortcut(string path, string profile, string launcher)
    {
        if (!String.Equals(Path.GetExtension(path), ".lnk", StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException("Choose a Windows shortcut (.lnk) filename.");
        if (String.IsNullOrWhiteSpace(launcher) || !File.Exists(launcher))
            throw new InvalidDataException("Open JFG-Launcher.exe to save a quick-launch shortcut.");
        launcher = LocalSetup.FullPath(launcher);
        object shell = null, shortcut = null;
        try
        {
            var type = Type.GetTypeFromProgID("WScript.Shell", true);
            shell = Activator.CreateInstance(type);
            shortcut = type.InvokeMember("CreateShortcut", System.Reflection.BindingFlags.InvokeMethod, null, shell, new object[] { path });
            var shortcutType = shortcut.GetType();
            Action<string, object> set = delegate(string key, object value) {
                shortcutType.InvokeMember(key, System.Reflection.BindingFlags.SetProperty, null, shortcut, new object[] { value });
            };
            set("TargetPath", launcher);
            set("Arguments", "--play --profile " + LocalSetup.Quote(LocalSetup.FullPath(profile)));
            set("WorkingDirectory", Path.GetDirectoryName(launcher));
            set("IconLocation", launcher + ",0");
            set("Description", "Play Jet Force Gemini with its game menu using your profile.");
            set("WindowStyle", 1);
            shortcutType.InvokeMember("Save", System.Reflection.BindingFlags.InvokeMethod, null, shortcut, new object[0]);
        }
        finally
        {
            if (shortcut != null) Marshal.FinalReleaseComObject(shortcut);
            if (shell != null) Marshal.FinalReleaseComObject(shell);
        }
    }

    private static void SaveShortcutDialog(IntPtr owner)
    {
        LocalSetup.ValidateRuntime(Read("frontend-runtime.txt"));
        if (!File.Exists(Read("frontend-rom.txt")))
            throw new InvalidDataException("Choose your ROM before saving a direct-launch shortcut.");
        using (var dialog = new SaveFileDialog())
        {
            dialog.Title = "Save direct-launch shortcut";
            dialog.Filter = "Windows shortcut (*.lnk)|*.lnk";
            dialog.DefaultExt = "lnk";
            dialog.AddExtension = true;
            dialog.OverwritePrompt = true;
            dialog.CheckPathExists = true;
            dialog.RestoreDirectory = true;
            dialog.FileName = "Jet Force Gemini.lnk";
            if (dialog.ShowDialog(new Owner(owner)) != DialogResult.OK) return;
            SaveDirectShortcut(dialog.FileName, root, Environment.GetEnvironmentVariable("JFG_LAUNCHER_EXE"));
            State("Direct-launch shortcut saved: " + dialog.FileName);
        }
    }

    // Explicit diagnostic arguments are forwarded after ordinary launch environment
    // sanitization. An inherited JFG_* variable must never activate a replay.
    // Each explicit diagnostic launch owns a fresh directory. Ordinary Play
    // continues to strip inherited JFG_* variables and never enables recording.
    internal static string ConfigureDiagnostics(ProcessStartInfo start, string destination)
    {
        string[] variables = { "JFG_GAMEPLAY_TRACE", "JFG_PHASE8_PROGRESS",
            "JFG_PHASE9_INPUT_RECORD", "JFG_PHASE9_ACTOR_TRACE",
            "JFG_PHASE95_NATIVE_POLL_TRACE", "JFG_PHASE8_FRAME_CAPTURE",
            "JFG_PHASE8_RDRAM_CAPTURE", "JFG_PHASE8_INPUT_REPLAY",
            "JFG_RT64_INTERPOLATION_TRACE" };
        foreach (string variable in variables)
            if (start.EnvironmentVariables.ContainsKey(variable))
                throw new InvalidDataException("Diagnostics cannot be combined with duplicate recording or replay options.");
        string directory = Path.Combine(LocalSetup.FullPath(destination),
            DateTime.UtcNow.ToString("yyyyMMdd-HHmmss") + "-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(directory);
        start.EnvironmentVariables["JFG_GAMEPLAY_TRACE"] = Path.Combine(directory, "gameplay.tsv");
        start.EnvironmentVariables["JFG_PHASE8_PROGRESS"] = Path.Combine(directory, "progress.json");
        start.EnvironmentVariables["JFG_PHASE9_INPUT_RECORD"] = Path.Combine(directory, "input.txt");
        start.EnvironmentVariables["JFG_PHASE9_ACTOR_TRACE"] = "1";
        start.EnvironmentVariables["JFG_RT64_INTERPOLATION_TRACE"] = Path.Combine(directory, "interpolation.tsv");
        start.EnvironmentVariables["JFG_PHASE95_NATIVE_POLL_TRACE"] = Path.Combine(directory, "polls.tsv");
        // Periodic GPU screenshots stall the game thread and contaminate FPS capture.
        // Keep explicit developer framebuffer probes separate from route logging.
        start.EnvironmentVariables["JFG_PHASE8_RDRAM_CAPTURE"] = Path.Combine(directory, "final-rdram.bin");
        return directory;
    }

    internal static void ApplyReplayOptions(ProcessStartInfo start, string[] args)
    {
        for (int i = 4; i < args.Length; i += 2)
        {
            if (args[1] != "play" || i + 1 >= args.Length)
                throw new InvalidDataException("Replay options require a play action and a file path.");
            if (args[i] == "--diagnostics") { ConfigureDiagnostics(start, args[i + 1]); continue; }
            if (args[i] == "--input-replay" && start.EnvironmentVariables.ContainsKey("JFG_PHASE9_INPUT_RECORD"))
                throw new InvalidDataException("A live diagnostic capture cannot replay inputs.");
            string variable;
            if (args[i] == "--input-replay") variable = "JFG_PHASE8_INPUT_REPLAY";
            else if (args[i] == "--progress-output") variable = "JFG_PHASE8_PROGRESS";
            else throw new InvalidDataException("Unknown replay option.");
            if (start.EnvironmentVariables.ContainsKey(variable))
                throw new InvalidDataException("Duplicate replay option.");
            string path = LocalSetup.FullPath(args[i + 1]);
            if (args[i] == "--input-replay" && !File.Exists(path))
                throw new InvalidDataException("The input recording was not found.");
            if (args[i] == "--progress-output" && !Directory.Exists(Path.GetDirectoryName(path)))
                throw new InvalidDataException("The progress output folder was not found.");
            start.EnvironmentVariables[variable] = path;
        }
    }

    private static async Task<int> Execute(string action, IntPtr owner, string[] args)
    {
        if (action == "updates") {
            var result = await Task.Run(delegate { return UpdateChecker.Check(UpdateChecker.InstalledVersion, UpdateChecker.Fetch); });
            Write("frontend-update.txt", result.Wire());
            return 0;
        }
        if (action == "shortcut") { SaveShortcutDialog(owner); return 0; }
        if ((action == "play" || action == "direct-play") && InventorySession.IsActive(root))
            throw new InvalidOperationException("The game is already running for this profile.");
        if (action == "map-data" || action == "inventory-data") return await NativeLiveTools.Run(root,owner,action == "inventory-data");
        if (action == "assets") {await Task.Run((Action)PrepareBackground);return 0;}
        if (action == "sessions" || action == "export")
        {
            var choices = SupportSession.Sessions(SupportSession.Root);
            var list = new StringBuilder();
            foreach (var item in choices)
                list.Append(Path.GetFileName(item.Path)).Append('\t').Append(item.Label.Replace("\t", " ").Replace("\n", " ")).Append('\n');
            Write("frontend-sessions.tsv", list.ToString());
            if (action == "export")
            {
                string selection = Read("frontend-report.txt");
                var selected = choices.Find(delegate(SupportChoice item) { return Path.GetFileName(item.Path) == selection; });
                if (selected == null) throw new InvalidDataException("Select an available session first.");
                string zip = SupportSession.ExportSelected(SupportSession.Root, selected.Path, Path.Combine(FirstRun.Root, "support-exports"));
                State("Support ZIP ready: " + zip);
                Process.Start(new ProcessStartInfo("explorer.exe", "/select,\"" + zip + "\"") { UseShellExecute = true });
            }
            return 0;
        }
        if (action == "controllers")
        {
            using (var f = new ControllerWindow(root)) f.ShowDialog(new Owner(owner));
            return 0;
        }
        if (action == "audio")
        {
            using (var f = new FrontendAudio(root)) f.ShowDialog(new Owner(owner));
            return 0;
        }
        if (action == "support")
        {
            using (var f = new SupportWindow(SupportSession.Root,
                                             Path.Combine(FirstRun.Root, "support-exports"), null))
                f.ShowDialog(new Owner(owner));
            return 0;
        }
        if (action == "map" || action == "inventory") {
            using(var tool=NativeLiveTools.OpenWindow(action,root,owner))await Task.Run(delegate{tool.WaitForExit();});
            return 0;
        }
        if (action == "import-rom")
        {
            State("Importing your ROM. This only needs to be done once...");
            string imported = await Task.Run(delegate { return ImportedRom.Import(root, Read("frontend-rom-source.txt")); });
            Write("frontend-rom.txt", imported);
            var saved = LocalSetup.LoadSettings(root);
            saved.RomPath = imported;
            LocalSetup.SaveSettings(root, saved);
            await Task.Run((Action)PrepareBackground);
            File.Delete(Path.Combine(root, "frontend-rom-source.txt"));
            State("ROM imported. Click Play. The original file is no longer needed.");
            return 0;
        }
        if (action == "init")
        {
            var settings = LocalSetup.LoadSettings(root);
            if (!File.Exists(Path.Combine(root, "frontend-rom.txt")))
                Write("frontend-rom.txt", settings.RomPath ?? "");
            string stored = ImportedRom.FileName(root);
            string selected = Read("frontend-rom.txt");
            if (File.Exists(stored) || File.Exists(selected)) {
                State("Checking your local ROM...");
                string imported = await Task.Run(delegate { return ImportedRom.Import(root, File.Exists(stored) ? stored : selected); });
                Write("frontend-rom.txt", imported);
                settings.RomPath = imported;
                LocalSetup.SaveSettings(root, settings);
            }
            else if (!File.Exists(selected)) Write("frontend-rom.txt", "");
            // Always resolve against this package, including after moving or updating it.
            Write("frontend-runtime.txt", PortablePackage.FromLauncher());
            if (!File.Exists(ControllerProfile.FileName(root)))
                new ControllerProfile().Save(root);
            await Task.Run((Action)PrepareBackground);
            State(File.Exists(Read("frontend-rom.txt")) ? "Ready. Your ROM is imported. Click Play." : "Import your ROM once, then click Play. No build tools are required.");
            return 0;
        }
        string rom = await Task.Run(delegate { return ImportedRom.Import(root, Read("frontend-rom.txt")); });
        Write("frontend-rom.txt", rom);
        State("Verifying your ROM...");
        using (var verified = await Task.Run(delegate { return LocalSetup.OpenVerifiedRom(rom); }))
        {
            using (var support = new FrontendSupport(action == "play" ? "launch" : action))
            {
                if (action == "setup")
                {
                    State("Checking game files...");
                    string bundled = await Task.Run((Func<string>)PortablePackage.FromLauncher);
                    await Task.Run(delegate { CheckRuntimeProtocol(bundled, 10000); });
                    Write("frontend-runtime.txt", bundled);
                    LocalSetup.SaveSettings(root, new Settings { RomPath = rom, RuntimePath = bundled });
                    State("ROM and game files verified. Click Play.");
                    return 0;
                }
                string game = await Task.Run((Func<string>)PortablePackage.FromLauncher);
                Write("frontend-runtime.txt", game);
                CheckRuntimeProtocol(game, 10000);
                LocalSetup.SaveSettings(root, new Settings { RomPath = rom, RuntimePath = game });
                using (var process = new Process())
                {
                    bool mods = Read("frontend-mods.txt") == "1";
                    process.StartInfo = mods ? LocalSetup.NavigationStartInfo(game, rom, root)
                                             : LocalSetup.LiveToolsStartInfo(game, rom, root);
                    ApplyReplayOptions(process.StartInfo, args);
                    process.StartInfo.EnvironmentVariables["JFG_CONTROLLER_CONFIG"] =
                        ControllerProfile.FileName(root);
                    process.StartInfo.EnvironmentVariables["JFG_MASTER_VOLUME_CONFIG"] =
                        AudioPreferences.FileName(root);
                    if (owner != IntPtr.Zero)
                        process.StartInfo.EnvironmentVariables["JFG_FRONTEND_PARENT"] =
                            owner.ToInt64().ToString(System.Globalization.CultureInfo.InvariantCulture);
                    process.StartInfo.EnvironmentVariables["JFG_SUPPORT_LOG"] = support.Session.NativePath;
                    string gameplayTrace = process.StartInfo.EnvironmentVariables["JFG_GAMEPLAY_TRACE"];
                    if (!String.IsNullOrEmpty(gameplayTrace))
                    {
                        string capture = Path.GetDirectoryName(gameplayTrace);
                        var metadata = new StringBuilder();
                        metadata.AppendLine("started_utc=" + DateTime.UtcNow.ToString("o"));
                        metadata.AppendLine("runtime=" + game);
                        metadata.AppendLine("runtime_sha256=" + SupportSession.Hash(game));
                        metadata.AppendLine("profile=" + root);
                        metadata.AppendLine("support=" + support.Session.DirectoryPath);
                        metadata.AppendLine("live_tools=" + process.StartInfo.EnvironmentVariables[mods ? "JFG_MOD_OUTPUT" : "JFG_LIVE_OUTPUT"]);
                        metadata.AppendLine("logs=gameplay,timing,actor,camera,input,polls,interpolation,support,final-frame,final-rdram");
                        metadata.AppendLine("performance_directory=" + Path.Combine(root, "performance"));
                        metadata.AppendLine("capture_boundary=Play starts a fresh session; launcher waiting is not recorded.");
                        metadata.AppendLine("analysis=Exclude initial gameplay idle from active-route timing; preserve original input timestamps for replay.");
                        metadata.AppendLine("note=Detailed logging can affect timings; compare against a normal session.");
                        File.WriteAllText(Path.Combine(capture, "session.txt"), metadata.ToString());
                        string initial = Path.Combine(capture, "initial-profile");
                        Directory.CreateDirectory(initial);
                        foreach (string source in Directory.GetFiles(root))
                        {
                            string name = Path.GetFileName(source);
                            if (name.EndsWith(".ini", StringComparison.OrdinalIgnoreCase) ||
                                name == "jfg.flash" || name == "controller-1.pak" || name == "launcher.json")
                                File.Copy(source, Path.Combine(initial, name));
                        }
                        Write("frontend-diagnostics-session.txt", capture);
                    }
                    try
                    {
                        support.Session.RuntimeDetails(game, root);
                    }
                    catch (IOException)
                    {
                    }
                    string mapDirectory=process.StartInfo.EnvironmentVariables[mods?"JFG_MOD_OUTPUT":"JFG_LIVE_OUTPUT"];
                    Write("frontend-map-session.txt",mapDirectory);
                    process.Exited+=delegate {try {File.Delete(Path.Combine(root,"frontend-map-session.txt"));}catch(IOException){}catch(UnauthorizedAccessException){}};
                    process.EnableRaisingEvents=true;
                    process.Start();
                    InventorySession.Record(root,process);
                    InventorySession.Record(mapDirectory,process);
                    var stdout = support.Session.Drain(process.StandardOutput);
                    var stderr = support.Session.Drain(process.StandardError);
                    State(String.IsNullOrEmpty(gameplayTrace) ? "Playing. F11 fullscreen. Esc settings."
                        : "Recording diagnostics. F11 fullscreen. Esc settings.");
                    string captureRequest = Path.Combine(root, "frontend-capture.request");
                    if (File.Exists(captureRequest)) File.Delete(captureRequest);
                    while (!process.HasExited)
                    {
                        if (File.Exists(captureRequest))
                        {
                            File.Delete(captureRequest);
                            try
                            {
                                bool captured = await support.Session.CaptureFreeze(process, game);
                                State(captured ? "Freeze captured. Create a support ZIP for this session." : "Freeze capture unavailable. Existing logs are preserved.");
                            }
                            catch (Exception e) { State("Freeze capture failed: " + LocalSetup.FriendlyError(e)); }
                        }
                        await Task.Delay(200);
                    }
                    await Task.WhenAll(stdout, stderr);
                    support.Session.Exit(process.ExitCode);
                    State(process.ExitCode == 0 ? "Game stopped. Your saves are ready for next time."
                                                : "Game exited unexpectedly. Open Help > Support report.");
                    return process.ExitCode;
                }
            }
        }
    }
    private sealed class FrontendSupport : IDisposable
    {
        internal readonly SupportSession Session;
        internal FrontendSupport(string action)
        {
            Session = new SupportSession(SupportSession.Root, action);
        }
        public void Dispose()
        {
            Session.Finish();
        }
    }
    internal static bool TryRun(string[] args)
    {
        if (args.Length < 4 || args[0] != "--frontend-worker")
            return false;
        System.Threading.Mutex launchLock = null;
        try
        {
            if (args.Length > 4 && args[1] != "play")
                throw new InvalidDataException("Replay options require a play action.");
            root = LocalSetup.FullPath(args[2]);
            Directory.CreateDirectory(root);
            long handle;
            if (!Int64.TryParse(args[3], out handle))
                throw new InvalidDataException("Invalid frontend owner.");
            if (Array.IndexOf(
                    new[] { "updates", "init", "import-rom", "assets", "setup", "play", "direct-play", "shortcut", "controllers", "audio", "support", "map", "inventory", "map-data", "inventory-data", "sessions", "export" },
                    args[1]) < 0)
                throw new InvalidDataException("Unknown frontend action.");
            if (args[1] == "play" || args[1] == "direct-play" || args[1] == "import-rom" || args[1] == "init") {
                string key;
                using (var hash = System.Security.Cryptography.SHA256.Create())
                    key = BitConverter.ToString(hash.ComputeHash(Encoding.UTF8.GetBytes(root.ToUpperInvariant()))).Replace("-", "");
                launchLock = new System.Threading.Mutex(false, @"Local\JFGGameProfile-" + key);
                bool acquired;
                try { acquired = launchLock.WaitOne(0); }
                catch (System.Threading.AbandonedMutexException) { acquired = true; }
                if (!acquired) { launchLock.Dispose(); launchLock = null; throw new InvalidOperationException("The game is already starting or running for this profile."); }
            }
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Environment.ExitCode = Execute(args[1], new IntPtr(handle), args).GetAwaiter().GetResult();
        }
        catch (Exception error)
        {
            if (root != null)
                State(LocalSetup.FriendlyError(error));
            Environment.ExitCode = 1;
            if (args[1] == "direct-play")
                MessageBox.Show(LocalSetup.FriendlyError(error), "Jet Force Gemini", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        finally { if (launchLock != null) { launchLock.ReleaseMutex(); launchLock.Dispose(); } }
        return true;
    }
}
internal sealed class FrontendAudio : ApplicationWindow
{
    internal FrontendAudio(string root)
    {
        Text = "Audio";
        ClientSize = new Size(440, 155);
        Font = new Font("Segoe UI", 11);
        StartPosition = FormStartPosition.CenterParent;
        var settings = AudioPreferences.Load(root);
        var value =
            new Label { Left = 24, Top = 15, Width = 390, Text = "Master volume: " + settings.Volume + "%" };
        var volume = new TrackBar { Left = 20,
                                    Top = 45,
                                    Width = 390,
                                    Minimum = 0,
                                    Maximum = 100,
                                    TickFrequency = 10,
                                    Value = settings.Volume };
        var mute =
            new CheckBox { Left = 24, Top = 108, Width = 170, Text = "Mute", Checked = settings.Muted };
        Controls.Add(value);
        Controls.Add(volume);
        Controls.Add(mute);
        EventHandler save = delegate
        {
            try
            {
                new AudioPreferences { Volume = volume.Value, Muted = mute.Checked }.Save(root);
                value.Text = "Master volume: " + volume.Value + "%";
            }
            catch (Exception e)
            {
                value.Text = LocalSetup.FriendlyError(e);
            }
        };
        volume.ValueChanged += save;
        mute.CheckedChanged += save;
    }
}
}
