
  std::string modernHighlightHtml(int binding,int family) const {
    constexpr std::array<int,18> pads{-10,-11,0,1,2,3,23,25,9,10,7,8,11,12,13,14,6,4};
    constexpr std::array<int,8> mouse{1,2,4,5,6,256,257,-12};
    int index=-1;
    if(family==2) {
      const auto it=std::find(mouse.begin(),mouse.end(),binding);
      if(it!=mouse.end())index=36+int(it-mouse.begin());
    } else {
      const auto it=std::find(pads.begin(),pads.end(),binding);
      if(it!=pads.end())index=family*18+int(it-pads.begin());
    }
    if(index<0)return "";
    struct Highlight {int x,y,w,h;double left,top,width,height;};
    static constexpr Highlight highlights[]{
      #include "modern_controls_highlight_rects.inl"
    };
    const auto& h=highlights[index];
    return "<img class=\"mc-art-highlight\" src=\"controls-art/highlights.tga\" rect=\""+
      std::to_string(h.x)+" "+std::to_string(h.y)+" "+std::to_string(h.w)+" "+std::to_string(h.h)+
      "\" style=\"left:"+std::to_string(h.left)+"%;top:"+std::to_string(h.top)+"%;width:"+std::to_string(h.width)+"%;height:"+std::to_string(h.height)+"%;\"/>";
  }

  std::string modernDevicePickerHtml() {
    std::string html="<button class=\"mc-device-dismiss\" data-action=\"device-cancel\" aria-label=\"Close device list\"/><div id=\"device-picker\" class=\"mc-device-menu\"><div class=\"mc-device-menu-heading\">Connected devices available to Player "+std::to_string(player+1)+"</div>";
    int available=0;
    for(int d=0;d<int(jfg::kControllerDeviceCount);++d)if(deviceAvailable(d)) {
      ++available;
      const bool selected=devices[player]==d;
      const auto* info=controllerCatalog.get(d);
      html+=mcControl("mc-device-pick-"+std::to_string(d),
        "<span class=\"mc-device-option-copy\"><strong>"+escape(deviceName(d))+"</strong><span>"+
        std::string(d<4?"XInput · Xbox layout":info && info->session_only?"Connected · select again after reconnecting":"Connected · saved device identity")+
        "</span></span><span class=\"mc-device-option-tag\">"+(selected?"Selected":"Available")+"</span>","mc-device-option",selected);
    }
    if(!available)html+="<p class=\"mc-device-empty\">No unused controllers are connected.</p>";
    if(deviceAvailable(-3))html+=mcControl("mc-device-pick--3","<span class=\"mc-device-option-copy\"><strong>Mouse and keyboard</strong><span>Keys, mouse buttons and mouse look</span></span><span class=\"mc-device-option-tag\">"+std::string(pcKeyboard()?"Selected":"Available")+"</span>","mc-device-option",pcKeyboard());
    html+="<div class=\"mc-device-menu-footer\">"+button("device-refresh","Refresh")+button("device-cancel","Cancel")+"</div></div>";
    return html;
  }
// Presentation follows the owner-supplied Idea 3 prototype. Native RmlUi only.
  std::string mcControl(const std::string& action,const std::string& body,const std::string& cls="",bool selected=false,bool disabled=false) const {
    const bool close=cls.find("mc-overlay-close")!=std::string::npos;
    const bool clear=cls.find("mc-row-clear")!=std::string::npos;
    const std::string content=close || clear?"<span class=\"mc-close-icon\"><span/><span/></span>":
      body.find('<')==std::string::npos?"<span>"+body+"</span>":body;
    return "<button id=\"action-"+action+"\" data-action=\""+action+"\" class=\"mc-button "+cls+
      (selected?" selected":"")+(disabled?" disabled":"")+"\""+(disabled?" disabled=\"disabled\"":"")+
      (close?" aria-label=\"Close dialog\"":clear?" aria-label=\"Clear binding\"":"")+">"+content+"</button>";
  }
  unsigned modernShownContext() const {return jfg::kModernGameplay | jfg::kModernScoped | jfg::kModernMenus;}
  std::string modernShownActions(int binding) const {
    if(binding==-12)return "Look";
    auto result=modernActionNames(binding,modernShownContext());
    if(result=="Look / aim")return "Look";
    if(result=="Unassigned")return "Free";
    return result;
  }
  std::string modernChip(int binding,bool selected=false,int size=32) const {
    const bool keyboard=pcKeyboard(),ps=modernGlyphs()==jfg::ModernGlyphStyle::playstation;
    constexpr std::array<int,18> pads{-10,-11,0,1,2,3,23,25,9,10,7,8,11,12,13,14,6,4};
    constexpr std::array<int,8> mouse{1,2,4,5,6,256,257,-12};
    int sprite=-1;
    if(keyboard) {
      const auto it=std::find(mouse.begin(),mouse.end(),binding);
      if(it!=mouse.end())sprite=(54+int(it-mouse.begin()))*2+int(selected);
    } else {
      const auto it=std::find(pads.begin(),pads.end(),binding);
      const int family=ps?1:modernGlyphs()==jfg::ModernGlyphStyle::xbox?0:2;
      if(it!=pads.end())sprite=(family*18+int(it-pads.begin()))*2+int(selected);
    }
    if(sprite>=0) {
      struct Rect {int x,y,w,h;};
      static constexpr Rect rects[]{
        #include "modern_controls_chip_rects.inl"
      };
      const auto& rect=rects[sprite];
      return "<img class=\"mc-prototype-chip\" src=\"controls-art/chips.tga\" rect=\""+std::to_string(rect.x)+" "+std::to_string(rect.y)+" "+std::to_string(rect.w)+" "+std::to_string(rect.h)+
        "\" style=\"width:"+std::to_string(float(size)*rect.w/rect.h)+"dp;height:"+std::to_string(size)+"dp;\"/>";
    }
    std::string text=modernInputName(binding),kind="round",body;
    if(binding==-12){text="";kind="mouse";body="<span class=\"mc-mouse-icon\"><span class=\"mc-mouse-motion\">+</span></span>";}
    else if(keyboard) {
      if(binding<=6 || binding>=256) {
        kind="mouse";std::string detail=binding==1?"left":binding==2?"right":binding==4?"middle":binding==256?"wheel-up":binding==257?"wheel-down":"side";
        body="<span class=\"mc-mouse-icon "+detail+"\"><span/></span>";
      } else {kind="key";if(binding==VK_BACK)text="Bksp";body=escape(text);}
    } else if(binding>=0 && binding<4 && ps) {
      body=std::array<std::string,4>{"<span class=\"mc-glyph-cross\"><span/><span/></span>","<span class=\"mc-glyph-circle\"/>","<span class=\"mc-glyph-square\"/>","<span class=\"mc-glyph-triangle\"/>"}[binding];
    } else if(binding>=11 && binding<=14) {
      body="<span class=\"mc-dpad-icon\">";
      for(int i=11;i<=14;++i)body+="<span class=\"d"+std::to_string(i)+(i==binding?" active":"")+"\"/>";
      body+="<span class=\"dc\"/></span>";
    } else if(binding==6){body="<span class=\"mc-menu-icon\"><span/><span/><span/></span>";}
    else if(binding==4){body="<span class=\"mc-view-icon\"><span/><span/></span>";}
    else {
      if(binding==-10)text=ps?"L":"LS";if(binding==-11)text=ps?"R":"RS";
      if(binding==7)text=ps?"L3":"LSB";if(binding==8)text=ps?"R3":"RSB";
      if(binding==23 || binding==25)kind=binding==23?"trigger-left":"trigger-right";
      if(binding==9 || binding==10)kind="bumper";
      if(text.size()>8)text=binding==0?"S":binding==1?"E":binding==2?"W":binding==3?"N":text;
      body=escape(text);
    }
    const int width=(kind=="bumper" || kind.starts_with("trigger"))?size*41/32:kind=="key"?std::max(size,int(text.size())*size/4+size/2):size;
    return "<span class=\"mc-chip "+kind+(selected?" on":"")+"\" style=\"width:"+std::to_string(width)+"dp;height:"+std::to_string(size)+"dp;font-size:"+std::to_string(text.size()>2?size*10/32:size*14/32)+"dp;\">"+(body.find('<')==std::string::npos?"<span>"+body+"</span>":body)+"</span>";
  }
  std::string modernInputDescription(int binding) const {
    if(binding==-12)return "Mouse movement";
    if(pcKeyboard())return modernInputName(binding);
    if(binding==-10)return "Left stick";if(binding==-11)return "Right stick";
    constexpr const char* names[]={"A button","B button","X button","Y button","View button","Guide button","Menu button","Left stick click","Right stick click","Left bumper","Right bumper","D-pad up","D-pad down","D-pad left","D-pad right"};
    if(binding>=0 && binding<15) {
      if(modernGlyphs()!=jfg::ModernGlyphStyle::xbox && binding<7)return modernInputName(binding);
      return names[binding];
    }
    if(binding==23)return "Left trigger";if(binding==25)return "Right trigger";
    return modernInputName(binding);
  }
  std::string modernHeaderHtml() {
    std::string html="<div class=\"mc-profile-header\"><div class=\"tabs mc-players\">";
    for(int p=0;p<4;++p) {
      const int d=devices[p];const bool on=d==-3 || (d>=0 && connected[d]);
      html+="<button id=\"action-player"+std::to_string(p)+"\" data-action=\"player"+std::to_string(p)+"\" class=\""+(p==player?"selected":"")+"\"><span class=\"tab-title\">PLAYER "+std::to_string(p+1)+"</span><span class=\"mc-player-device\"><span class=\"mc-dot "+(on?"online":"")+"\"/><span class=\"tab-device\" id=\"tab-device"+std::to_string(p)+"\">"+escape(deviceName(d))+"</span></span></button>";
    }
    const int d=devices[player];const bool online=d==-3 || (d>=0 && connected[d]);
    html+="</div><div class=\"mc-device\"><div class=\"mc-device-text\"><strong>"+escape(deviceName(d))+"</strong><span id=\"assignment\"/></div><span class=\"mc-connected "+std::string(online?"online":"")+"\"><span class=\"mc-dot\"/>"+"<span>"+(online?"Connected":"Disconnected")+"</span></span>"+
      mcControl("device-open","<span>"+std::string(mappings[player].device==-2?"Add device":"Change device")+"</span><span class=\"mc-chevron\"/>","mc-device-change")+
      mcControl("device-remove","Remove","",false,mappings[player].device==-2)+"</div></div>";
    if(devicePicker && modernMode())html+=modernDevicePickerHtml();
    else if(devicePicker) {
      const auto legacy=deviceSetupHtml();const auto start=legacy.find("<div id=\"device-picker\"");
      if(start!=std::string::npos)html+="<div class=\"mc-device-popover\">"+legacy.substr(start)+"</div>";
    }
    return html;
  }
  std::string modernStyleHtml() {
    std::string html="<div class=\"mc-style\"><span class=\"mc-section-title mc-style-label\">CONTROL<br/>STYLE</span><div class=\"mc-style-options\">";
    for(bool active:{false,true}) {
      const bool selected=modernMode()==active;
      html+=mcControl(active?"mc-style-modern":"mc-style-mapping",
        "<img class=\"mc-radio\" src=\"controls-art/ui-details.tga\" rect=\""+std::string(selected?"64 128 64 64":"0 128 64 64")+"\"/><span class=\"mc-style-copy\"><span class=\"mc-style-name\">"+
        std::string(active?"Modern Controls <span class=\"mc-badge\">EXPERIMENTAL</span>":"Controller Mapping")+
        "</span><span class=\"mc-style-desc\">"+(active?(pcKeyboard()?"Modern PC controls: WASD to move, mouse to look and aim.":"Movement, aiming and action bindings made for modern controllers."):(pcKeyboard()?"Map keys and mouse buttons to the original game controls.":"Map your inputs to the original game controls."))+"</span></span>",
        "mc-style-choice",selected,active && (player!=0 || modernInvalid[player]));
    }
    const std::string note=player!=0?"Modern Controls is available for Player 1 in single-player.":"Modern Controls is experimental.";
    html+="</div><span class=\"mc-style-note\">"+note+"</span></div>";
    if(modernInvalid[player])html+="<div class=\"mc-warning\">Modern profile could not be read. "+modernButton("mc-repair","Restore modern defaults")+"</div>";
    return html;
  }
  std::string modernFooterHtml() {
    return modernButton("mc-reset",(modernSettings || modernAdvanced)?"Reset sensitivity & behavior":"Restore modern defaults");
  }
  struct ModernGroup {std::string title;std::vector<int> inputs;bool add=false;};
  std::vector<ModernGroup> modernGroups() const {
    if(!pcKeyboard()) {
      std::vector<ModernGroup> groups{{"TRIGGERS & BUMPERS",{23,25,9,10}},{"FACE BUTTONS",{0,1,2,3}},{"STICKS",{-10,-11,7,8}},{"D-PAD",{11,12,13,14}},{"SYSTEM",{6,4}}};
      std::vector<int> extra;
      for(int b:modern[player].pad)if(b>=15 && b!=23 && b!=25 && std::find(extra.begin(),extra.end(),b)==extra.end())extra.push_back(b);
      if(!extra.empty())groups.push_back({"OTHER INPUTS",extra});
      return groups;
    }
    // Keep the familiar left-hand movement keys in place. Every other bound
    // keyboard key belongs to Keys, regardless of its action or input context.
    const std::vector<int> moves{'W','A','S','D'};
    std::vector<int> keys;
    for(int b:modern[player].keys) {
      if(b>6 && b<256 && !jfg::modern_arrow_key(b) &&
        std::find(moves.begin(),moves.end(),b)==moves.end() &&
        std::find(keys.begin(),keys.end(),b)==keys.end())keys.push_back(b);
    }
    std::vector<int> mouse{-12,1,2,4,256,257};
    for(int b:{5,6})if(std::find(modern[player].keys.begin(),modern[player].keys.end(),b)!=modern[player].keys.end())mouse.push_back(b);
    return {{"MOUSE",mouse},{"MOVEMENT KEYS",moves},{"KEYS",keys,true}};
  }
  std::vector<int> modernInputs() const {
    std::vector<int> out;for(const auto& group:modernGroups())for(int b:group.inputs)out.push_back(b);return out;
  }
  std::string modernInputNote(int binding) const {
    if(binding==-12)return "Always the mouse";
    const auto& c=modern[player];const auto& binds=pcKeyboard()?c.keys:c.pad;
    int count=0,found=-1;
    for(int i=0;i<int(jfg::kModernActionCount);++i)if(binds[i]==binding && (jfg::kModernActions[i].contexts&modernShownContext())){++count;found=i;}
    if(count>1)return "Shared";
    if(found==int(jfg::ModernAction::aim))return c.aim_toggle?"Toggle":"Hold";
    if(found>=int(jfg::ModernAction::scope_cycle) && found<=int(jfg::ModernAction::scope_decrease))return "While scoped";
    return count || binding<0?"":"Bind an action";
  }
  std::string modernGroupHtml(const ModernGroup& group,bool diagram=false) {
    std::string html="<section"+std::string(group.title=="KEYS"?" id=\"mc-gameplay-keys\"":"")+" class=\"mc-binding-group"+std::string(pcKeyboard() && group.title!="MOUSE"?" mc-key-group":"")+"\"><div class=\"mc-section-title\">"+escape(group.title)+"</div>";
    for(int binding:group.inputs) {
      const std::string number=std::to_string(binding),actions=modernShownActions(binding),note=modernInputNote(binding);
      const bool free=actions=="Free",selected=modernSelectedInput()==binding;
      if(diagram) {
        html+=mcControl("mc-select-"+number,modernChip(binding,selected,30)+"<span class=\"mc-diagram-row-title"+std::string(free?" free":"")+"\">"+escape(actions)+"</span><span class=\"mc-row-note\">"+escape(note)+"</span>","mc-diagram-row",selected);
      } else {
        html+="<div class=\"mc-binding-row"+std::string(selected?" selected":"")+"\"><span class=\"mc-chip-space\">"+modernChip(binding,selected)+"</span><span class=\"mc-physical-name\">"+escape(modernInputDescription(binding))+"</span>";
        const auto action=binding==-12?"mc-mouse-settings":"mc-input-"+number;
        html+="<button id=\"mc-row-"+number+"\" data-action=\""+action+"\" class=\"mc-binding-value"+std::string(free?" free":"")+"\"><span>"+escape(actions)+"</span><span class=\"mc-row-note\">"+escape(note)+"</span></button>";
        html+=mcControl("mc-free-"+number,"×","mc-row-clear"+std::string(free || binding<0?" invisible":""));
        html+="</div>";
      }
    }
    if(group.add)html+=mcControl("mc-add-key","+ Add a key or button","mc-add-key",false,!modernOnline());
    return html+"</section>";
  }

  std::string modernHowHtml() {
    const auto& c=modern[player];const auto& binds=pcKeyboard()?c.keys:c.pad;
    const auto key=[&](jfg::ModernAction a){
      const int b=binds[std::size_t(a)];
      if(!pcKeyboard() && modernGlyphs()==jfg::ModernGlyphStyle::xbox && (b==7 || b==8))return std::string(b==7?"LSB":"RSB");
      return modernInputName(b);
    };
    const auto instruction=[&](jfg::ModernAction a,const char* verb){
      return binds[std::size_t(a)]==(pcKeyboard()?0:-1)?
        "Bind "+escape(std::string(jfg::kModernActions[std::size_t(a)].label))+" to "+verb+".":
        "Use "+escape(key(a))+" to "+verb+".";
    };
    std::string html="<section class=\"mc-how mc-binding-group\"><div class=\"mc-section-title\">HOW IT PLAYS</div><p class=\"mc-how-scope\">";
    html+=binds[0]==(pcKeyboard()?0:-1)?"Bind Aim to use the sniper scope.":
      std::string(c.aim_toggle?"Press ":"Hold ")+escape(key(jfg::ModernAction::aim))+" to aim. "+
      (binds[7]==(pcKeyboard()?0:-1)?"Bind Cycle magnification to change zoom.":"While using the sniper scope, press "+escape(key(jfg::ModernAction::scope_cycle))+" to change magnification.");
    html+="</p><p>"+std::string(pcKeyboard()?"Move and strafe with "+escape(key(jfg::ModernAction::move_forward)+" "+key(jfg::ModernAction::move_left)+" "+key(jfg::ModernAction::move_back)+" "+key(jfg::ModernAction::move_right))+" and turn with the mouse.":c.movement_stick?"Move and strafe with the right stick; turn with the left stick.":"Move and strafe with the left stick; turn with the right stick.")+"</p>";
    const auto bound=[&](jfg::ModernAction a){return binds[std::size_t(a)]!=(pcKeyboard()?0:-1);};
    html+="<p>"+(bound(jfg::ModernAction::fire)?escape(key(jfg::ModernAction::fire))+" fires.":instruction(jfg::ModernAction::fire,"fire"))+"</p>";
    html+="<p>"+(bound(jfg::ModernAction::jump)&&bound(jfg::ModernAction::crouch)&&bound(jfg::ModernAction::interact)?
      escape(key(jfg::ModernAction::jump))+" jumps, "+escape(key(jfg::ModernAction::crouch))+" crouches and "+escape(key(jfg::ModernAction::interact))+" interacts.":
      instruction(jfg::ModernAction::jump,"jump")+" "+instruction(jfg::ModernAction::crouch,"crouch")+" "+instruction(jfg::ModernAction::interact,"interact"))+"</p>";
    html+="<p>"+(bound(jfg::ModernAction::weapon_previous)&&bound(jfg::ModernAction::weapon_next)?
      escape(key(jfg::ModernAction::weapon_previous))+" and "+escape(key(jfg::ModernAction::weapon_next))+" switch weapons.":
      instruction(jfg::ModernAction::weapon_previous,"select the previous weapon")+" "+instruction(jfg::ModernAction::weapon_next,"select the next weapon"))+"</p>";
    html+="<p>"+(bound(jfg::ModernAction::pause)?escape(key(jfg::ModernAction::pause))+" pauses the game.":instruction(jfg::ModernAction::pause,"pause the game"))+"</p>";
    if(modernContext==jfg::kModernScoped)html+="<p class=\"mc-hint\">Jump keeps your aim and sights active. Leave aim before crouching.</p>";
    html+="<div class=\"mc-how-menus\">In menus and dialogue, "+escape(key(jfg::ModernAction::menu_confirm))+" confirms, advances speech or closes an information page, and "+escape(key(jfg::ModernAction::menu_back))+" goes back. Save from the pause menu with "+escape(key(jfg::ModernAction::menu_save))+". "+std::string(pcKeyboard()?"Movement keys also navigate menus; right-click goes back. ":"")+"Menu inputs can share with gameplay.</div></section>";
    return html;
  }
  std::string modernRowsHtml() {
    const auto groups=modernGroups();std::string html="<div class=\"mc-columns\">";
    if(pcKeyboard()) {
      html+="<div id=\"mc-column-0\" class=\"mc-column\">"+modernGroupHtml(groups[0])+modernGroupHtml(groups[1])+"</div><div id=\"mc-column-1\" class=\"mc-column\">"+modernGroupHtml(groups[2])+"</div><div id=\"mc-column-2\" class=\"mc-column\">"+modernHowHtml()+"</div>";
    } else {
      html+="<div id=\"mc-column-0\" class=\"mc-column\">"+modernGroupHtml(groups[0])+modernGroupHtml(groups[1])+"</div><div id=\"mc-column-1\" class=\"mc-column\">"+modernGroupHtml(groups[2])+modernGroupHtml(groups[3]);
      if(groups.size()>5)html+=modernGroupHtml(groups[5]);
      html+="</div><div id=\"mc-column-2\" class=\"mc-column\">"+modernGroupHtml(groups[4])+modernHowHtml()+"</div>";
    }
    return html+"</div>";
  }


  int modernSelectedInput() const {return modernSelected==-100 && modernMode()?(pcKeyboard()?VK_LBUTTON:0):modernSelected;}
  std::string modernKeyboardArt() {
    struct Key {int code,x,y,w;};
    const std::vector<Key> keys{{'1',64,10,50},{'2',120,10,50},{'3',176,10,50},{'4',232,10,50},{'5',288,10,50},
      {9,0,68,58},{'Q',64,68,50},{'W',120,68,50},{'E',176,68,50},{'R',232,68,50},{'T',288,68,50},{8,352,68,84},
      {'A',78,126,50},{'S',134,126,50},{'D',190,126,50},{'F',246,126,50},{'G',302,126,50},{13,366,126,70},
      {16,0,184,86},{'Z',92,184,50},{'X',148,184,50},{'C',204,184,50},{'V',260,184,50},{17,0,242,58},{18,64,242,50},{32,120,242,232}};
    std::string html="<div class=\"mc-keyboard-art\">";
    for(const auto& key:keys) {
      auto actions=modernShownActions(key.code);if(actions=="Free")actions="";
      if(actions=="Move forward")actions="Move";else if(actions=="Move back")actions="Move";
      else if(actions=="Move left" || actions=="Move right")actions="Strafe";else if(actions=="Full movement")actions="Run";
      html+="<button id=\"mc-key-"+std::to_string(key.code)+"\" data-action=\"mc-select-"+std::to_string(key.code)+"\" class=\"mc-key"+std::string(!actions.empty()?" bound":"")+(modernSelectedInput()==key.code?" selected":"")+"\" style=\"left:"+std::to_string(key.x)+"dp;top:"+std::to_string(key.y)+"dp;width:"+std::to_string(key.w)+"dp;\"><strong>"+escape(key.code==VK_BACK?"Bksp":pcKeyName(key.code))+"</strong><span>"+escape(actions)+"</span></button>";
    }
    std::string others;std::vector<int> shown;
    for(int b:modern[player].keys)if(b>6 && b<256 && modernShownActions(b)!="Free" && std::none_of(keys.begin(),keys.end(),[&](const Key& k){return k.code==b;}) && std::find(shown.begin(),shown.end(),b)==shown.end()) {
      shown.push_back(b);others+=mcControl("mc-key-other-"+std::to_string(b),escape(pcKeyName(b))+" <span>"+escape(modernShownActions(b))+"</span>","mc-other-key");
    }
    if(!others.empty())html+="<span class=\"mc-other-title\">OTHER BOUND KEYS</span><div class=\"mc-other-keys\">"+others+"</div>";
    html+="<div class=\"mc-mouse-art\"><img class=\"mc-mouse-base\" src=\"controls-art/mouse.tga\"/>"+modernHighlightHtml(modernSelectedInput(),2);
    struct MouseSpot {int binding,x,y,w,h;};
    for(const auto& spot:std::vector<MouseSpot>{{1,16,5,44,61},{2,61,5,45,61},{4,54,17,14,34},{5,7,111,13,20},{6,7,87,13,20},{-12,44,145,39,39}})
      html+="<button id=\"mc-mouse-hit-"+std::to_string(spot.binding)+"\" data-action=\"mc-select-"+std::to_string(spot.binding)+"\" aria-label=\""+escape(modernInputDescription(spot.binding))+"\" class=\"mc-mouse-hit"+std::string(modernSelectedInput()==spot.binding?" selected":"")+"\" style=\"left:"+std::to_string(spot.x)+"dp;top:"+std::to_string(spot.y)+"dp;width:"+std::to_string(spot.w)+"dp;height:"+std::to_string(spot.h)+"dp;\"/>";
    html+="</div><div class=\"mc-mouse-legend\">";
    for(int b:{1,2,4,256,257,5,6,-12})html+=mcControl("mc-mouse-select-"+std::to_string(b),"<span>"+escape(modernInputDescription(b))+"</span><strong>"+escape(modernShownActions(b))+"</strong>","mc-mouse-row",modernSelectedInput()==b);
    return html+"</div></div>";
  }
  std::string modernPadArt() {
    const bool ps=modernGlyphs()==jfg::ModernGlyphStyle::playstation;
    const char* art=ps?"dualsense":modernGlyphs()==jfg::ModernGlyphStyle::xbox?"xbox":"generic";
    std::string html="<div class=\"mc-pad-art "+std::string(ps?"ps":"xbox")+"\"><img src=\"controls-art/"+art+".tga\"/>"+modernHighlightHtml(modernSelectedInput(),ps?1:0);
    struct Spot {int binding;double x,y,w,h;};
    const std::vector<Spot> xbox{{-10,25.4,31,11.1,14.8},{-11,62.5,49.8,11.1,14.8},
      {23,27,6,15,8},{25,74,6,15,8},{9,25.5,12.5,17,7},{10,74.5,12.5,17,7},
      {0,74.5,39.4,6.5,8.6},{1,80.8,31,6.5,8.6},{2,68.1,31,6.5,8.6},{3,74.5,22.5,6.5,8.6},
      {7,25.4,31,4.5,6},{8,62.5,49.8,4.5,6},{4,43,30,4.6,6.1},{6,56.8,30,4.6,6.1},
      {11,37.4,43.5,5.1,7.4},{12,37.4,56.6,5.1,7.4},{13,32.7,49.9,5.1,7.4},{14,42,49.9,5.1,7.4}};
    const std::vector<Spot> playstation{{-10,34.8,51.2,12,17.5},{-11,65,51.2,12,17.5},
      {23,22,1,14,8},{25,78,1,14,8},{9,22,9,15,7},{10,78,9,15,7},
      {0,80,42,6.6,9.5},{1,86,33,6.6,9.5},{2,73.5,33,6.6,9.5},{3,80,24,6.6,9.5},
      {7,34.8,51.2,4.5,6.5},{8,65,51.2,4.5,6.5},{4,27.8,19.5,3,8},{6,72,19.5,3,8},
      {11,20.5,25,5,8},{12,20.5,41,5,8},{13,15,33,5,8},{14,26,33,5,8}};
    for(const auto& spot:ps?playstation:xbox)
      html+="<button id=\"mc-art-hit-"+std::to_string(spot.binding)+"\" data-action=\"mc-select-"+std::to_string(spot.binding)+"\" aria-label=\""+escape(modernInputDescription(spot.binding))+"\" class=\"mc-art-hit"+std::string(modernSelectedInput()==spot.binding?" selected":"")+((spot.binding>=9 && spot.binding<=14) || spot.binding==23 || spot.binding==25?" rect":"")+"\" style=\"left:"+std::to_string(spot.x)+"%;top:"+std::to_string(spot.y)+"%;width:"+std::to_string(spot.w)+"%;height:"+std::to_string(spot.h)+"%;\"/>";
    return html+"</div>";
  }
  std::string modernDiagramHtml() {
    const int selected=modernSelectedInput();
    std::string html="<div class=\"mc-diagram-layout\"><div id=\"mc-diagram-list\" class=\"mc-diagram-list\">";
    for(const auto& group:modernGroups())html+=modernGroupHtml(group,true);
    html+="</div><div class=\"mc-diagram-stage\"><div class=\"mc-art-area\">"+(pcKeyboard()?modernKeyboardArt():modernPadArt())+"</div>";
    html+="<div class=\"mc-current\">"+modernChip(selected,true,44)+"<span class=\"mc-current-copy\"><strong>"+escape(modernShownActions(selected))+"</strong><span>"+escape(modernInputDescription(selected))+" · "+escape(modernContextName(modernContext))+"</span></span>";
    const auto& binds=pcKeyboard()?modern[player].keys:modern[player].pad;int first=-1;
    for(int i=0;i<int(binds.size());++i)if(binds[i]==selected && (jfg::kModernActions[i].contexts&modernShownContext())){first=i;break;}
    if(first>=0)html+=mcControl("mc-capture-"+std::to_string(first),pcKeyboard()?"Move to another key...":"Move to another button...","mc-secondary");
    html+=mcControl(selected==-12?"mc-mouse-settings":"mc-input-"+std::to_string(selected),selected==-12?"Mouse settings":"Change action","mc-primary");
    html+="</div></div></div>";
    return html;
  }
  std::string modernRange(const std::string& id,const std::string& name,int value,int minimum,int maximum,const std::string& help="",bool percent=false) {
    return "<div class=\"mc-setting\"><div class=\"mc-setting-title\"><label for=\"mc-range-"+id+"\">"+escape(name)+"</label><span id=\"mc-value-"+id+"\">"+
      std::to_string(percent?(value*100+16383)/32767:value)+"%</span></div><input id=\"mc-range-"+id+"\" type=\"range\" class=\"volume\" min=\""+
      std::to_string(minimum)+"\" max=\""+std::to_string(maximum)+"\" step=\""+std::string(percent?"100":"5")+"\" value=\""+std::to_string(value)+"\" data-action=\"mc-tune-"+id+"\"/>"+
      (help.empty()?"":"<div class=\"mc-hint\">"+escape(help)+"</div>")+"</div>";
  }
  std::string modernSwitch(const std::string& action,const std::string& label,const std::string& help,bool active) {
    return "<div class=\"mc-switch-row\"><span><strong>"+escape(label)+"</strong><span class=\"mc-hint\">"+escape(help)+"</span></span>"+
      mcControl(action,"<span/>","mc-switch",active)+"</div>";
  }
  std::string modernSettingsHtml() {
    const auto& c=modern[player];const auto& look=c.look;const bool keyboard=pcKeyboard();
    std::string html="<section id=\"mc-camera-card\" class=\"mc-settings-card\"><div class=\"mc-section-title\">"+std::string(keyboard?"MOUSE":"CAMERA")+"</div>";
    html+=modernRange("camera",keyboard?"Mouse sensitivity":"Camera sensitivity",keyboard?look.mouse_sensitivity:look.camera_sensitivity,10,keyboard?500:300);
    if(keyboard)html+=modernRange("vertical","Mouse vertical sensitivity",look.mouse_vertical_sensitivity,10,300);
    html+=modernRange("aim","Aiming sensitivity",keyboard?look.mouse_aim_sensitivity:look.aim_sensitivity,10,keyboard?500:300);
    html+=modernSwitch("mc-invert-x","Invert horizontal",keyboard?"Move right to look left":"Push right to look left",keyboard?look.mouse_invert_x:look.aim_invert_x);
    html+=modernSwitch("mc-invert-y","Invert vertical",keyboard?"Move the mouse up to look down":"Push up to look down",keyboard?look.mouse_invert_y:look.aim_invert_y);
    const std::string camera=html+"</section>";html="<section id=\"mc-scope-card\" class=\"mc-settings-card\"><div class=\"mc-section-title\">AIM &amp; SCOPE</div><div class=\"mc-setting\"><strong>Aim behavior</strong><div class=\"mc-segment\">"+
      modernButton("mc-aim-hold","Hold to aim",!c.aim_toggle)+modernButton("mc-aim-toggle","Toggle aim",c.aim_toggle)+"</div><div class=\"mc-hint\">"+
      std::string(c.aim_toggle?"Press ":"Hold ")+escape(modernInputName((keyboard?c.keys:c.pad)[0]))+" to aim. Jump keeps your aim and sights active. Leave aim before crouching.</div></div>";
    html+="<div class=\"mc-setting\"><strong>Scope bindings</strong><div class=\"mc-segment\">"+
      modernButton("mc-scope-cycle","Cycle magnification",true)+modernButton("mc-scope-separate","Zoom in / out")+"</div><div class=\"mc-hint\">Press "+
      escape(modernInputName((keyboard?c.keys:c.pad)[7]))+" while scoped to step through zoom levels. Separate zoom bindings are optional.</div></div>";
    html+=modernRange("scope","Scope sensitivity",keyboard?c.mouse_scope_sensitivity:c.pad_scope_sensitivity,10,300);
    html+="<div class=\"mc-zoom-example\"><img class=\"mc-crosshair\" src=\"controls-art/ui-details.tga\" rect=\"128 128 176 176\"/><span><span class=\"mc-zoom-levels\"><span/><span/><span/></span><span class=\"mc-hint\">Low · Medium · High. Magnification slows aiming automatically.</span></span></div></section>";
    const std::string scope=html;std::string sticks,vibration;
    if(!keyboard) {
      html="<section id=\"mc-sticks-card\" class=\"mc-settings-card\"><div class=\"mc-section-title\">STICKS</div>"+
        modernRange("move-deadzone","Movement deadzone",c.movement_deadzone,0,30000,"",true)+
        modernRange("look-deadzone","Look deadzone",look.aim_deadzone,0,30000,"",true)+
        "<div class=\"mc-setting\"><strong>Response curve</strong><div class=\"mc-segment\">"+
        modernButton("mc-curve-0","Linear",look.aim_curve==0)+modernButton("mc-curve-1","Quadratic",look.aim_curve==1)+modernButton("mc-curve-2","Cubic",look.aim_curve==2)+
        "</div><div class=\"mc-hint\">"+std::string(look.aim_curve==0?"Stick movement maps evenly to turn speed.":look.aim_curve==1?"Gentler near the center, with the same speed at full deflection.":"Very fine near the center, with the same speed at full deflection.")+"</div></div></section>";
      sticks=html;html="<section id=\"mc-vibration-card\" class=\"mc-settings-card\"><div class=\"mc-section-title\">VIBRATION</div>";
      if(devices[player]>=0 && devices[player]<4)html+=modernRange("rumble","Vibration strength",look.rumble_strength,0,100);
      else html+="<div class=\"mc-capability-note\">Vibration is not available for this controller backend.</div>";
      html+="</section>";
    }
    if(!keyboard)vibration=html;
    return "<div id=\"mc-settings-scroll\" class=\"mc-settings-scroll\"><div class=\"mc-settings-grid\"><div class=\"mc-settings-column\">"+camera+sticks+"</div><div class=\"mc-settings-column\">"+scope+vibration+"</div></div></div>";
  }
  std::string modernAdvancedHtml() {
    const auto& c=modern[player];
    if(pcKeyboard())return "<div id=\"mc-settings-scroll\" class=\"mc-settings-scroll\"><div class=\"mc-settings-grid\"><section id=\"mc-movement-card\" class=\"mc-settings-card\"><div class=\"mc-section-title\">MOVEMENT</div>"+
      modernSwitch("mc-original-acceleration","Original movement acceleration",
        "Ease into movement and slow down gradually when releasing keys. Off gives immediate starts and stops. Top speed stays the same.",c.mouse_original_acceleration)+"</section></div></div>";
    return "<div id=\"mc-settings-scroll\" class=\"mc-settings-scroll\"><div class=\"mc-settings-grid\"><section class=\"mc-settings-card\"><div class=\"mc-section-title\">VERTICAL LOOK</div>"+
      modernRange("vertical","Vertical sensitivity",c.look.stick_vertical_sensitivity,10,300)+"</section><section class=\"mc-settings-card\"><div class=\"mc-section-title\">BINDING THRESHOLDS</div>"+
      modernRange("axis","Button axis threshold",c.axis_threshold,1000,32000,"",true)+modernRange("trigger","Trigger threshold",c.trigger_threshold,1000,32000,"",true)+"</section></div></div>";
  }
  std::string modernHtml() {
    const auto& c=modern[player];
    std::string html="<div id=\"modern-content\" class=\"mc-content\"><div id=\"mc-toolbar\" class=\"mc-toolbar\"><div class=\"mc-tabs\">"+
      mcControl("mc-tab-buttons",pcKeyboard()?"Keys &amp; buttons":"Buttons","mc-tab",!modernSettings && !modernAdvanced)+mcControl("mc-tab-settings","Sensitivity &amp; behavior","mc-tab",modernSettings)+mcControl("mc-tab-advanced","Advanced settings","mc-tab",modernAdvanced)+"</div><div class=\"mc-toolbar-options\">";
    if(!pcKeyboard()) {
      html+="<span class=\"mc-symbols-label\"><span>Button symbols</span></span><select id=\"mc-glyph-select\" data-action=\"mc-glyph-select\">";
      for(int i=0;i<3;++i)html+="<option value=\""+std::to_string(i)+"\""+(int(c.glyph_style)==i?" selected=\"selected\"":"")+">"+(i==0?modernAutomaticGlyphLabel():std::string(i==1?"Xbox":"PlayStation"))+"</option>";
      html+="</select>";
    }
    if(!modernSettings && !modernAdvanced)html+=mcControl(modernDiagram?"mc-view-list":"mc-view-diagram","<img class=\"mc-pad-icon\" src=\"controls-art/toolbar-pad.tga\"/><span>"+std::string(modernDiagram?"Hide ":"Show ")+(pcKeyboard()?"key map":"controller diagram")+"</span>","mc-view-toggle",modernDiagram);
    html+="</div></div><div id=\"mc-workspace\" class=\"mc-workspace\">";
    if(!modernOnline())html+="<div class=\"mc-offline-note\">Device disconnected. Saved bindings can be edited; capture requires a connected device.</div>";
    html+=modernAdvanced?modernAdvancedHtml():modernSettings?modernSettingsHtml():modernDiagram?modernDiagramHtml():modernRowsHtml();
    return html+"</div></div>";
  }

  std::string modernActionDescription(std::size_t index) const {
    static constexpr const char* descriptions[]{
      "Bring up precision aiming and the sniper scope.", "Fire the equipped weapon.",
      "Jump outside the sniper aiming stance.", "Crouch outside the sniper aiming stance.",
      "Interact with objects or start a conversation. Use Confirm to advance dialogue.", "Equip the previous weapon.",
      "Equip the next weapon.", "Step through sniper magnification levels, then wrap to the lowest.",
      "Increase magnification while scoped.", "Decrease magnification while scoped.",
      "Open or close the pause menu.", "Advance dialogue, confirm a menu option or close an information page.", "Go back in menus.",
      "Move forward.", "Move backward.", "Strafe left.", "Strafe right.",
      "Hold for full movement speed.", "Move up in menus.", "Move down in menus.",
      "Move left in menus.", "Move right in menus.", "Save from the pause menu when Save Game is available."
    };
    return index<jfg::kModernActionCount?descriptions[index]:"";
  }
  std::string modernPickerHtml(bool captureAction=false,bool scopeOnly=false) {
    struct ActionGroup {const char* title;std::vector<int> actions;};
    const std::vector<ActionGroup> groups{{"MOVEMENT & COMBAT",{13,14,15,16,17,0,1,2,3,4}},
      {"WEAPONS",{5,6}},{"SNIPER SCOPE",{7,8,9}},{"SYSTEM",{10}},{"MENUS",{11,12,18,19,20,21,22}}};
    const auto& binds=pcKeyboard()?modern[player].keys:modern[player].pad;
    std::string html="<div class=\"mc-picker-list\">";
    for(const auto& group:groups) {
      if(scopeOnly && std::string_view(group.title)!="SNIPER SCOPE")continue;
      std::string rows;
      for(int i:group.actions) {
        if(!pcKeyboard() && !jfg::kModernActions[i].gamepad)continue;
        const int binding=binds[i];const bool unbound=binding==(pcKeyboard()?0:-1);
        const bool assigned=!captureAction && binding==modernSelected;
        rows+=mcControl(std::string(captureAction?"mc-palette-":"mc-assign-")+std::to_string(i),
          "<span class=\"mc-option-copy\"><strong>"+escape(std::string(jfg::kModernActions[i].label))+"</strong><span>"+escape(modernActionDescription(i))+"</span></span>"+
          "<span class=\"mc-option-current\"><span>"+std::string(assigned?"Assigned (click to remove)":unbound?"Unbound":"Currently")+"</span>"+
          (unbound?"":modernChip(binding,false,26))+"</span>","mc-action-option",assigned);
      }
      if(!rows.empty())html+="<div class=\"mc-section-title\">"+std::string(group.title)+"</div>"+rows;
    }
    return html+"</div>";
  }
