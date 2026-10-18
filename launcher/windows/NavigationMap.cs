using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Windows.Forms;

namespace JfgLauncher
{
    [DataContract] internal sealed class MapFace
    {
        [DataMember] public int[] v = null;
        [DataMember] public float[] normal = null;
    }
    [DataContract] internal sealed class MapGeometry
    {
        internal MapGeometry BaseGeometry;
        internal string EntitySignature;
        [DataMember] public int schema = 0;
        [DataMember] public uint level = 0;
        [DataMember] public long generation = 0;
        [DataMember] public float[][] vertices = null;
        [DataMember] public MapFace[] triangles = null;
    }
    [DataContract] internal sealed class MapPlayerMotion {
        [DataMember] public bool known=false,hang_entry=false,grab_entry=false;
        [DataMember] public int state_id=0,animation_id=0;
        [DataMember] public float animation_frame=0;
    }
    [DataContract] internal sealed class MapPlayer
    {
        [DataMember] public MapPlayerMotion motion=null;
        [DataMember] public int? yaw=null;
        [DataMember] public uint address = 0;
        [DataMember] public float[] position = null;
    }
    [DataContract] internal sealed class MapMarker
    {
        [DataMember] public float[] position = null;
        [DataMember] public string kind = "";
        [DataMember] public string label = "";
        [DataMember] public uint address = 0;
        [DataMember] public int destination_code = 0;
        [DataMember] public uint radius = 0;
        [DataMember] public float plane_d = 0;
        [DataMember] public int directional = 0;
        [DataMember] public float[] normal = null;
    }
    [DataContract] internal sealed class MapInventory
    {
        [DataMember] public bool known = false;
        [DataMember] public int? character = null;
        [DataMember] public bool? red_key = null;
        [DataMember] public int? weapons_mask = null;
        internal string Summary { get {
            if (!known) return "Inventory: unknown";
            return "Character " + character + " | Red key: " + (red_key == true ? "owned" : "missing")
                + "\r\nMachine gun: " + ((weapons_mask.Value & 4) != 0 ? "owned" : "missing");
        } }
    }
    [DataContract] internal sealed class MapNpcCondition
    {
        [DataMember] public string domain = "", description = "", state = "";
        [DataMember] public int id = -1;
    }
    [DataContract] internal sealed class MapNpcOffer
    {
        [DataMember] public string id = "", kind = "", reward = "", status = "", scope = "";
        [DataMember] public int action = -1, item = -1, weapon = -1, flag = -1, destination = -1, cost = 0;
        [DataMember] public int[] consumed_items = null;
        [DataMember] public MapNpcCondition[] conditions = null;
        internal void Validate()
        {
            foreach (string text in new string[] { id, kind, reward, status, scope })
                if (String.IsNullOrEmpty(text) || text.Length > 256) throw new InvalidDataException("Invalid NPC offer text.");
            if (status != "owned" && status != "available" && status != "blocked" && status != "unknown")
                throw new InvalidDataException("Invalid NPC offer status.");
            if (action < -1 || action > 18 || item < -1 || item > 26 || weapon < -1 || weapon > 14 ||
                flag < -1 || flag > 1151 || destination < -1 || destination > 4095 || cost < 0 || cost > 32767 ||
                conditions == null || conditions.Length > 40 || consumed_items == null || consumed_items.Length > 27)
                throw new InvalidDataException("Invalid NPC offer values.");
            foreach (int value in consumed_items) if (value < 0 || value > 26) throw new InvalidDataException("Invalid trade item.");
            foreach (MapNpcCondition c in conditions) {
                if (c == null || String.IsNullOrEmpty(c.description) || c.description.Length > 512 ||
                    (c.domain != "dialogue_row" && c.domain != "visibility" && c.domain != "prerequisite") ||
                    (c.state != "met" && c.state != "missing" && c.state != "unknown"))
                    throw new InvalidDataException("Invalid NPC prerequisite.");
            }
        }
        internal string Details { get {
            string result = reward + " [" + status + "] (" + kind.Replace('_', ' ') + ")";
            if (scope != "none") result += "\r\nOwnership: " + scope.Replace('_', ' ');
            foreach (MapNpcCondition c in conditions) result += "\r\n  " + c.description + ": " + c.state;
            if (cost > 0) result += "\r\nCost: " + cost + " tokens";
            if (consumed_items.Length > 0) result += "\r\nTrade items are consumed when accepted.";
            if (kind == "transition") result += "\r\nScene effects require separate verification.";
            return result;
        } }
    }
    [DataContract] internal sealed class MapNpcCatalog
    {
        [DataMember] public bool known = false;
        [DataMember] public int dialogue_groups = 0, choice_tables = 0;
    }
    [DataContract] internal sealed class MapChestAccess {
        [DataMember] public bool known=false;
        [DataMember] public float[] point=null;
        [DataMember] public float radius=0,max_height=0;
        [DataMember] public int facing=0;
    }
    [DataContract] internal sealed class MapInteraction
    {
        [DataMember] public MapChestAccess activation=null;
        [DataMember] public float talk_radius=0,talk_lower=0,talk_upper=0;
        [DataMember] public uint address = 0, linked_actor = 0;
        [DataMember] public float[] position = null;
        [DataMember] public string kind = "", label = "", action = "", status = "";
        [DataMember] public string requirement = "", reward = "", traversal = "";
        [DataMember] public bool requirement_known = false;
        [DataMember] public bool access_known=false,access_allowed=false,condition_known=false,condition_met=false;
        [DataMember] public uint pending_openers=0,approach_radius=0;
        [DataMember] public bool npc_catalog_known = false;
        [DataMember] public MapNpcOffer[] offers = null;
        internal string OfferDetails { get {
            if (offers == null || offers.Length == 0) return "";
            string result = "\r\n\r\nNPC offers (alternative dialogue paths):";
            foreach (MapNpcOffer offer in offers) result += "\r\n\r\n" + offer.Details;
            return result;
        } }
        [DataMember] public int reward_item = -1, reward_weapon = -1, required_weapon = -1;
        [DataMember] public int spoken = -1, encounter = -1, dialogue = -1;
        [DataMember] public int raw_state = -1, raw_condition = -1;
        [DataMember] public int door_id = -1, required_item = -1, target_health = -1, target_max_health = -1, reset_ticks = -1;
        [OnDeserializing] private void Init(StreamingContext context) {
            reward_item = reward_weapon = required_weapon = spoken = encounter = dialogue = raw_state = raw_condition = -1;
            door_id = required_item = target_health = target_max_health = reset_ticks = -1;
        }
        internal string ExplorationStatus = "";
        internal string Caption { get { return label + " [" + status.Replace('_', ' ') + "]" + (String.IsNullOrEmpty(ExplorationStatus) ? "" : " - " + ExplorationStatus); } }
        public override string ToString() { return Caption; }
        internal string Details { get {
            return Caption + "\r\n\r\nAction: " + action.Replace('_', ' ')
                + "\r\nRequires: " + requirement + (requirement_known ? "" : " (unverified)")
                + "\r\nReward: " + reward
                + (door_id < 0 ? "" : "\r\nDoor group: " + door_id)
                + (target_health < 0 ? "" : "\r\nTarget strength: " + target_health + " / " + target_max_health + " (raw)")
                + (spoken < 0 ? "" : "\r\nSpoken to: " + (spoken == 1 ? "yes" : "no"))
                + OfferDetails
                + "\r\n\r\nX " + position[0].ToString("0.##") + "  Y " + position[1].ToString("0.##") + "  Z " + position[2].ToString("0.##")
                + (linked_actor == 0 ? "" : "\r\nRelated actor (exit: nearby door): " + linked_actor.ToString("X8"))
                + (pending_openers==0?"":"\r\nPending opener groups: "+pending_openers)
                + (approach_radius==0?"":"\r\nProximity radius: "+approach_radius)
                + "\r\n\r\nPhysical route: checked separately from door requirements.";
        } }
    }
    [DataContract] internal sealed class MapProgression
    {
        [DataMember] public int schema = 0;
        [DataMember] public MapInventory inventory = null;
        [DataMember] public MapNpcCatalog npc_catalog = null;
        [DataMember] public MapInteraction[] nodes = null;
        internal void Validate()
        {
            if (schema != 1 || inventory == null || nodes == null || nodes.Length > 4096)
                throw new InvalidDataException("Invalid progression data.");
            if (inventory.known && (!inventory.character.HasValue || inventory.character < 0 || inventory.character > 2 ||
                !inventory.red_key.HasValue || !inventory.weapons_mask.HasValue || inventory.weapons_mask < 0 || inventory.weapons_mask > 65535))
                throw new InvalidDataException("Invalid inventory.");
            if (!inventory.known && (inventory.character.HasValue || inventory.red_key.HasValue || inventory.weapons_mask.HasValue))
                throw new InvalidDataException("Unknown inventory contains ownership claims.");
            if (npc_catalog != null && (npc_catalog.known ? npc_catalog.dialogue_groups != 45 || npc_catalog.choice_tables < 0 || npc_catalog.choice_tables > 107
                : npc_catalog.dialogue_groups != 0 || npc_catalog.choice_tables != 0)) throw new InvalidDataException("Invalid NPC catalog coverage.");
            HashSet<uint> seen = new HashSet<uint>();
            foreach (MapInteraction node in nodes) {
                if (node == null || node.address == 0 || !seen.Add(node.address)) throw new InvalidDataException("Invalid interaction identity.");
                if (node.offers != null) {
                    if (node.offers.Length > 128 || (!node.npc_catalog_known && node.offers.Length != 0)) throw new InvalidDataException("Invalid NPC offers.");
                    HashSet<string> offerIds = new HashSet<string>();
                    foreach (MapNpcOffer offer in node.offers) {
                        if (offer == null) throw new InvalidDataException("Missing NPC offer.");
                        offer.Validate();
                        if (!offerIds.Add(offer.id)) throw new InvalidDataException("Duplicate NPC offer.");
                    }
                }
                MapSnapshot.Point(node.position);
                if(node.activation!=null&&node.activation.known) {
                    MapSnapshot.Point(node.activation.point);
                    if(node.action!="open_chest"||Single.IsNaN(node.activation.radius)||node.activation.radius<1||node.activation.radius>256||
                       Single.IsNaN(node.activation.max_height)||node.activation.max_height<0||node.activation.max_height>256||
                       node.activation.facing< -32768||node.activation.facing>32767)throw new InvalidDataException("Invalid chest activation region.");
                }
                foreach (string text in new string[] { node.kind, node.label, node.action, node.status, node.requirement, node.reward })
                    if (String.IsNullOrEmpty(text) || text.Length > 160) throw new InvalidDataException("Invalid interaction text.");
                if(node.pending_openers>255||node.approach_radius>65535||(!node.access_known&&node.access_allowed)||
                   (!node.condition_known&&node.condition_met))throw new InvalidDataException("Invalid door access facts.");
                if (node.traversal != "unknown" || node.spoken < -1 || node.spoken > 1 ||
                    node.reward_weapon < -1 || node.reward_weapon > 14 || node.required_weapon < -1 || node.required_weapon > 14)
                    throw new InvalidDataException("Unsupported interaction state.");
            }
        }
    }

    [DataContract] internal sealed class MapAi {
        [DataMember] public string state = "off";
        [DataMember] public bool active = false;
        [DataMember] public int waypoint = 0, count = 0, jump_attempts = 0;
        [DataMember] public long nonce = 0, manual_inputs = 0, confirmations = 0;
    }
    [DataContract] internal sealed class MapLive
    {
        [DataMember] public int schema = 0;
        [DataMember] public uint level = 0;
        [DataMember] public long generation = 0;
        [DataMember] public long timestamp_ms = 0;
        [DataMember] public long update = 0;
        [DataMember] public bool mesh_ready = false;
        [DataMember] public bool clearing_active = false;
        [DataMember] public bool mods_available = false, mod_warp_exits = false,
            mod_infinite_health = false, mod_instant_kill = false;
        [DataMember] public bool gameplay_active = false, navigation_enabled = false;
        [DataMember] public bool scripted_camera = false;
        [DataMember] public bool transition_confirm = false;
        [DataMember] public MapPlayer player = null;
        [DataMember] public MapMarker[] exits = null;
        [DataMember] public MapMarker[] markers = null;
        [DataMember] public MapMarker[] npcs = null;
        [DataMember] public MapProgression progression = null;
        [DataMember] public MapDialogue dialogue=null;
        [DataMember] public InventoryTracker inventory_tracker=null;
        [DataMember] public MapAi navigation_ai = null;
        [DataMember] public MapActor[] actors = null;
        [DataMember] public MapCollision collision = null;
        [DataMember] public MapJump box_jump = null;
    }
    internal sealed class MapMods {
        internal bool Warp, Health, Kill;
        internal const string WarpHelp = "Double-click an exit marker on the map or an exit in the inspector to enter the next room. Normal exit requirements still apply.";
        internal const string HealthHelp = "Keep your current character at full health and prevent damage during gameplay.";
        internal const string KillHelp = "Automatically defeat loaded ordinary enemies during gameplay. Tribals and friendly NPCs are spared; defeated enemies stay defeated when switched off.";
        internal static MapMods Load(string directory, MapLive live) {
            var value = new MapMods { Warp=live!=null&&live.mod_warp_exits, Health=live!=null&&live.mod_infinite_health, Kill=live!=null&&live.mod_instant_kill };
            string path=Path.Combine(directory,"mods.txt");
            if(!File.Exists(path))return value;
            if(new FileInfo(path).Length>128)throw new InvalidDataException("Invalid Mods settings.");
            string[] parts=File.ReadAllText(path).Split((char[])null,StringSplitOptions.RemoveEmptyEntries);
            if(parts.Length!=4||parts[0]!="JFGMODS1")throw new InvalidDataException("Invalid Mods settings.");
            for(int i=1;i<4;i++)if(parts[i]!="0"&&parts[i]!="1")throw new InvalidDataException("Invalid Mods setting.");
            value.Warp=parts[1]=="1";value.Health=parts[2]=="1";value.Kill=parts[3]=="1";return value;
        }
        private static void Write(string directory,string name,string text) {
            string path=Path.Combine(directory,name),temp=path+"."+Guid.NewGuid().ToString("N")+".tmp";
            File.WriteAllText(temp,text+"\n",new System.Text.UTF8Encoding(false));
            if(File.Exists(path))File.Replace(temp,path,null);else File.Move(temp,path);
        }
        internal void Save(string directory) { Write(directory,"mods.txt","JFGMODS1 "+(Warp?"1":"0")+" "+(Health?"1":"0")+" "+(Kill?"1":"0")); }
        internal void WarpTo(string directory,MapSnapshot map,uint address,long nonce,bool paused) {
            if(!Warp||map==null||(!map.IsLive&&!paused)||!map.Live.mods_available||
                !map.Live.gameplay_active||map.Live.scripted_camera||map.Live.exits==null||
                !Array.Exists(map.Live.exits,delegate(MapMarker exit){return exit.address==address;}))
                throw new InvalidDataException("Warp requires a loaded exit in active gameplay and the Warp to exits mod.");
            Write(directory,"warp-exit.txt",String.Format(System.Globalization.CultureInfo.InvariantCulture,
                "JFGWARP1 {0} {1} {2} {3} {4}",map.Live.level,map.Live.generation,nonce,NavigationExplorer.Clock,address));
        }
    }
    // Profile preferences are available before a game connects. Only a concrete
    // read/write error, unsupported live response or failed acknowledgement locks
    // the controls for that connection; a new session can retry normally.
    internal sealed class MapModPreferences {
        private readonly string settingsDirectory;
        private string session="";
        private bool saved, applied, awaiting;
        private long sentUpdate;
        internal MapMods Values=new MapMods();
        internal bool Failed {get;private set;}
        internal int ErrorVersion {get;private set;}
        internal MapModPreferences(string profile) {settingsDirectory=Path.Combine(profile,"map-settings");Load();}
        private static bool Expected(Exception e) {return e is IOException||e is UnauthorizedAccessException||e is InvalidDataException;}
        private void Load() {
            try {saved=File.Exists(Path.Combine(settingsDirectory,"mods.txt"));Values=MapMods.Load(settingsDirectory,null);}
            catch(Exception e){if(!Expected(e))throw;Fail();}
        }
        private void Fail() {if(!Failed)++ErrorVersion;Failed=true;awaiting=false;}
        internal void Choose(MapMods value) {
            try {
                Directory.CreateDirectory(settingsDirectory);value.Save(settingsDirectory);
                Values=value;saved=true;applied=false;awaiting=false;Failed=false;
            }catch(Exception e){if(!Expected(e))throw;Fail();}
        }
        internal void Sync(string directory,MapLive live,bool active) {
            string next=active ? directory??"" : "";
            if(next!=session){session=next;Failed=false;applied=awaiting=false;Load();}
            if(Failed||String.IsNullOrEmpty(session)||live==null)return;
            try {
                if(!live.mods_available){Fail();return;}
                if(!applied) {
                    if(saved)Values.Save(session);else Values=MapMods.Load(session,live);
                    applied=true;awaiting=saved;sentUpdate=live.update;
                }
                bool matches=Values.Warp==live.mod_warp_exits&&Values.Health==live.mod_infinite_health&&Values.Kill==live.mod_instant_kill;
                if(matches)awaiting=false;
                // Pauses/loading do not advance updates, so they cannot time out.
                if(awaiting&&live.update-sentUpdate>=18)Fail();
            }catch(Exception e){if(!Expected(e))throw;Fail();}
        }
        internal static void PrepareSession(string profile,string directory) {
            string settings=Path.Combine(profile,"map-settings");
            if(!File.Exists(Path.Combine(settings,"mods.txt")))return;
            try {var value=MapMods.Load(settings,null);Directory.CreateDirectory(directory);value.Save(directory);}
            catch(Exception e){if(!Expected(e))throw;} // The map reports the failure when it connects.
        }
    }
    internal sealed class MapSnapshot
    {
        internal MapGeometry Mesh;
        internal MapLive Live;
        internal bool IsLive {
            get {
                double age = (DateTime.UtcNow - new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc)).TotalMilliseconds - Live.timestamp_ms;
                return age >= 0 && age <= 5000;
            }
        }
        internal static void Point(float[] value)
        {
            if (value == null || value.Length != 3) throw new InvalidDataException("Invalid map position.");
            foreach (float v in value)
                if (Single.IsNaN(v) || Single.IsInfinity(v)) throw new InvalidDataException("Invalid map coordinate.");
        }
        private static T Read<T>(string path)
        {
            using (FileStream file = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete))
            {
                if (file.Length > 128 * 1024 * 1024) throw new InvalidDataException("Map export is too large.");
                return (T)new DataContractJsonSerializer(typeof(T)).ReadObject(file);
            }
        }
        internal static MapSnapshot Load(string directory, MapGeometry cached)
        {
            MapLive live = Read<MapLive>(Path.Combine(directory, "live.json"));
            if (live == null || live.schema != 1 || live.generation < 1)
                throw new InvalidDataException("Unsupported map export.");
            if (!live.mesh_ready || live.player == null)
                throw new InvalidDataException("Waiting for gameplay and room geometry...");
            MapGeometry mesh = cached==null?null:cached.BaseGeometry??cached;
            if (mesh == null || mesh.generation != live.generation || mesh.level != live.level)
            {
                mesh = Read<MapGeometry>(Path.Combine(directory, "mesh.json"));
                if (mesh == null || mesh.schema != 1 || mesh.generation != live.generation || mesh.level != live.level)
                    throw new InvalidDataException("Room is changing...");
                if (mesh.vertices == null || mesh.vertices.Length == 0 || mesh.vertices.Length > 131072 ||
                    mesh.triangles == null || mesh.triangles.Length == 0 || mesh.triangles.Length > 262144)
                    throw new InvalidDataException("Invalid room geometry.");
                foreach (float[] point in mesh.vertices) Point(point);
                foreach (MapFace face in mesh.triangles) {
                    if (face == null || face.v == null || face.v.Length != 3)
                        throw new InvalidDataException("Invalid room face.");
                    if (face.normal != null) Point(face.normal);
                    foreach (int index in face.v)
                        if (index < 0 || index >= mesh.vertices.Length) throw new InvalidDataException("Invalid room vertex.");
                }
                live = Read<MapLive>(Path.Combine(directory, "live.json"));
            }
            if (live == null || live.schema != 1 || !live.mesh_ready || live.player == null ||
                live.generation != mesh.generation || live.level != mesh.level)
                throw new InvalidDataException("Room is changing...");
            Point(live.player.position);
            if(live.dialogue!=null)live.dialogue.Validate();
            if(live.player.yaw.HasValue&&(live.player.yaw.Value< -32768||live.player.yaw.Value>32767))throw new InvalidDataException("Invalid player facing.");
            if(live.actors==null)live.actors=new MapActor[0];
            if(live.actors.Length>1024)throw new InvalidDataException("Too many entities.");
            HashSet<uint> actorIds=new HashSet<uint>();
            foreach(MapActor actor in live.actors) {
                if(actor==null || actor.address==0 || !actorIds.Add(actor.address&0x1FFFFFFF) || actor.behavior<0 || actor.behavior>65535 || (actor.name??"").Length>80)
                    throw new InvalidDataException("Invalid map entity.");
                Point(actor.position);
                foreach(float coordinate in actor.position)if(Math.Abs(coordinate)>1000000)throw new InvalidDataException("Entity outside map range.");
                if(actor.name!=null)foreach(char c in actor.name)if(Char.IsControl(c))throw new InvalidDataException("Invalid entity name.");
            }
            if(live.collision!=null)live.collision.Validate(live.actors);
            if (live.exits == null) live.exits = new MapMarker[0];
            if (live.markers == null) live.markers = new MapMarker[0];
            if (live.npcs == null) live.npcs = new MapMarker[0];
            if (live.exits.Length > 1024 || live.markers.Length > 1024 || live.npcs.Length > 1024)
                throw new InvalidDataException("Too many map markers.");
            foreach (MapMarker marker in live.exits) { if (marker == null) throw new InvalidDataException(); Point(marker.position); if(marker.normal!=null)Point(marker.normal); }
            foreach (MapMarker marker in live.markers) {
                if (marker == null) throw new InvalidDataException();
                Point(marker.position);
                if ((marker.label ?? "").Length > 80 || (marker.kind ?? "").Length > 32) throw new InvalidDataException();
            }
            foreach (MapMarker npc in live.npcs) {
                if (npc == null || (npc.kind != "npc" && npc.kind != "tribal"))
                    throw new InvalidDataException("Invalid NPC marker.");
                Point(npc.position);
                if (String.IsNullOrEmpty(npc.label) || npc.label.Length > 80)
                    throw new InvalidDataException("Invalid NPC label.");
            }
            if (live.progression != null) live.progression.Validate();
            return new MapSnapshot { Mesh = NavigationCollision.Merge(mesh,live,cached), Live = live };
        }
    }

    internal sealed class MapCanvas : Control
    {
        private MapSnapshot snapshot;
        internal NavigationRoute Route;
        private MapLayers layers;
        private FloorFollower follower=new FloorFollower();
        private readonly List<HeightPoint> trail=new List<HeightPoint>();
        private float centerX,centerZ,spanX=1,spanZ=1,zoom=1;
        private PointF pan,movement;
        private Point mouse;
        private bool dragging;
        private long lastUpdate=-1;
        internal uint SelectedAddress;
        internal int Mode; // 0 player floor, 1 manual slice, 2 all heights.
        internal float ManualHeight, SliceWidth=64;
        internal bool OtherLevels=true, ShowCollision=true, ShowEntityOrigins=false, RouteBlocked=false;
        internal event Action<uint> EntitySelected;
        internal event Action<uint> ExitActivated;
        internal event Action ZoomChanged;
        internal float ZoomLevel {get{return zoom;}}
        private void ChangedZoom(){Invalidate();if(ZoomChanged!=null)ZoomChanged();}
        private Point mouseStart;
        private void PickEntity(Point point) {
            if(snapshot==null || !ShowCollision)return;
            uint picked=0;float area=Single.MaxValue;
            float low=CenterHeight-SliceWidth/2,high=CenterHeight+SliceWidth/2;
            if(snapshot.Live.collision!=null && snapshot.Live.collision.known)foreach(MapCollisionModel model in snapshot.Live.collision.models) {
                if(NavigationCollision.SameActor(model.address,snapshot.Live.player.address) || (Mode!=2 && !OtherLevels && !model.Overlaps(EntityLow,EntityHigh)))continue;
                RectangleF box=CollisionMapDrawing.Footprint(model,Project);
                float size=Math.Max(1,box.Width*box.Height);box.Inflate(3,3);
                if(box.Contains(point) && size<area){picked=model.address;area=size;}
            }
            if(ShowEntityOrigins && picked==0 && snapshot.Live.actors!=null)foreach(MapActor actor in snapshot.Live.actors) {
                if(Mode!=2 && !OtherLevels && (actor.position[1]<low||actor.position[1]>high))continue;
                PointF p=Project(new HeightPoint(actor.position));
                if(Math.Abs(p.X-point.X)<8 && Math.Abs(p.Y-point.Y)<8){picked=actor.address;break;}
            }
            if(picked!=0 && EntitySelected!=null)EntitySelected(picked);
        }
        internal float CenterHeight {get {return Mode==1?ManualHeight:follower.Height;}}
        internal float EntityLow {get {return Mode==0?CenterHeight+NavigationCollision.FootClearance:CenterHeight-SliceWidth/2;}}
        internal float EntityHigh {get {return Mode==0?CenterHeight+NavigationCollision.BodyHeight:CenterHeight+SliceWidth/2;}}
        internal string LayerStatus {
            get {
                if(layers==null) return "";
                if(layers.Floors.Count==0) return "No upward surfaces found.";
                if(Mode==2) return "All heights: upper surfaces may cover lower floors.";
                return "Y " + (CenterHeight-SliceWidth/2).ToString("0") + " to " + (CenterHeight+SliceWidth/2).ToString("0") +
                    (Mode==0&&!follower.Grounded?" | ground unconfirmed; holding height":"") +
                    " | surface view, not a verified route";
            }
        }
        internal MapCanvas() {DoubleBuffered=true;ResizeRedraw=true;BackColor=Color.FromArgb(16,24,34);Dock=DockStyle.Fill;}
        internal void Fit() {zoom=1;pan=new PointF();ChangedZoom();}
        internal void Zoom(float amount){ZoomAt(amount,new PointF(ClientSize.Width/2f,ClientSize.Height/2f));}
        internal void ZoomAt(float amount,PointF anchor) {
            float next=Math.Max(.25f,Math.Min(16,zoom*amount)),ratio=next/zoom;
            float originX=(Width-30)/2f,originY=(Height-25)/2f;
            pan=new PointF(anchor.X-originX-(anchor.X-originX-pan.X)*ratio,
                           anchor.Y-originY-(anchor.Y-originY-pan.Y)*ratio);
            zoom=next;ChangedZoom();
        }
        internal void CenterOn(float[] position){PointF projected=Project(new HeightPoint(position));pan.X+=ClientSize.Width/2f-projected.X;pan.Y+=ClientSize.Height/2f-projected.Y;Invalidate();}
        internal void UpdateMap(MapSnapshot value) {
            if(value!=null&&(snapshot==null||!Object.ReferenceEquals(snapshot.Mesh,value.Mesh))) {
                float lx=Single.MaxValue,lz=Single.MaxValue,hx=Single.MinValue,hz=Single.MinValue;
                foreach(float[] p in value.Mesh.vertices) {lx=Math.Min(lx,p[0]);hx=Math.Max(hx,p[0]);lz=Math.Min(lz,p[2]);hz=Math.Max(hz,p[2]);}
                centerX=(lx+hx)/2;centerZ=(lz+hz)/2;spanX=Math.Max(1,hx-lx);spanZ=Math.Max(1,hz-lz);
                layers=new MapLayers(value.Mesh);
                bool changedRoom=snapshot==null||snapshot.Live.level!=value.Live.level||snapshot.Live.generation!=value.Live.generation;
                if(changedRoom) {follower=new FloorFollower();trail.Clear();lastUpdate=-1;movement=new PointF();Fit();}
            }
            snapshot=value;
            if(value==null) {layers=null;trail.Clear();Invalidate();return;}
            HeightPoint player=new HeightPoint(value.Live.player.position);
            follower.Update(layers,player);
            if(value.Live.update!=lastUpdate) {
                if(trail.Count>0) {
                    HeightPoint before=trail[trail.Count-1];float dx=player.X-before.X,dz=player.Z-before.Z;
                    float length=(float)Math.Sqrt(dx*dx+dz*dz);
                    if(length>1000) trail.Clear();
                    else if(length>.2f) movement=new PointF(dx/length,dz/length);
                }
                trail.Add(player);if(trail.Count>256) trail.RemoveAt(0);lastUpdate=value.Live.update;
            }
            Invalidate();
        }
        private PointF Project(HeightPoint p) {
            float scale=Math.Min(Math.Max(1,Width-80)/spanX,Math.Max(1,Height-75)/spanZ)*zoom;
            return new PointF((Width-30)/2f+(p.X-centerX)*scale+pan.X,(Height-25)/2f+(p.Z-centerZ)*scale+pan.Y);
        }
        private PointF[] Project(HeightPoint[] vertices) {
            PointF[] result=new PointF[vertices.Length];
            for(int i=0;i<result.Length;i++) result[i]=Project(vertices[i]);
            return result;
        }
        private void Label(Graphics g,string text,PointF p,Color color) {
            using(Brush back=new SolidBrush(Color.FromArgb(220,9,16,24))) {
                SizeF size=g.MeasureString(text,Font);g.FillRectangle(back,p.X,p.Y,size.Width,size.Height);
            }
            using(Brush brush=new SolidBrush(color)) g.DrawString(text,Font,brush,p);
        }
        private void Marker(Graphics g,float[] position,Color color,string text,int shape) {
            HeightPoint p=new HeightPoint(position);
            float low=CenterHeight-SliceWidth/2,high=CenterHeight+SliceWidth/2;
            bool off=Mode!=2&&(p.Y<low||p.Y>high);
            if(off&&!OtherLevels) return;
            if(off) {color=Color.FromArgb(130,color);if(!String.IsNullOrEmpty(text))text+=(p.Y>high?" \u2191":" \u2193")+Math.Abs(p.Y-CenterHeight).ToString("0");}
            PointF at=Project(p);
            using(Brush brush=new SolidBrush(color))
            using(Pen pen=new Pen(off?color:Color.Black,2)) {
                if(shape==3) {
                    if(!off)g.FillEllipse(brush,at.X-3,at.Y-3,6,6);g.DrawEllipse(pen,at.X-3,at.Y-3,6,6);
                } else if(shape==2) {
                    PointF[] diamond={new PointF(at.X,at.Y-6),new PointF(at.X+6,at.Y),new PointF(at.X,at.Y+6),new PointF(at.X-6,at.Y)};
                    if(!off)g.FillPolygon(brush,diamond);g.DrawPolygon(pen,diamond);
                } else if(shape==1) {
                    if(!off)g.FillRectangle(brush,at.X-4,at.Y-4,8,8);g.DrawRectangle(pen,at.X-4,at.Y-4,8,8);
                } else {
                    if(!off)g.FillEllipse(brush,at.X-5,at.Y-5,10,10);g.DrawEllipse(pen,at.X-5,at.Y-5,10,10);
                }
            }
            if(!String.IsNullOrEmpty(text))Label(g,text,new PointF(at.X+8,at.Y-8),color);
        }
        protected override void OnPaint(PaintEventArgs e) {
            base.OnPaint(e);Graphics g=e.Graphics;
            if(snapshot==null||layers==null) {g.DrawString("Waiting for a loaded room...",Font,Brushes.LightGray,20,20);return;}
            g.SmoothingMode=SmoothingMode.AntiAlias;
            float low=CenterHeight-SliceWidth/2,high=CenterHeight+SliceWidth/2;
            g.SmoothingMode=SmoothingMode.None;
            if(Mode!=2&&OtherLevels) {
                using(Brush context=new SolidBrush(Color.FromArgb(65,145,163,176)))
                    foreach(HeightSurface floor in layers.Floors)
                        if(floor.Low<low||floor.High>high) g.FillPolygon(context,Project(floor.Points));
            }
            foreach(HeightPatch patch in layers.Patches) {
                HeightSurface floor=patch.Surface;
                if(Mode!=2&&(floor.High<low||floor.Low>high)) continue;
                HeightPoint[] poly=Mode==2?floor.Points:MapLayers.Clip(floor.Points,low,high);
                if(poly.Length<3) continue;
                using(Brush brush=new SolidBrush(patch.Color)) g.FillPolygon(brush,Project(poly));
            }
            g.SmoothingMode=SmoothingMode.AntiAlias;
            if(ShowCollision)CollisionMapDrawing.Paint(g,snapshot.Live,layers,Mode,EntityLow,EntityHigh,OtherLevels,ShowEntityOrigins,SelectedAddress,Font,Project);
            // Each trail segment retains height; never connect across a hidden floor.
            using(Pen pen=new Pen(Color.FromArgb(170,80,230,250),2)) {
                for(int i=1;i<trail.Count;i++) {
                    HeightPoint a=trail[i-1],b=trail[i];
                    if(Mode!=2) {
                        if((a.Y<low&&b.Y<low)||(a.Y>high&&b.Y>high))continue;
                        if(Math.Abs(b.Y-a.Y)>.0001f) {
                            float ta=(low-a.Y)/(b.Y-a.Y),tb=(high-a.Y)/(b.Y-a.Y);
                            float start=Math.Max(0,Math.Min(ta,tb)),end=Math.Min(1,Math.Max(ta,tb));
                            HeightPoint original=a;a=HeightPoint.Lerp(original,b,start);b=HeightPoint.Lerp(original,b,end);
                        }
                    }
                    g.DrawLine(pen,Project(a),Project(b));
                }
            }
            for(int n=0;snapshot.Live.progression == null && n<snapshot.Live.exits.Length;n++)Marker(g,snapshot.Live.exits[n].position,Color.Gold,"Exit "+(n+1),0);
            HashSet<uint> enriched = new HashSet<uint>();
            if (snapshot.Live.progression != null) foreach (MapInteraction node in snapshot.Live.progression.nodes) {
                enriched.Add(node.address);
                // Alternate progression triggers can occupy the exact same spot.
                // Keep both in the list; let the active exit label remain visible.
                if(node.kind=="exit"&&node.condition_known&&!node.condition_met&&node.address!=SelectedAddress&&
                    Array.Exists(snapshot.Live.progression.nodes,delegate(MapInteraction other) {
                        return other.kind=="exit"&&other.condition_known&&other.condition_met&&
                            Math.Abs(other.position[0]-node.position[0])<1&&Math.Abs(other.position[1]-node.position[1])<1&&
                            Math.Abs(other.position[2]-node.position[2])<1;
                    }))continue;
                if (node.address == SelectedAddress && (Mode == 2 || OtherLevels || (node.position[1] >= low && node.position[1] <= high))) {
                    PointF selectedPoint = Project(new HeightPoint(node.position));
                    using (Pen selectedPen = new Pen(Color.White, 2)) g.DrawEllipse(selectedPen, selectedPoint.X - 10, selectedPoint.Y - 10, 20, 20);
                }
                Color color = node.kind == "npc" ? Color.CornflowerBlue : node.kind == "tribal" ? Color.White :
                    node.kind == "exit" ? Color.Yellow : node.kind == "gate" ? Color.Violet : node.kind == "target" ? Color.OrangeRed :
                    node.kind == "key" ? Color.Plum : node.kind == "weapon" ? Color.Orange : Color.LightGreen;
                if (node.status == "owned" || node.status == "opened" || node.status == "activated" || node.status=="inactive_alternative") color = Color.Gray;
                Marker(g, node.position, color, node.Caption, node.kind == "npc" || node.kind == "tribal" ? 2 : node.kind == "exit" ? 0 : 1);
            }
            foreach (MapMarker marker in snapshot.Live.markers)
                if (!enriched.Contains(marker.address)) Marker(g, marker.position, marker.kind == "token" ? Color.Gold : marker.kind == "opened" ? Color.Gray : marker.kind == "key" ? Color.Plum : marker.kind == "weapon" ? Color.Orange : Color.LightGreen, marker.kind == "token" ? null : marker.label, marker.kind == "token" ? 3 : 1);
            if (snapshot.Live.progression == null) foreach (MapMarker npc in snapshot.Live.npcs)
                if (!enriched.Contains(npc.address)) Marker(g, npc.position, npc.kind == "tribal" ? Color.White : Color.CornflowerBlue, npc.label, 2);
            if (Route != null && Route.Level == snapshot.Live.level && Route.Generation == snapshot.Live.generation) {
                using(Pen routePen = new Pen(RouteBlocked?Color.OrangeRed:Color.Yellow,3)) {
                    routePen.DashStyle = DashStyle.Dash;
                    PointF previous = Project(new HeightPoint(snapshot.Live.player.position));
                    if(Route.PlatformGoal.HasValue) {
                        PointF goal=Project(Route.PlatformGoal.Value);
                        g.DrawEllipse(routePen,goal.X-7,goal.Y-7,14,14);
                        g.DrawString("Planning supported jumps",Font,Brushes.Yellow,goal.X+10,goal.Y);
                    }else foreach(HeightPoint p in Route.Points) { PointF next=Project(p); g.DrawLine(routePen,previous,next);previous=next; }
                }
            }
            HeightPoint player=new HeightPoint(snapshot.Live.player.position);PointF atPlayer=Project(player);
            using(Brush brush=new SolidBrush(snapshot.IsLive?Color.Cyan:Color.Gray)) {
                g.FillEllipse(Brushes.Black,atPlayer.X-8,atPlayer.Y-8,16,16);g.FillEllipse(brush,atPlayer.X-6,atPlayer.Y-6,12,12);
                if(movement.X!=0||movement.Y!=0)g.FillPolygon(brush,new PointF[]{
                    new PointF(atPlayer.X+movement.X*17,atPlayer.Y+movement.Y*17),
                    new PointF(atPlayer.X-movement.Y*7,atPlayer.Y+movement.X*7),
                    new PointF(atPlayer.X+movement.Y*7,atPlayer.Y-movement.X*7)});
            }
            if(Mode!=2&&(player.Y<low||player.Y>high))Label(g,player.Y>high?"Player above slice":"Player below slice",new PointF(atPlayer.X+10,atPlayer.Y+10),Color.Cyan);
            // Fixed room-wide scale; moving the slice never changes color meanings.
            int x=Width-66,y=42;
            for(int i=0;i<MapLayers.Bands;i++)
                using(Brush brush=new SolidBrush(MapLayers.HeightColor((i+.5f)/MapLayers.Bands)))g.FillRectangle(brush,x,y+(MapLayers.Bands-1-i)*7,16,7);
            Label(g,layers.High.ToString("0"),new PointF(x-12,y-20),Color.White);
            Label(g,layers.Low.ToString("0"),new PointF(x-12,y+114),Color.White);
            Label(g,"Height Y",new PointF(x-14,y+134),Color.White);
            Label(g,LayerStatus,new PointF(8,Height-24),Color.LightGray);
        }
        protected override void OnMouseDoubleClick(MouseEventArgs e) {
            base.OnMouseDoubleClick(e);
            if(e.Button!=MouseButtons.Left||snapshot==null||ExitActivated==null)return;
            uint chosen=0;float best=14;
            foreach(var exit in snapshot.Live.exits) {
                if(Mode!=2&&!OtherLevels&&Math.Abs(exit.position[1]-CenterHeight)>SliceWidth/2)continue;
                PointF p=Project(new HeightPoint(exit.position));float dx=p.X-e.X,dy=p.Y-e.Y;
                float distance=(float)Math.Sqrt(dx*dx+dy*dy);if(distance<best){best=distance;chosen=exit.address;}
            }
            if(chosen!=0)ExitActivated(chosen);
        }
        protected override void OnMouseWheel(MouseEventArgs e){base.OnMouseWheel(e);if(e.Delta!=0)ZoomAt((float)Math.Pow(1.2,e.Delta/120.0),e.Location);}
        protected override void OnMouseDown(MouseEventArgs e){base.OnMouseDown(e);Focus();dragging=e.Button==MouseButtons.Left;mouse=e.Location;mouseStart=e.Location;Capture=dragging;}
        protected override void OnMouseMove(MouseEventArgs e){base.OnMouseMove(e);if(dragging){pan.X+=e.X-mouse.X;pan.Y+=e.Y-mouse.Y;mouse=e.Location;Invalidate();}}
        protected override void OnMouseUp(MouseEventArgs e){base.OnMouseUp(e);dragging=false;Capture=false;if(e.Button==MouseButtons.Left && Math.Abs(e.X-mouseStart.X)+Math.Abs(e.Y-mouseStart.Y)<5)PickEntity(e.Location);}
    }


    internal sealed class MapMenuRenderer:ToolStripProfessionalRenderer {
        // Match the main frontend's 13dp Barlow menu type and flat colors.
        private static readonly System.Drawing.Text.PrivateFontCollection fonts=new System.Drawing.Text.PrivateFontCollection();
        private static IntPtr fontBytes;
        [System.Runtime.InteropServices.DllImport("gdi32.dll")] private static extern IntPtr AddFontMemResourceEx(IntPtr data,uint size,IntPtr reserved,ref uint count);
        internal static readonly Font MenuFont=LoadFont();
        private static Font LoadFont() {
            using(var stream=System.Reflection.Assembly.GetExecutingAssembly().GetManifestResourceStream("JfgLauncher.Barlow.Regular")) {
                if(stream==null)return new Font("Segoe UI",9.75f);
                byte[] bytes=new byte[stream.Length];int read=0,n;
                while(read<bytes.Length&&(n=stream.Read(bytes,read,bytes.Length-read))>0)read+=n;
                if(read!=bytes.Length)throw new InvalidDataException("Incomplete menu font.");
                fontBytes=System.Runtime.InteropServices.Marshal.AllocHGlobal(bytes.Length);
                System.Runtime.InteropServices.Marshal.Copy(bytes,0,fontBytes,bytes.Length);
                uint count=0;AddFontMemResourceEx(fontBytes,(uint)bytes.Length,IntPtr.Zero,ref count);
                fonts.AddMemoryFont(fontBytes,bytes.Length);
                return new Font(fonts.Families[0],9.75f,FontStyle.Regular,GraphicsUnit.Point);
            }
        }
        internal MapMenuRenderer():base(new MapMenuColors()){RoundedEdges=false;}
        protected override void OnRenderToolStripBackground(ToolStripRenderEventArgs e){e.Graphics.Clear(ToolColors.Panel);}
        protected override void OnRenderMenuItemBackground(ToolStripItemRenderEventArgs e){
            using(var brush=new SolidBrush(e.Item.Selected||e.Item.Pressed?Color.FromArgb(38,49,73):ToolColors.Panel))
                e.Graphics.FillRectangle(brush,new Rectangle(Point.Empty,e.Item.Size));
        }
        protected override void OnRenderToolStripBorder(ToolStripRenderEventArgs e){
            using(var pen=new Pen(e.ToolStrip is MenuStrip?Color.FromArgb(34,43,64):ToolColors.Border)) {
                if(e.ToolStrip is MenuStrip)e.Graphics.DrawLine(pen,0,e.ToolStrip.Height-1,e.ToolStrip.Width,e.ToolStrip.Height-1);
                else e.Graphics.DrawRectangle(pen,0,0,e.ToolStrip.Width-1,e.ToolStrip.Height-1);
            }
        }
        protected override void OnRenderImageMargin(ToolStripRenderEventArgs e){}
        protected override void OnRenderArrow(ToolStripArrowRenderEventArgs e){e.ArrowColor=e.Item.Enabled?ToolColors.Text:ToolColors.Muted;base.OnRenderArrow(e);}
        protected override void OnRenderItemText(ToolStripItemTextRenderEventArgs e){e.TextColor=!e.Item.Enabled?ToolColors.Muted:e.Item.Selected||e.Item.Pressed?Color.FromArgb(242,244,248):ToolColors.Text;base.OnRenderItemText(e);}
        internal static void Style(MenuStrip menu) {
            menu.Font=MenuFont;
            foreach(ToolStripMenuItem top in menu.Items) {
                top.Padding=new Padding(10,0,10,0);top.Margin=Padding.Empty;
                top.DropDown.Font=MenuFont;top.DropDown.Padding=new Padding(3);
                top.DropDown.BackColor=ToolColors.Panel;top.DropDown.ForeColor=ToolColors.Text;
                top.DropDown.Renderer=menu.Renderer;
                foreach(ToolStripItem item in top.DropDownItems) {
                    var command=item as ToolStripMenuItem;
                    if(command!=null){command.AutoSize=false;command.Size=new Size(252,26);command.Padding=new Padding(12,4,12,4);command.Margin=Padding.Empty;}
                }
            }
        }
    }
    internal sealed class MapMenuColors:ProfessionalColorTable {
        public override Color ToolStripDropDownBackground{get{return ToolColors.Panel;}}
        public override Color ImageMarginGradientBegin{get{return ToolColors.Panel;}}
        public override Color ImageMarginGradientMiddle{get{return ToolColors.Panel;}}
        public override Color ImageMarginGradientEnd{get{return ToolColors.Panel;}}
        public override Color MenuItemSelected{get{return Color.FromArgb(38,49,73);}}
        public override Color MenuItemSelectedGradientBegin{get{return MenuItemSelected;}}
        public override Color MenuItemSelectedGradientEnd{get{return MenuItemSelected;}}
        public override Color MenuItemPressedGradientBegin{get{return MenuItemSelected;}}
        public override Color MenuItemPressedGradientEnd{get{return MenuItemSelected;}}
        public override Color MenuBorder{get{return ToolColors.Border;}}
    }
    internal sealed class MapSettingsDialog: ApplicationWindow {
        internal MapSettingsDialog(params ToolStripMenuItem[] sections) {
            Text="Live map settings";ClientSize=new Size(710,540);MinimumSize=new Size(640,480);
            StartPosition=FormStartPosition.CenterParent;ShowInTaskbar=false;MinimizeBox=false;MaximizeBox=false;
            AutoScaleMode=AutoScaleMode.Dpi;Font=new Font(MapMenuRenderer.MenuFont.FontFamily,10.5f);
            BackColor=ToolColors.Background;ForeColor=ToolColors.Text;
            var content=new Panel{Dock=DockStyle.Fill,Padding=new Padding(24,18,24,18)};
            var sidebar=new FlowLayoutPanel{Dock=DockStyle.Left,Width=160,FlowDirection=FlowDirection.TopDown,WrapContents=false,Padding=new Padding(12,18,12,12),BackColor=ToolColors.Panel};
            var footer=new Panel{Dock=DockStyle.Bottom,Height=52,Padding=new Padding(10),BackColor=ToolColors.Panel};
            var done=ToolColors.Button("Done");done.Width=90;done.Dock=DockStyle.Right;done.DialogResult=DialogResult.OK;footer.Controls.Add(done);AcceptButton=done;CancelButton=done;
            Controls.Add(content);Controls.Add(sidebar);Controls.Add(footer);
            var pages=new List<Control>();var buttons=new List<Button>();string[] titles={"Display","AI","Mods","Tools"};
            for(int index=0;index<sections.Length;index++) {
                var page=new FlowLayoutPanel{Dock=DockStyle.Fill,FlowDirection=FlowDirection.TopDown,WrapContents=false,AutoScroll=true,Visible=false};pages.Add(page);content.Controls.Add(page);
                page.Controls.Add(new Label{Text=titles[index].ToUpperInvariant(),AutoSize=false,Size=new Size(430,34),ForeColor=ToolColors.Muted});
                foreach(ToolStripItem item in sections[index].DropDownItems) AddSetting(page,item);
                int selected=index;var tab=ToolColors.Button(titles[index]);tab.Size=new Size(136,40);tab.TextAlign=ContentAlignment.MiddleLeft;tab.Padding=new Padding(12,0,0,0);tab.Margin=new Padding(0,0,0,6);buttons.Add(tab);sidebar.Controls.Add(tab);
                tab.Click+=delegate{for(int i=0;i<pages.Count;i++){pages[i].Visible=i==selected;buttons[i].BackColor=i==selected?Color.FromArgb(38,49,73):ToolColors.Panel;buttons[i].ForeColor=i==selected?ToolColors.Amber:ToolColors.Text;}};
            }
            pages[0].Visible=true;buttons[0].BackColor=Color.FromArgb(38,49,73);buttons[0].ForeColor=ToolColors.Amber;
        }
        private void AddSetting(FlowLayoutPanel page,ToolStripItem item) {
            if(item is ToolStripSeparator){page.Controls.Add(new Panel{Size=new Size(420,1),BackColor=ToolColors.Border,Margin=new Padding(0,10,0,10)});return;}
            var command=item as ToolStripMenuItem;
            if(command!=null) {
                Control control;
                if(command.CheckOnClick) {
                    var toggle=new CheckBox{Text=command.Text,Checked=command.Checked,Size=new Size(420,32),ForeColor=ToolColors.Text};
                    toggle.CheckedChanged+=delegate{if(toggle.Checked!=command.Checked)command.PerformClick();};
                    EventHandler changed=delegate{toggle.Checked=command.Checked;};command.CheckedChanged+=changed;Disposed+=delegate{command.CheckedChanged-=changed;};control=toggle;
                }else {var button=ToolColors.Button(command.Text);button.Size=new Size(420,34);button.TextAlign=ContentAlignment.MiddleLeft;button.Padding=new Padding(10,0,0,0);button.Click+=delegate{command.PerformClick();};control=button;}
                control.Margin=new Padding(0,0,0,8);control.Visible=command.Available;control.Enabled=command.Enabled;
                EventHandler available=delegate{control.Visible=command.Available;control.Enabled=command.Enabled;};command.AvailableChanged+=available;command.EnabledChanged+=available;
                Disposed+=delegate{command.AvailableChanged-=available;command.EnabledChanged-=available;};page.Controls.Add(control);
                if(!String.IsNullOrEmpty(command.ToolTipText))page.Controls.Add(new Label{Text=command.ToolTipText,AutoSize=true,MaximumSize=new Size(420,0),ForeColor=ToolColors.Muted,Margin=new Padding(0,0,0,18)});
                return;
            }
            var host=item as ToolStripControlHost;
            if(host!=null) {
                var combo=host.Control as ComboBox;
                if(combo!=null) {
                    var next=new ComboBox{DropDownStyle=ComboBoxStyle.DropDownList,Width=420,BackColor=ToolColors.Panel,ForeColor=ToolColors.Text,FlatStyle=FlatStyle.Flat,Margin=new Padding(0,0,0,12)};
                    foreach(var choice in combo.Items)next.Items.Add(choice);next.SelectedIndex=combo.SelectedIndex;
                    next.SelectedIndexChanged+=delegate{combo.SelectedIndex=next.SelectedIndex;};page.Controls.Add(next);return;
                }
                var numeric=host.Control as NumericUpDown;
                if(numeric!=null) {
                    var next=new NumericUpDown{Minimum=numeric.Minimum,Maximum=numeric.Maximum,Increment=numeric.Increment,Value=numeric.Value,Enabled=numeric.Enabled,Width=420,BackColor=ToolColors.Panel,ForeColor=ToolColors.Text,Margin=new Padding(0,0,0,12)};
                    next.ValueChanged+=delegate{numeric.Value=next.Value;};EventHandler sync=delegate{next.Enabled=numeric.Enabled;next.Value=numeric.Value;};numeric.EnabledChanged+=sync;numeric.ValueChanged+=sync;
                    Disposed+=delegate{numeric.EnabledChanged-=sync;numeric.ValueChanged-=sync;};page.Controls.Add(next);return;
                }
                var label=host.Control as Label;
                if(label!=null) {
                    var next=new Label{Text=label.Text,Size=new Size(420,80),ForeColor=ToolColors.Blue};EventHandler sync=delegate{next.Text=label.Text;};label.TextChanged+=sync;Disposed+=delegate{label.TextChanged-=sync;};page.Controls.Add(next);return;
                }
            }
            page.Controls.Add(new Label{Text=item.Text,Size=new Size(420,24),ForeColor=ToolColors.Muted});
        }
    }
    internal sealed class NavigationMapWindow : ApplicationWindow
    {
        internal ToolStripMenuItem[] SettingsSections;
        private ToolStripMenuItem warpMod, healthMod, killMod;
        private bool syncingMods;
        private readonly MapModPreferences modPreferences=new MapModPreferences(LocalSetup.ProfileRoot);
        private void SyncMods(MapSnapshot value) {
            syncingMods=true;
            try {
                modPreferences.Sync(directory,value==null?null:value.Live,value!=null&&value.IsLive);
                var mods=modPreferences.Values;
                warpMod.Checked=mods.Warp;healthMod.Checked=mods.Health;killMod.Checked=mods.Kill;
                warpMod.Enabled=healthMod.Enabled=killMod.Enabled=!modPreferences.Failed;
                if(modPreferences.Failed)status.Text="Error loading mods";
            } finally {syncingMods=false;}
        }
        private void SaveMods() {
            if(syncingMods)return;
            modPreferences.Choose(new MapMods{Warp=warpMod.Checked,Health=healthMod.Checked,Kill=killMod.Checked});
            SyncMods(aiSnapshot);
        }
        private void WarpToExit(uint address) {
            if(!warpMod.Checked)return;
            try {StopAi();MapMods.Load(directory,aiSnapshot==null?null:aiSnapshot.Live).WarpTo(directory,aiSnapshot,address,++aiNonce,false);status.Text="Exit warp requested.";}
            catch(InvalidDataException error){status.Text=error.Message;}
            catch(IOException error){status.Text=error.Message;}
            catch(UnauthorizedAccessException error){status.Text=error.Message;}
        }
        private readonly Label inventoryStatus = new Label { Dock = DockStyle.Top, Height = 68, Padding = new Padding(8) };
        private readonly ListBox interactionList = new ListBox { Dock = DockStyle.Top, Height = 230, HorizontalScrollbar = true };
        private readonly TextBox interactionDetails = new TextBox { Dock = DockStyle.Fill, Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, BorderStyle = BorderStyle.None, BackColor = SystemColors.Control };
        private readonly ComboBox interactionFilter=new ComboBox{FlatStyle=FlatStyle.Flat,Dock=DockStyle.Top,DropDownStyle=ComboBoxStyle.DropDownList};
        private static bool InGroup(MapInteraction node,int group){return group==0||group==1&&node.action=="open_chest"||group==2&&(node.kind=="key"||node.kind=="weapon"||node.kind=="item"||node.kind=="pickup"||node.kind=="health"||node.kind=="ammo"||node.kind=="token")||group==3&&node.kind=="exit"||group==4&&(node.kind=="npc"||node.kind=="tribal");}
        private AutonomousExplorer explorer = new AutonomousExplorer(null,new NavigationExplorer());
        private string explorerError;
        private uint selectedEntity;
        private void EntityDetails(MapSnapshot value) {
            if(value==null || selectedEntity==0)return;
            MapActor actor=NavigationCollision.Actor(value.Live,selectedEntity);
            if(actor==null){selectedEntity=0;return;}
            canvas.SelectedAddress=selectedEntity;interactionDetails.Text=NavigationCollision.Details(value.Live,actor);
        }
        private MapSnapshot aiSnapshot;
        private NavigationRoute aiRoute;
        private long aiNonce = DateTime.UtcNow.Ticks;
        private readonly Label aiStatus = new Label { Text = "AI off - select an exit and plan a candidate route", AutoSize = true };
        private void StopAi() {
            explorer.Stop("Explorer stopped");StopPilot();
            if(directory!=null)try {File.Delete(Path.Combine(directory,"ai-confirm.txt"));File.Delete(Path.Combine(directory,"ai-dialogue.txt"));}catch(IOException){}catch(UnauthorizedAccessException){}
            aiStatus.Text="AI stopped";
        }
        private void StopPilot() {
            // The runner checks cancellation and writes its final stop itself.
            // Do not race its atomic command file with a second UI writer.
            if(explorer!=null&&explorer.Busy)return;
            if(aiRoute!=null)try { aiRoute.Send(directory,++aiNonce,false,true); } catch(IOException) {} catch(UnauthorizedAccessException) {}
        }
        private bool updatingInteractions;
        private long interactionGeneration = -1;
        private uint interactionLevel;
        internal void UpdateInteractions(MapSnapshot value)
        {
            MapProgression progress = value == null ? null : value.Live.progression;
            MapInteraction selected = interactionList.SelectedItem as MapInteraction;
            bool sameRoom = value != null && value.Live.generation == interactionGeneration && value.Live.level == interactionLevel;
            uint address = sameRoom && selected != null ? selected.address : 0;
            int top = sameRoom ? interactionList.TopIndex : 0;
            MapInteraction topNode = top >= 0 && top < interactionList.Items.Count ? interactionList.Items[top] as MapInteraction : null;
            interactionGeneration = value == null ? -1 : value.Live.generation;
            interactionLevel = value == null ? 0 : value.Live.level;
            MapInteraction[] nodes = progress == null ? new MapInteraction[0] : Array.FindAll(progress.nodes,delegate(MapInteraction node){return InGroup(node,interactionFilter.SelectedIndex);});
            updatingInteractions = true;
            interactionList.BeginUpdate();
            try {
                bool sameOrder = sameRoom && nodes.Length == interactionList.Items.Count;
                for (int i = 0; sameOrder && i < nodes.Length; ++i)
                    sameOrder = ((MapInteraction)interactionList.Items[i]).address == nodes[i].address;
                if (!sameOrder) interactionList.Items.Clear();
                int selection = -1, anchor = -1;
                for (int i = 0; i < nodes.Length; ++i) {
                    MapInteraction node = nodes[i];
                    node.ExplorationStatus = "";
                    if (node.kind == "exit") foreach (MapMarker marker in value.Live.exits)
                        if (marker.address == node.address) { node.ExplorationStatus = explorer.Describe(value.Live.level, marker); break; }
                    if (sameOrder) interactionList.Items[i] = node;
                    else interactionList.Items.Add(node);
                    if (node.address == address) selection = i;
                    if (sameRoom && topNode != null && node.address == topNode.address) anchor = i;
                }
                interactionList.SelectedIndex = selection >= 0 ? selection : nodes.Length > 0 ? 0 : -1;
                // Keep the viewport even when the selected row is off-screen.
                if (nodes.Length > 0) interactionList.TopIndex = sameRoom ? Math.Min(nodes.Length - 1, anchor >= 0 ? anchor : top) : 0;
            } finally { interactionList.EndUpdate(); updatingInteractions = false; }
            inventoryStatus.Text = progress == null ? "Progression data unavailable" :
                (value.IsLive ? "LIVE  " : "SAVED SNAPSHOT  ") + progress.inventory.Summary;
            selected = interactionList.SelectedItem as MapInteraction;
            string details = selected == null ? "Select a loaded interaction to inspect its reward, requirements and coordinates." : selected.Details;
            if (interactionDetails.Text != details) interactionDetails.Text = details;
            canvas.SelectedAddress = selectedEntity != 0 ? selectedEntity : selected == null ? 0 : selected.address;
        }

        private readonly MapCanvas canvas = new MapCanvas();
        private readonly Label status = new Label();
        private NumericUpDown layerHeight;
        private readonly Timer timer = new Timer();
        private MapGeometry cached;
        private string directory;
        internal NavigationMapWindow(string path):this(path,LocalSetup.LoadSettings(LocalSetup.ProfileRoot).RomPath){}
        internal NavigationMapWindow(string path,string romPath)
        {
            Text = "JFG Live Map";
            ClientSize = new Size(1240, 760);
            MinimumSize = new Size(1040, 600);
            Font = new Font("Segoe UI", 9);AutoScaleMode=AutoScaleMode.Dpi;
            BackColor=ToolColors.Background;ForeColor=ToolColors.Text;
            interactionDetails.BackColor=ToolColors.Panel;interactionDetails.ForeColor=ToolColors.Text;
            interactionList.BackColor=ToolColors.Background;interactionList.ForeColor=ToolColors.Text;interactionList.BorderStyle=BorderStyle.None;
            interactionList.DrawMode=DrawMode.OwnerDrawFixed;interactionList.ItemHeight=34;
            interactionList.DrawItem+=delegate(object sender,DrawItemEventArgs e){
                if(e.Index<0)return;bool selected=(e.State&DrawItemState.Selected)!=0;
                using(var background=new SolidBrush(selected?Color.FromArgb(48,39,26):ToolColors.Background))e.Graphics.FillRectangle(background,e.Bounds);
                var node=interactionList.Items[e.Index] as MapInteraction;
                TextRenderer.DrawText(e.Graphics,node==null?"":node.ToString(),Font,new Rectangle(e.Bounds.X+8,e.Bounds.Y+7,e.Bounds.Width-16,e.Bounds.Height-7),selected?ToolColors.Amber:ToolColors.Text,TextFormatFlags.EndEllipsis|TextFormatFlags.SingleLine);
                e.DrawFocusRectangle();
            };
            Controls.Add(canvas);
            Panel progressionPanel = new Panel { Dock = DockStyle.Right, Width = 300, Padding = new Padding(12), BackColor=ToolColors.Panel };
            progressionPanel.Controls.Add(interactionDetails); progressionPanel.Controls.Add(interactionList); progressionPanel.Controls.Add(inventoryStatus);
            interactionFilter.Items.AddRange(new object[]{"All interactions","Chests","Pickups","Exits","Characters"});interactionFilter.SelectedIndex=0;
            interactionFilter.BackColor=ToolColors.Panel;interactionFilter.ForeColor=ToolColors.Text;
            progressionPanel.Controls.Add(interactionFilter);
            progressionPanel.Controls.Add(new Label{Text="INSPECTOR",Dock=DockStyle.Top,Height=32,ForeColor=ToolColors.Muted,Font=new Font(Font,FontStyle.Bold),Padding=new Padding(0,6,0,0)});
            interactionFilter.SelectedIndexChanged+=delegate{selectedEntity=0;UpdateInteractions(aiSnapshot);canvas.Invalidate();};
            Controls.Add(progressionPanel);
            canvas.EntitySelected+=delegate(uint address) {selectedEntity=address;EntityDetails(aiSnapshot);canvas.Invalidate();};
            interactionList.MouseDown+=delegate {selectedEntity=0;};
            interactionList.SelectedIndexChanged += delegate {
                if(updatingInteractions)return;
                MapInteraction node = interactionList.SelectedItem as MapInteraction;
                interactionDetails.Text = node == null ? "" : node.Details;
                canvas.SelectedAddress = node == null ? 0 : node.address; canvas.Invalidate();
            };
            var menus=new MenuStrip{Dock=DockStyle.Top,Height=24,AutoSize=false,BackColor=ToolColors.Panel,ForeColor=ToolColors.Text,Padding=new Padding(4,0,4,0),Renderer=new MapMenuRenderer()};
            var view=new ToolStripMenuItem("View");var layersMenu=new ToolStripMenuItem("Layers");var aiMenu=new ToolStripMenuItem("AI");var toolsMenu=new ToolStripMenuItem("Tools");
            menus.Items.AddRange(new ToolStripItem[]{view,layersMenu,aiMenu,toolsMenu});
            var fit=new ToolStripMenuItem("Fit room");fit.ShortcutKeys=Keys.Control|Keys.D0;fit.Click+=delegate{canvas.Fit();};view.DropDownItems.Add(fit);
            var inspector=new ToolStripMenuItem("Inspector"){CheckOnClick=true,Checked=true};inspector.CheckedChanged+=delegate{progressionPanel.Visible=inspector.Checked;};view.DropDownItems.Add(inspector);
            var legend=new ToolStripMenuItem("Legend"){CheckOnClick=true,Checked=true};view.DropDownItems.Add(legend);
            var files=new ToolStripMenuItem("Open exports");files.Click+=delegate{OpenExports();};toolsMenu.DropDownItems.Add(files);
            var mode=new ComboBox{DropDownStyle=ComboBoxStyle.DropDownList,Width=210};mode.Items.AddRange(new object[]{"Player floor","Height slice","All heights"});mode.SelectedIndex=0;
            var height=new NumericUpDown{Minimum=-1000000,Maximum=1000000,Increment=16,Width=210,Enabled=false};layerHeight=height;
            var thickness=new NumericUpDown{Minimum=4,Maximum=4096,Value=64,Increment=16,Width=210};
            var context=new ToolStripMenuItem("Show other heights"){CheckOnClick=true,Checked=true};
            layersMenu.DropDownItems.Add(new ToolStripControlHost(mode));layersMenu.DropDownItems.Add(new ToolStripLabel("Height Y"));layersMenu.DropDownItems.Add(new ToolStripControlHost(height));
            layersMenu.DropDownItems.Add(new ToolStripLabel("Slice width"));layersMenu.DropDownItems.Add(new ToolStripControlHost(thickness));layersMenu.DropDownItems.Add(context);
            mode.SelectedIndexChanged+=delegate{
                if(mode.SelectedIndex==1)height.Value=Math.Max(height.Minimum,Math.Min(height.Maximum,(decimal)canvas.CenterHeight));
                canvas.Mode=mode.SelectedIndex;canvas.ManualHeight=(float)height.Value;height.Enabled=canvas.Mode==1;thickness.Enabled=context.Enabled=canvas.Mode!=2;canvas.Invalidate();
            };
            height.ValueChanged+=delegate{canvas.ManualHeight=(float)height.Value;canvas.Invalidate();};thickness.ValueChanged+=delegate{canvas.SliceWidth=(float)thickness.Value;canvas.Invalidate();};
            context.CheckedChanged+=delegate{canvas.OtherLevels=context.Checked;canvas.Invalidate();};
            var collisionToggle=new ToolStripMenuItem("Entity collision boxes"){CheckOnClick=true,Checked=true};
            var originsToggle=new ToolStripMenuItem("Unknown entity origins"){CheckOnClick=true};
            collisionToggle.CheckedChanged+=delegate{canvas.ShowCollision=collisionToggle.Checked;canvas.Invalidate();};originsToggle.CheckedChanged+=delegate{canvas.ShowEntityOrigins=originsToggle.Checked;canvas.Invalidate();};
            layersMenu.DropDownItems.Add(new ToolStripSeparator());layersMenu.DropDownItems.Add(collisionToggle);layersMenu.DropDownItems.Add(originsToggle);
            var planAi=new ToolStripMenuItem("Plan exit route");var startAi=new ToolStripMenuItem("Start planned route");var stopAi=new ToolStripMenuItem("Stop AI"){ShortcutKeyDisplayString="Esc"};
            var explore=new ToolStripMenuItem("Explore automatically");var retry=new ToolStripMenuItem("Retry room exits");
            var aiControls=new ComboBox{DropDownStyle=ComboBoxStyle.DropDownList,Width=210};aiControls.Items.AddRange(new object[]{"Normal (C-Up jump)","Expert (A jump)"});aiControls.SelectedIndex=0;
            aiMenu.DropDownItems.AddRange(new ToolStripItem[]{explore,stopAi,new ToolStripSeparator(),new ToolStripControlHost(aiControls)});
            var advanced=new ToolStripMenuItem("Advanced individual tests"){CheckOnClick=true};aiMenu.DropDownItems.Add(advanced);
            aiMenu.DropDownItems.AddRange(new ToolStripItem[]{planAi,startAi,retry});planAi.Visible=startAi.Visible=retry.Visible=false;
            advanced.CheckedChanged+=delegate{planAi.Visible=startAi.Visible=retry.Visible=advanced.Checked;};
            var inventory=new ToolStripMenuItem("Live inventory");toolsMenu.DropDownItems.Add(inventory);
            inventory.Click+=delegate{try{using(var tool=NativeLiveTools.OpenWindow("inventory",LocalSetup.ProfileRoot,IntPtr.Zero)) {}}catch(IOException error){status.Text=error.Message;}};
            // AI feedback belongs with its commands, not in a permanent footer.
            aiStatus.AutoSize=false;aiStatus.Size=new Size(244,64);aiStatus.Padding=new Padding(8);
            aiStatus.BackColor=ToolColors.Panel;aiStatus.ForeColor=ToolColors.Blue;aiStatus.Font=MapMenuRenderer.MenuFont;
            aiMenu.DropDownItems.Add(new ToolStripSeparator());
            aiMenu.DropDownItems.Add(new ToolStripControlHost(aiStatus){AutoSize=false,Size=aiStatus.Size,Margin=Padding.Empty,Padding=Padding.Empty});
            var settingsMenu=new ToolStripMenuItem("Settings");
            var settingsItem=new ToolStripMenuItem("Map settings...");settingsMenu.DropDownItems.Add(settingsItem);
            menus.Items.Remove(layersMenu);menus.Items.Remove(aiMenu);menus.Items.Remove(toolsMenu);menus.Items.Add(settingsMenu);
            var modsMenu=new ToolStripMenuItem("Mods");
            warpMod=new ToolStripMenuItem("Warp to exits"){CheckOnClick=true,ToolTipText=MapMods.WarpHelp};
            healthMod=new ToolStripMenuItem("Infinite health"){CheckOnClick=true,ToolTipText=MapMods.HealthHelp};
            killMod=new ToolStripMenuItem("Instant kill enemies"){CheckOnClick=true,ToolTipText=MapMods.KillHelp};
            modsMenu.DropDownItems.AddRange(new ToolStripItem[]{warpMod,healthMod,killMod});
            foreach(var mod in new[]{warpMod,healthMod,killMod})mod.CheckedChanged+=delegate{SaveMods();};
            canvas.ExitActivated+=WarpToExit;
            interactionList.MouseDoubleClick+=delegate(object sender,MouseEventArgs e){
                int index=interactionList.IndexFromPoint(e.Location);
                var node=index<0?null:interactionList.Items[index] as MapInteraction;
                if(e.Button==MouseButtons.Left&&node!=null&&node.kind=="exit")WarpToExit(node.address);
            };
            SettingsSections=new[]{layersMenu,aiMenu,modsMenu,toolsMenu};
            settingsItem.Click+=delegate{using(var settings=new MapSettingsDialog(SettingsSections))settings.ShowDialog(this);};
            FormClosed+=delegate{layersMenu.Dispose();aiMenu.Dispose();modsMenu.Dispose();toolsMenu.Dispose();};
            MapMenuRenderer.Style(menus);MainMenuStrip=menus;Controls.Add(menus);
            status.Dock=DockStyle.Bottom;status.Height=30;status.Padding=new Padding(12,6,0,0);status.BackColor=ToolColors.Panel;status.ForeColor=ToolColors.Muted;Controls.Add(status);
            var legendLabel=new Label{AutoSize=false,Size=new Size(355,68),BackColor=ToolColors.Panel,ForeColor=ToolColors.Muted,Padding=new Padding(10),Text="Cyan  Player     Yellow  Exits     Squares  Items\nBlue  NPCs     White  Tribals     Violet  Doors\nWheel to zoom  /  Drag to pan"};canvas.Controls.Add(legendLabel);
            Action placeLegend=delegate{legendLabel.Location=new Point(14,Math.Max(5,canvas.Height-legendLabel.Height-38));};canvas.Resize+=delegate{placeLegend();};placeLegend();
            legend.CheckedChanged+=delegate{legendLabel.Visible=legend.Checked;};
            var zoomBar=new FlowLayoutPanel{Name="map-zoom-controls",Size=new Size(176,32),WrapContents=false,Padding=new Padding(2),BackColor=ToolColors.Panel};
            var less=ToolColors.Button("\u2212");less.Name="zoom-out";less.AccessibleName="Zoom out";less.Size=new Size(30,28);less.Margin=Padding.Empty;
            var percent=new Label{Name="zoom-level",Text="100%",Size=new Size(62,28),TextAlign=ContentAlignment.MiddleCenter,ForeColor=ToolColors.Text,Margin=Padding.Empty};
            var more=ToolColors.Button("+");more.Name="zoom-in";more.AccessibleName="Zoom in";more.Size=new Size(30,28);more.Margin=Padding.Empty;
            var fitCorner=ToolColors.Button("Fit");fitCorner.Name="zoom-fit";fitCorner.AccessibleName="Fit room";fitCorner.Size=new Size(48,28);fitCorner.Margin=Padding.Empty;
            zoomBar.Controls.AddRange(new Control[]{less,percent,more,fitCorner});canvas.Controls.Add(zoomBar);
            Action placeZoom=delegate{zoomBar.Location=new Point(Math.Max(4,canvas.Width-zoomBar.Width-14),Math.Max(4,canvas.Height-zoomBar.Height-14));};canvas.Resize+=delegate{placeZoom();};placeZoom();
            less.Click+=delegate{canvas.Zoom(1/1.2f);};more.Click+=delegate{canvas.Zoom(1.2f);};fitCorner.Click+=delegate{canvas.Fit();};
            canvas.ZoomChanged+=delegate{percent.Text=(canvas.ZoomLevel*100).ToString("0")+"%";less.Enabled=canvas.ZoomLevel>.25f;more.Enabled=canvas.ZoomLevel<16;};
            var center=ToolColors.Button("Center selected");center.Dock=DockStyle.Bottom;center.Height=34;center.Click+=delegate{
                var actor=aiSnapshot==null||selectedEntity==0?null:NavigationCollision.Actor(aiSnapshot.Live,selectedEntity);
                if(actor!=null)canvas.CenterOn(actor.position);
                else {var selected=interactionList.SelectedItem as MapInteraction;if(selected!=null)canvas.CenterOn(selected.position);}
            };progressionPanel.Controls.Add(center);center.SendToBack();
            foreach(ToolStripMenuItem menu in menus.Items){menu.DropDown.BackColor=ToolColors.Panel;menu.DropDown.ForeColor=ToolColors.Text;}

            planAi.Click += delegate {
                StopAi();
                try {
                    MapInteraction target = interactionList.SelectedItem as MapInteraction;
                    if(target==null || target.kind!="exit")throw new InvalidDataException("Select an exit in the interaction list first.");
                    MapMarker marker=Array.Find(aiSnapshot.Live.exits,delegate(MapMarker e){return e.address==target.address;});
                    if(marker==null)throw new InvalidDataException("That exit is no longer loaded.");
                    aiRoute=NavigationPlanner.ToExit(aiSnapshot,marker);canvas.Route=aiRoute;canvas.RouteBlocked=false;canvas.Invalidate();
                    aiStatus.Text=(aiRoute.ApproachOnly?"Verified approach to blocked door: ":"Collision-checked candidate: ")+aiRoute.Points.Count+" waypoints. Body allowance R20 / H80; parkour needs verification.";
                }catch(InvalidDataException error){aiRoute=null;canvas.Route=null;aiStatus.Text=error.Message;}
            };
            startAi.Click += delegate {
                if(explorer.Busy){aiStatus.Text="Wait for the current action to stop.";return;}
                explorer.Stop("Explorer stopped for manual route");
                if(aiRoute==null || aiSnapshot==null || !aiSnapshot.IsLive || !aiSnapshot.Live.clearing_active ||
                    aiRoute.Level!=aiSnapshot.Live.level || aiRoute.Generation!=aiSnapshot.Live.generation){aiStatus.Text="Plan a route in active gameplay first.";return;}
                try {
                    explorer.JumpButton=aiControls.SelectedIndex==0?8:32768;
                    explorer.StartRoute(aiSnapshot,aiRoute,NavigationExplorer.Clock);aiStatus.Text=explorer.Status;
                }catch(InvalidDataException error){StopAi();aiStatus.Text=error.Message;}
            };
            explore.Click += delegate {
                StopAi();aiRoute=null;canvas.Route=null;
                try {
                    if(explorerError!=null)throw new InvalidDataException(explorerError);
                    explorer.JumpButton=aiControls.SelectedIndex==0?8:32768;
                    explorer.Start(aiSnapshot,NavigationExplorer.Clock);aiStatus.Text=explorer.Status;RefreshMap();
                }catch(InvalidDataException error){aiStatus.Text=error.Message;}
            };
            retry.Click+=delegate {
                StopAi();
                try {if(explorerError!=null)throw new InvalidDataException(explorerError);explorer.RetryRoom(aiSnapshot,NavigationExplorer.Clock);aiStatus.Text=explorer.Status;}
                catch(InvalidDataException error){aiStatus.Text=error.Message;}
            };
            KeyPreview=true;KeyDown+=delegate(object sender,KeyEventArgs args){if(args.KeyCode==Keys.Escape){StopAi();args.Handled=true;}};
            stopAi.Click += delegate { StopAi(); };
            FormClosing += delegate { StopAi(); };
            timer.Interval = 200; timer.Tick += delegate { RefreshMap(); };
            BindDirectory(path); timer.Start();
            FormClosed += delegate { timer.Stop(); timer.Dispose(); };
        }
        private void OpenExports() {
            if(String.IsNullOrEmpty(directory)||!Directory.Exists(directory)) {
                MessageBox.Show(this,"No export folder is available yet. Start a game and load a room first.","Live map exports",MessageBoxButtons.OK,MessageBoxIcon.Information);return;
            }
            try {System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo{FileName=directory,UseShellExecute=true,Verb="open"});}
            catch(Exception error) {
                if(!(error is System.ComponentModel.Win32Exception)&&!(error is IOException)&&!(error is UnauthorizedAccessException))throw;
                MessageBox.Show(this,"Cannot open the export folder.\n\n"+directory+"\n\n"+error.Message,"Live map exports",MessageBoxButtons.OK,MessageBoxIcon.Error);
            }
        }
        internal void BindDirectory(string path) {
            StopAi();aiRoute=null;canvas.Route=null;selectedEntity=0;directory=LocalSetup.FullPath(path);cached=null;explorerError=null;
            try { explorer=AutonomousExplorer.Load(directory); }
            catch(Exception error) {
                if(!(error is IOException) && !(error is UnauthorizedAccessException))throw;
                explorer=new AutonomousExplorer(directory,new NavigationExplorer());explorerError="Cannot load exploration history: "+error.Message;aiStatus.Text=explorerError;
            }
            canvas.UpdateMap(null);UpdateInteractions(null);RefreshMap();
        }
        private void MapUnavailable() {
            if(explorer.Running) {
                ExploreCommand command=explorer.MissingMap(NavigationExplorer.Clock);
                if(command.Stop)StopPilot();aiStatus.Text=explorer.Status;
            }else StopPilot();
            aiSnapshot=null;cached=null;canvas.UpdateMap(null);UpdateInteractions(null);
            SyncMods(null);
        }
        internal void RefreshMap()
        {
            try {
                MapSnapshot value = MapSnapshot.Load(directory, cached);
                if(aiSnapshot!=null && (aiSnapshot.Live.level!=value.Live.level || aiSnapshot.Live.generation!=value.Live.generation))selectedEntity=0;
                aiSnapshot=value;SyncMods(value);
                if(explorer.Running) {
                    ExploreCommand command=explorer.Tick(value,NavigationExplorer.Clock);
                    if(command.Stop){StopPilot();aiRoute=null;canvas.Route=null;}
                    if(command.Confirm)NavigationRoute.ConfirmTransition(directory,value.Live,++aiNonce);
                    if(command.Route!=null) {
                        aiRoute=command.Route;canvas.Route=aiRoute;canvas.RouteBlocked=false;
                    }
                    aiStatus.Text=explorer.Status;
                }
                if(explorer.Busy){aiRoute=explorer.ActiveRoute;canvas.Route=aiRoute;}
                if(explorerError==null) {explorer.ObserveIdle(value,NavigationExplorer.Clock);explorer.Save(directory);}
                cached = value.Mesh; canvas.UpdateMap(value); UpdateInteractions(value);EntityDetails(value);
                if (canvas.Mode != 1) layerHeight.Value = Math.Max(layerHeight.Minimum, Math.Min(layerHeight.Maximum, (decimal)canvas.CenterHeight));
                int tribalCount = 0;
                foreach (MapMarker npc in value.Live.npcs)
                    if (npc.kind == "tribal") tribalCount++;
                status.Text = (value.IsLive ? ((value.Live.gameplay_active || value.Live.clearing_active) ? "LIVE" : "LIVE - scripted scene / controls suspended") : "Saved map - game closed, paused, or no longer exporting")
                    + "  |  " + value.Live.exits.Length + " exits  |  " + value.Live.markers.Length + " items  |  "
                    + (value.Live.npcs.Length - tribalCount) + " NPCs  |  " + tribalCount + " Tribals"
                    + " | Collision: " + (value.Live.collision==null || !value.Live.collision.known?"unknown":value.Live.collision.models.Length+" models");
            }
            catch (InvalidDataException error) { MapUnavailable(); status.Text = error.Message; }
            catch (IOException) { MapUnavailable(); status.Text = "Waiting for the game to export a room..."; }
            catch (SerializationException) { MapUnavailable(); status.Text = "Waiting for a complete map update..."; }
            catch (UnauthorizedAccessException) { MapUnavailable(); status.Text = "Cannot read this export folder."; }
        }
    }
}
