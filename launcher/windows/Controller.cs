using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace JfgLauncher
{
    internal sealed class ControllerProfile
    {
        internal int Device = -1, Stick, Deadzone = 7849, Threshold = 8689, Trigger = 3855, InvertX, InvertY;
        internal string DeviceKey = "";
        internal int[] Bindings = { 0, 1, 23, 6, 11, 12, 13, 14, 9, 10, 22, 21, 20, 19 };
        internal static string FileName(string root) { return FileName(root, 0); }
        internal static string FileName(string root, int port) { if(port<0||port>=4)throw new ArgumentOutOfRangeException("port"); return Path.Combine(root, port == 0 ? "controller.ini" : "controller-" + (port + 1) + ".ini"); }
        internal string Encode()
        {
            StringBuilder text = new StringBuilder(Device == -4 ? "version=2\n" : "version=1\n");
            string[] keys = { "device", "stick", "deadzone", "threshold", "trigger", "invert_x", "invert_y" };
            int[] values = { Device, Stick, Deadzone, Threshold, Trigger, InvertX, InvertY };
            for (int i = 0; i < keys.Length; ++i) text.Append(keys[i]).Append('=').Append(values[i].ToString(CultureInfo.InvariantCulture)).Append('\n');
            for (int i = 0; i < Bindings.Length; ++i) text.Append("map").Append(i).Append('=').Append(Bindings[i].ToString(CultureInfo.InvariantCulture)).Append('\n');
            if(Device == -4) text.Append("device_key=").Append(DeviceKey).Append('\n');
            return text.ToString();
        }
        private static int Number(Dictionary<string,string> fields,string key) {
            int number;string value=fields[key];
            if(!Int32.TryParse(value,NumberStyles.AllowLeadingSign,CultureInfo.InvariantCulture,out number) || number.ToString(CultureInfo.InvariantCulture)!=value)
                throw new InvalidDataException("The controller profile has an invalid number.");
            return number;
        }
        internal static bool ValidDeviceKey(string key) {
            if(key==null || key.Length!=68 || !key.StartsWith("sdl:",StringComparison.Ordinal))return false;
            for(int i=4;i<key.Length;++i)if(!((key[i]>='0'&&key[i]<='9')||(key[i]>='a'&&key[i]<='f')))return false;
            return true;
        }
        internal static ControllerProfile Parse(string text)
        {
            if(String.IsNullOrEmpty(text) || text.Length > 4096) throw new InvalidDataException("The controller profile is invalid. Restore defaults in Controllers.");
            Dictionary<string,string> fields = new Dictionary<string,string>(StringComparer.Ordinal);
            string trimmed = text.EndsWith("\n",StringComparison.Ordinal) ? text.Substring(0,text.Length-1) : text;
            foreach(string raw in trimmed.Split('\n')) {
                string line=raw.EndsWith("\r",StringComparison.Ordinal)?raw.Substring(0,raw.Length-1):raw;
                int at=line.IndexOf('=');
                if(at<1 || line.Length>128 || fields.ContainsKey(line.Substring(0,at)))throw new InvalidDataException("The controller profile is invalid.");
                fields.Add(line.Substring(0,at),line.Substring(at+1));
            }
            try {
                ControllerProfile p=new ControllerProfile();int version=Number(fields,"version");
                p.Device=Number(fields,"device");p.Stick=Number(fields,"stick");p.Deadzone=Number(fields,"deadzone");
                p.Threshold=Number(fields,"threshold");p.Trigger=Number(fields,"trigger");p.InvertX=Number(fields,"invert_x");p.InvertY=Number(fields,"invert_y");
                if(version==1) {
                    if(fields.Count!=22 || fields.ContainsKey("device_key") || p.Device==-4)throw new InvalidDataException();
                } else if(version==2) {
                    if(fields.Count!=23 || p.Device!=-4 || !fields.TryGetValue("device_key",out p.DeviceKey) || !ValidDeviceKey(p.DeviceKey))throw new InvalidDataException();
                } else throw new InvalidDataException();
                for(int i=0;i<14;++i)p.Bindings[i]=Number(fields,"map"+i.ToString(CultureInfo.InvariantCulture));
                if(p.Device < -4 || p.Device > 3 || p.Stick < 0 || p.Stick > 1 || p.Deadzone < 0 || p.Deadzone > 30000 ||
                   p.Threshold < 1000 || p.Threshold > 32000 || p.Trigger < 1000 || p.Trigger > 32000 ||
                   p.InvertX < 0 || p.InvertX > 1 || p.InvertY < 0 || p.InvertY > 1)throw new InvalidDataException();
                foreach(int binding in p.Bindings)if(binding < -1 || binding > 26)throw new InvalidDataException();
                return p;
            } catch(KeyNotFoundException) {throw new InvalidDataException("The controller profile is incomplete. Restore defaults in Controllers.");}
        }
        internal static ControllerProfile Load(string root) { return Load(root, 0); }
        internal static ControllerProfile Load(string root, int port)
        {
            string path = FileName(root, port);
            if (!File.Exists(path)) return new ControllerProfile { Device = -1 };
            if (new FileInfo(path).Length > 4096) throw new InvalidDataException("The controller profile is too large. Restore defaults in Controllers.");
            return Parse(File.ReadAllText(path));
        }
        internal void ValidateAssignment(string root,int port) {
            for(int p=0;p<4;++p) {
                if(p==port)continue;var other=Load(root,p);
                if((Device>=0 || Device==-3 || Device==-4) && other.Device==Device && (Device!=-4 || other.DeviceKey==DeviceKey))
                    throw new InvalidDataException("That device is already assigned to Player " + (p+1) + ". Disconnect that assignment first.");
            }
        }
        internal void Save(string root) { Save(root, 0); }
        internal void Save(string root, int port)
        {
            string text = Encode(); Parse(text);
            Directory.CreateDirectory(root);
            string path = FileName(root, port), temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try
            {
                File.WriteAllText(temporary, text, new UTF8Encoding(false));
                if (File.Exists(path)) File.Replace(temporary, path, null); else File.Move(temporary, path);
            }
            finally { if (File.Exists(temporary)) File.Delete(temporary); }
        }
    }

    internal static class ControllerInput
    {
        [StructLayout(LayoutKind.Sequential)] internal struct Pad { internal ushort Buttons; internal byte LT, RT; internal short LX, LY, RX, RY; }
        [StructLayout(LayoutKind.Sequential)] internal struct State { internal uint Packet; internal Pad Pad; }
        [DllImport("xinput1_4.dll", EntryPoint = "XInputGetState")] private static extern uint GetState(uint index, out State state);
        internal static bool Read(int device, out bool[] buttons, out int[] axes, out int connected)
        {
            buttons = new bool[15]; axes = new int[6]; connected = -1;
            if (device < -1) return false;
            try
            {
                for (int index = 0; index < 4; ++index)
                {
                    if (device != -1 && device != index) continue;
                    State state;
                    if (GetState((uint)index, out state) != 0) continue;
                    connected = index;
                    ushort[] masks = { 0x1000, 0x2000, 0x4000, 0x8000, 0x20, 0, 0x10, 0x40, 0x80, 0x100, 0x200, 1, 2, 4, 8 };
                    for (int i = 0; i < masks.Length; ++i) buttons[i] = (state.Pad.Buttons & masks[i]) != 0;
                    axes = new int[] { state.Pad.LX, -(int)state.Pad.LY, state.Pad.RX, -(int)state.Pad.RY,
                        state.Pad.LT * 32767 / 255, state.Pad.RT * 32767 / 255 };
                    return true;
                }
            }
            catch (DllNotFoundException) { }
            catch (EntryPointNotFoundException) { }
            return false;
        }
        internal static int Pressed(bool[] buttons, int[] axes)
        {
            for (int i = 0; i < buttons.Length; ++i) if (buttons[i]) return i;
            for (int i = 0; i < axes.Length; ++i)
                if (Math.Abs(axes[i]) > 20000) return 15 + i * 2 + (axes[i] < 0 ? 1 : 0);
            return -1;
        }
    }

    internal sealed class ControllerWindow : ApplicationWindow
    {
        private readonly ComboBox device = new ComboBox(), stick = new ComboBox();
        private readonly ComboBox[] bindings = new ComboBox[14];
        private readonly NumericUpDown deadzone = Percent(0, 90), threshold = Percent(5, 97), trigger = Percent(5, 97);
        private readonly CheckBox invertX = new CheckBox { Text = "Invert horizontal", AutoSize = true }, invertY = new CheckBox { Text = "Invert vertical", AutoSize = true };
        private readonly Label status = new Label { Dock = DockStyle.Fill };
        private readonly Timer timer = new Timer { Interval = 50 };
        private readonly string root;
        private int port;
        private string savedDeviceKey="";
        private readonly TabControl ports = new TabControl { Dock = DockStyle.Top, Height = 28 };
        private int learning = -1;
        private bool released;
        private DateTime learnUntil, noticeUntil;
        private readonly List<Control> mappingControls = new List<Control>();
        private readonly List<Button> learnButtons = new List<Button>();
        private static readonly string[] BindingNames = { "Unbound", "A", "B", "X", "Y", "Back / View", "Guide (reserved)", "Start / Menu", "Left stick click", "Right stick click", "Left bumper", "Right bumper", "D-pad up", "D-pad down", "D-pad left", "D-pad right", "Left stick right", "Left stick left", "Left stick down", "Left stick up", "Right stick right", "Right stick left", "Right stick down", "Right stick up", "Left trigger", "Left trigger negative", "Right trigger", "Right trigger negative" };
        private static NumericUpDown Percent(int min, int max) { return new NumericUpDown { Minimum = min, Maximum = max, DecimalPlaces = 1, Increment = 1, Width = 70 }; }
        internal ControllerWindow(string profileRoot)
        {
            root = profileRoot; Text = "Controller mapping"; Font = new Font("Segoe UI", 10);
            ClientSize = new Size(840, 530); MinimumSize = new Size(856, 569); StartPosition = FormStartPosition.CenterParent;
            TableLayoutPanel layout = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(16), ColumnCount = 1, RowCount = 6 };
            layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 80)); layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 38));
            layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100)); layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 74));
            layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 52)); layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 44)); Controls.Add(layout);
            Panel header = new Panel { Dock = DockStyle.Fill };
            for (int p = 0; p < 4; ++p) ports.TabPages.Add("Player " + (p + 1));
            header.Controls.Add(new Label { Dock = DockStyle.Bottom, Height = 42, Text = "Assign one device to each player. Apply before changing tabs.\nLive changes take effect immediately; Learn waits for release before recording input." });
            header.Controls.Add(ports); layout.Controls.Add(header, 0, 0);
            FlowLayoutPanel selection = new FlowLayoutPanel { Dock = DockStyle.Fill };
            device.DropDownStyle = ComboBoxStyle.DropDownList; device.Width = 200;
            device.Items.AddRange(new object[] { "Automatic (one per player)", "Controller 1", "Controller 2", "Controller 3", "Controller 4", "Disconnected", "Keyboard", "Saved SDL controller (use native mapping)" });
            selection.Controls.Add(device); selection.Controls.Add(new Label { Text = "Movement stick", AutoSize = true, Padding = new Padding(10, 4, 0, 0) });
            stick.DropDownStyle = ComboBoxStyle.DropDownList; stick.Items.AddRange(new object[] { "Left stick", "Right stick" }); selection.Controls.Add(stick);
            layout.Controls.Add(selection, 0, 1);
            TableLayoutPanel maps = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 6, RowCount = 7 };
            for (int row = 0; row < 7; ++row) maps.RowStyles.Add(new RowStyle(SizeType.Percent, 100F / 7));
            for (int column = 0; column < 2; ++column) { maps.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 18)); maps.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 32)); maps.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 60)); }
            string[] names = { "A / jump", "B", "Z / fire", "Start / pause", "D-pad up", "D-pad down", "D-pad left", "D-pad right", "L", "R", "C up", "C down", "C left", "C right" };
            for (int i = 0; i < 14; ++i)
            {
                int index = i, column = i / 7 * 3, row = i % 7;
                maps.Controls.Add(new Label { Text = names[i], Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft }, column, row);
                bindings[i] = new ComboBox { Dock = DockStyle.Fill, DropDownStyle = ComboBoxStyle.DropDownList }; bindings[i].Items.AddRange(BindingNames);
                maps.Controls.Add(bindings[i], column + 1, row);
                Button learn = new Button { Text = "Learn", Dock = DockStyle.Fill }; learnButtons.Add(learn);
                learn.Click += delegate { learning = index; released = false; learnUntil = DateTime.UtcNow.AddSeconds(10); status.Text = "Release controls, then press the input for " + names[index] + "."; };
                maps.Controls.Add(learn, column + 2, row);
            }
            layout.Controls.Add(maps, 0, 2); mappingControls.Add(maps); mappingControls.Add(stick);
            FlowLayoutPanel options = new FlowLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(0, 8, 0, 0) };
            options.Controls.Add(new Label { Text = "Movement dead zone %", AutoSize = true }); options.Controls.Add(deadzone);
            options.Controls.Add(new Label { Text = "Stick button threshold %", AutoSize = true }); options.Controls.Add(threshold);
            options.Controls.Add(new Label { Text = "Trigger threshold %", AutoSize = true }); options.Controls.Add(trigger);
            options.SetFlowBreak(trigger, true); options.Controls.Add(invertX); options.Controls.Add(invertY); layout.Controls.Add(options, 0, 3); mappingControls.Add(options);
            layout.Controls.Add(status, 0, 4);
            FlowLayoutPanel actions = new FlowLayoutPanel { Dock = DockStyle.Fill };
            Button save = new Button { Text = "Apply mapping", Width = 140, Height = 32 };
            save.Click += delegate { try { var profile = ReadProfile();
                    profile.ValidateAssignment(root,port);
                    profile.Save(root,port); status.Text="Player " + (port+1) + " mapping applied."; noticeUntil=DateTime.UtcNow.AddSeconds(3); } catch (Exception error) { status.Text = LocalSetup.FriendlyError(error); noticeUntil=DateTime.UtcNow.AddSeconds(5); } };
            Button defaults = new Button { Text = "Restore defaults", Width = 140, Height = 32 };
            defaults.Click += delegate { Apply(new ControllerProfile { Device=-1 }); learning = -1; };
            Button cancel = new Button { Text = "Close", Width = 100, Height = 32, DialogResult = DialogResult.Cancel };
            actions.Controls.Add(save); actions.Controls.Add(defaults); actions.Controls.Add(cancel); layout.Controls.Add(actions, 0, 5); CancelButton = cancel;
            device.SelectedIndexChanged += delegate {
                learning=-1;noticeUntil=DateTime.MinValue;
                foreach(Control c in mappingControls)c.Enabled=device.SelectedIndex<5;
                foreach(Button b in learnButtons)b.Enabled=device.SelectedIndex>=1 && device.SelectedIndex<=4;
            };
            try { Apply(ControllerProfile.Load(root)); } catch (Exception) { Apply(new ControllerProfile()); status.Text = "The saved mapping could not be read. Save to restore defaults."; }
            ports.SelectedIndexChanged += delegate { port=ports.SelectedIndex; learning=-1;noticeUntil=DateTime.MinValue; try { Apply(ControllerProfile.Load(root,port)); } catch(Exception error) { Apply(new ControllerProfile { Device=-1 }); status.Text=LocalSetup.FriendlyError(error); } };
            timer.Tick += delegate { Poll(); }; timer.Start(); FormClosed += delegate { timer.Dispose(); };
        }
        private void Apply(ControllerProfile p)
        {
            savedDeviceKey=p.DeviceKey;
            device.SelectedIndex = p.Device == -4 ? 7 : p.Device == -2 ? 5 : p.Device == -3 ? 6 : p.Device + 1; stick.SelectedIndex = p.Stick;
            deadzone.Value = Math.Min(deadzone.Maximum, p.Deadzone * 100M / 32767);
            threshold.Value = Math.Max(threshold.Minimum, Math.Min(threshold.Maximum, p.Threshold * 100M / 32767)); trigger.Value = Math.Max(trigger.Minimum, Math.Min(trigger.Maximum, p.Trigger * 100M / 32767));
            invertX.Checked = p.InvertX != 0; invertY.Checked = p.InvertY != 0;
            for (int i = 0; i < 14; ++i) bindings[i].SelectedIndex = p.Bindings[i] + 1;
        }
        private ControllerProfile ReadProfile()
        {
            ControllerProfile p = new ControllerProfile { DeviceKey = device.SelectedIndex==7?savedDeviceKey:"", Device = device.SelectedIndex == 7 ? -4 : device.SelectedIndex == 5 ? -2 : device.SelectedIndex == 6 ? -3 : device.SelectedIndex - 1, Stick = stick.SelectedIndex,
                Deadzone = (int)(deadzone.Value * 32767 / 100), Threshold = (int)(threshold.Value * 32767 / 100), Trigger = (int)(trigger.Value * 32767 / 100),
                InvertX = invertX.Checked ? 1 : 0, InvertY = invertY.Checked ? 1 : 0 };
            for (int i = 0; i < 14; ++i) p.Bindings[i] = bindings[i].SelectedIndex - 1; return p;
        }
        private void Poll()
        {
            if(DateTime.UtcNow<noticeUntil)return;
            if(device.SelectedIndex==7){status.Text="Use the native Controller Mapping window to configure this SDL device.";return;}
            if(device.SelectedIndex==5){status.Text="Player "+(port+1)+" is disconnected. Apply mapping to unplug this port.";return;}
            if(device.SelectedIndex==6){status.Text="Keyboard: WASD move; Space/Z = A; X = B; C = Z; Enter = Start.\nQ/E = L/R; IJKL = C buttons; arrows = D-pad; Shift = full stick.";return;}
            if(device.SelectedIndex==0){status.Text="Automatic assigns a different connected controller to each player.\nSelect Controller 1-4 to preview a specific device or use Learn.";return;}
            bool[] buttons; int[] axes; int connected;
            bool found = ControllerInput.Read(ReadProfile().Device, out buttons, out axes, out connected);
            int pressed = ControllerInput.Pressed(buttons, axes);
            if (learning >= 0)
            {
                if (DateTime.UtcNow > learnUntil) { learning = -1; status.Text = "Learn timed out. Click Learn to try again."; return; }
                if (!found) return;
                if (pressed < 0) released = true;
                else if (released) { bindings[learning].SelectedIndex = pressed + 1; learning = -1; }
            }
            else status.Text = found ? "Input preview: Controller " + (connected + 1) + ". " + (pressed < 0 ? "Move a stick or press a button to test it." : "Input: " + BindingNames[pressed + 1]) :
                "No XInput controller detected. Connect one to test. You can still edit and save mappings.\nOther controller types need an XInput-compatible driver or adapter.";
        }
    }
}
