// Native RmlUi Modern Controls editor. Included inside Ui; no browser runtime.
  std::array<jfg::ModernControlsConfig,4> modern{};
  std::array<bool,4> modernInvalid{};
  bool modernDeferredRefresh=false;
  bool modernSettings=false, modernDiagram=false, modernReleased=false, modernAdvanced=false, modernResetTuning=false;
  unsigned modernContext=jfg::kModernGameplay;
  int modernSelected=-100, modernCapture=-1, modernCapturePlayer=-1, modernCaptureDevice=-2;
  int modernPendingAction=-1, modernPendingBinding=-1;
  double modernDeadline=0;
  std::string modernPanel, modernNotice, modernReturnFocus, modernViewIdentity;
  std::optional<jfg::ModernControlsConfig> modernRetry;
  std::function<bool()> modernHasFocus=[this]{return GetForegroundWindow()==window;};
  std::function<bool()> modernKeysNeutral=[] {
    for(int k=1;k<256;++k)if(GetAsyncKeyState(k)&0x8000)return false;
    return true;
  };
  std::string modernFile(int p) const {
    return p?"modern-controls-"+std::to_string(p+1)+".ini":"modern-controls.ini";
  }
  void loadModern() {
    for(int p=0;p<4;++p) {
      modern[p]={};const auto text=read(modernFile(p).c_str());
      std::error_code ec;
      modernInvalid[p]=!text.empty()? !jfg::parse_modern_controls(text,modern[p]):
        fs::exists(profile/modernFile(p),ec);
    }
  }
  bool modernMode() const {return player==0 && modern[player].enabled;}
  bool modernOnline() const {
    return pcKeyboard() || (devices[player]>=0 && connected[devices[player]]);
  }
  jfg::ModernGlyphStyle modernGlyphs() const {
    auto family=jfg::ControllerFamily::generic;
    if(devices[player]>=0 && devices[player]<4)family=jfg::ControllerFamily::xbox;
    else if(const auto* info=controllerCatalog.get(devices[player]))family=info->family;
    return jfg::resolve_modern_glyph_style(modern[player].glyph_style,family);
  }
  std::string modernAutomaticGlyphLabel() const {
    auto family=jfg::ControllerFamily::generic;
    if(devices[player]>=0 && devices[player]<4)family=jfg::ControllerFamily::xbox;
    else if(const auto* info=controllerCatalog.get(devices[player]))family=info->family;
    const auto glyph=jfg::resolve_modern_glyph_style(jfg::ModernGlyphStyle::automatic,family);
    return std::string("Automatic · ")+(glyph==jfg::ModernGlyphStyle::xbox?"Xbox":glyph==jfg::ModernGlyphStyle::playstation?"PlayStation":"Generic");
  }
  std::string modernInputName(int binding) const {
    if(binding==-12)return "Mouse movement";
    if(binding==-10)return "Left stick";
    if(binding==-11)return "Right stick";
    return pcKeyboard()?pcKeyName(binding):jfg::modern_pad_name(binding,modernGlyphs());
  }
  static std::string modernContextName(unsigned context) {
    return context==jfg::kModernMenus?"Menus":context==jfg::kModernScoped?"Scoped aiming":"Gameplay";
  }
  std::string modernActionNames(int binding,unsigned context=0) const {
    if(binding==-10 || binding==-11)
      return (binding==-10?0:1)==modern[player].movement_stick?"Move / strafe":"Look / aim";
    const auto& bindings=pcKeyboard()?modern[player].keys:modern[player].pad;
    std::string result;
    for(std::size_t i=0;i<bindings.size();++i)
      if(bindings[i]==binding && (!context || (jfg::kModernActions[i].contexts&context))) {
        if(!result.empty())result+=" + ";
        result+=jfg::kModernActions[i].label;
      }
    return result.empty()?"Unassigned":result;
  }
  std::string modernButton(const std::string& action,const std::string& text,bool selected=false,bool disabled=false) {
    return "<button id=\"action-"+action+"\" data-action=\""+action+"\" class=\"mc-button"+
      (selected?" selected":"")+(disabled?" disabled":"")+"\""+(disabled?" disabled=\"disabled\"":"")+"><span>"+escape(text)+"</span></button>";
  }
  void modernCancel(const std::string& notice="") {
    modernCapture=-1;modernCapturePlayer=-1;modernPanel.clear();modernRetry.reset();
    modernPendingAction=-1;modernPendingBinding=-1;modernNotice=notice;
    if(current=="controllers")modal("controllers");
  }
  bool saveModern(const jfg::ModernControlsConfig& candidate) {
    if(modernInvalid[player])return false;
    if(!writeFile(modernFile(player),jfg::serialize_modern_controls(candidate))) {
      modernRetry=candidate;modernPanel="save-error";modernCapture=-1;
      modernNotice="Could not save. Your previous settings are still active.";
      modernOverlay();return false;
    }
    modern[player]=candidate;modernRetry.reset();modernPanel.clear();modernCapture=-1;
    modernNotice="Saved";
    return true;
  }
  #include "modern_controls_presentation.inl"

  void modernOverlay() {
    auto* host=el("modern-overlay");if(!host)return;
    if(current!="controllers" || modernPanel.empty()) {
      const bool wasOpen=host->GetNumChildren()>0;
      host->SetInnerRML("");host->SetProperty("display","none");
      if(wasOpen && current=="controllers") {
        auto* focus=el(modernReturnFocus.c_str());
        if(!focus)focus=el("action-mc-tab-buttons");
        if(focus)focus->Focus(true);
      }
      return;
    }
    std::string title,body,actions;
    if(modernPanel=="picker") {
      title=modernInputDescription(modernSelected);
      if(modernSelected==-10 || modernSelected==-11) {
        body="<p class=\"muted\">Choose a role for this stick. The other stick takes the remaining role.</p>"+
          mcControl("mc-stick-move","<span class=\"mc-option-copy\"><strong>Move / strafe</strong><span>Move in any direction without turning the camera.</span></span>","mc-action-option")+
          mcControl("mc-stick-look","<span class=\"mc-option-copy\"><strong>Look / aim</strong><span>Look around and aim independently of movement.</span></span>","mc-action-option");
      } else {
        body=modernPickerHtml();
        actions=mcControl("mc-free-"+std::to_string(modernSelected),pcKeyboard()?"Leave this key free":"Leave this button free","mc-picker-free");
      }
    } else if(modernPanel=="palette" || modernPanel=="scope-bindings") {
      title=modernPanel=="scope-bindings"?"Bind scope controls":"Choose an action to rebind";
      body="<p class=\"muted\">Choose an action, then press the key or controller input to use.</p>"+modernPickerHtml(true,modernPanel=="scope-bindings");
    } else if(modernPanel=="capture") {
      title=modernCapture==-2?"Add an input":"Bind "+std::string(jfg::kModernActions[modernCapture].label);
      body=std::string("<div class=\"mc-capture\"><div class=\"mc-capture-heading\"><span>WAITING FOR INPUT</span><span id=\"mc-capture-count\">12s</span></div>")+
        "<div class=\"mc-card-title\" id=\"mc-capture-status\">"+
        std::string(modernReleased?(pcKeyboard()?"Press a key, mouse button, or scroll the wheel.":"Press a button, pull a trigger, or move a stick."):"Release all controls first.")+
        "</div><div class=\"mc-capture-progress\"><span id=\"mc-capture-progress\"/></div><p class=\"muted\">"+
        escape(deviceName(devices[player]))+" · Connected</p><p class=\"muted\">Esc or Cancel keeps your current binding. F11 is reserved for fullscreen.</p></div>";
    } else if(modernPanel=="conflict") {
      title="This input already has an action";
      body="<p>Assign <strong>"+escape(modernInputName(modernPendingBinding))+"</strong> to <strong>"+
        escape(std::string(jfg::kModernActions[modernPendingAction].label))+"</strong>?</p><p>Also used by: ";
      const auto conflicts=jfg::modern_binding_conflicts(modern[player],pcKeyboard(),
        static_cast<jfg::ModernAction>(modernPendingAction),modernPendingBinding);
      bool first=true;
      for(std::size_t i=0;i<conflicts.size();++i)if(conflicts[i]) {
        if(!first)body+=", ";first=false;body+=escape(std::string(jfg::kModernActions[i].label));
      }
      body+="</p><div class=\"mc-conflict-options\">";
      const std::string incoming=jfg::kModernActions[modernPendingAction].label.data();
      body+=mcControl("mc-conflict-replace","<strong>Replace</strong><span>"+escape(incoming)+" moves here. Overlapping actions become unbound.</span>","mc-conflict-option");
      if(modernCanSwap())body+=mcControl("mc-conflict-swap","<strong>Swap</strong><span>Exchange these two bindings.</span>","mc-conflict-option");
      body+=mcControl("mc-conflict-both","<strong>Keep both</strong><span>Both actions respond when their gameplay contexts overlap.</span>","mc-conflict-option")+"</div>";
    } else if(modernPanel=="reset" || modernPanel=="repair") {
      title=modernPanel=="repair"?"Restore unreadable modern profile?":modernResetTuning?"Reset sensitivity and behavior?":"Restore defaults for this input type?";
      body="<p>"+std::string(modernPanel=="repair"?
        "The unreadable Modern Controls file will be backed up before a default profile is written.":
        modernResetTuning?"Reset tuning for this input type and restore hold-to-aim. Button bindings remain saved.":pcKeyboard()?"Reset keyboard and mouse bindings and sensitivity. Controller bindings and original mapping stay saved.":
        "Reset controller bindings, sticks, sensitivity, and vibration. Keyboard and mouse bindings and original mapping stay saved.")+"</p>";
      actions=modernButton(modernPanel=="repair"?"mc-repair-confirm":"mc-reset-confirm","Restore defaults");
    } else if(modernPanel=="save-error") {
      title="Settings could not be saved";
      body="<p>Your previous saved configuration is still active. Check that the profile folder is writable, then retry.</p>";
      actions=modernButton("mc-save-retry","Retry save");
    }
    actions+=modernButton("mc-cancel","Cancel");
    host->SetInnerRML("<div class=\"mc-overlay-shade\"><div class=\"mc-overlay-panel mc-panel-"+modernPanel+"\"><div class=\"mc-overlay-title\">"+
      (modernPanel=="picker"?modernChip(modernSelected,true,40):"")+"<span class=\"mc-overlay-heading\"><strong>"+escape(title)+"</strong>"+
      (modernPanel=="picker"?"<span>A key or button can have multiple actions. Click an assigned action to remove only that action.</span>":"")+"</span>"+
      mcControl("mc-dismiss","×","mc-overlay-close")+"</div><div id=\"mc-overlay-body\" class=\"mc-overlay-body\">"+body+"</div><div class=\"mc-overlay-actions\">"+actions+"</div></div></div>");
    host->SetProperty("display","block");
    if(auto* cancel=el("action-mc-cancel"))cancel->Focus(true);
  }
  bool modernCanSwap() const {
    if(modernPendingAction<0)return false;
    auto c=modern[player];auto& bindings=pcKeyboard()?c.keys:c.pad;
    const int old=bindings[modernPendingAction],unbound=pcKeyboard()?0:-1;
    if(old==unbound)return false;
    const auto conflicts=jfg::modern_binding_conflicts(c,pcKeyboard(),
      static_cast<jfg::ModernAction>(modernPendingAction),modernPendingBinding);
    int other=-1,count=0;
    for(std::size_t i=0;i<conflicts.size();++i)if(conflicts[i]){other=int(i);++count;}
    if(count!=1)return false;
    bindings[modernPendingAction]=modernPendingBinding;bindings[other]=unbound;
    const auto next=jfg::modern_binding_conflicts(c,pcKeyboard(),static_cast<jfg::ModernAction>(other),old);
    return std::none_of(next.begin(),next.end(),[](bool conflict){return conflict;});
  }
  void modernBind(int index,int binding,jfg::ModernBindPolicy policy=jfg::ModernBindPolicy::reject_conflict) {
    if(index<0 || index>=int(jfg::kModernActionCount))return;
    auto candidate=modern[player];
    const auto result=jfg::rebind_modern_action(candidate,pcKeyboard(),static_cast<jfg::ModernAction>(index),binding,policy);
    modernCapture=-1;
    if(result==jfg::ModernBindResult::conflict) {
      modernPendingAction=index;modernPendingBinding=binding;modernPanel="conflict";modernOverlay();return;
    }
    if(result==jfg::ModernBindResult::invalid)return;
    if(saveModern(candidate)){modernSelected=binding;modal("controllers");}
  }
  void modernBeginCapture(int actionIndex) {
    if(!modernOnline())return;
    if(modernPanel=="palette" || modernPanel=="scope-bindings")modernReturnFocus=modernSettings?"action-mc-scope-separate":"action-mc-bind-menu";
    learning=keyLearning=-1;modernCapture=actionIndex;modernCapturePlayer=player;
    modernCaptureDevice=devices[player];modernReleased=false;
    modernDeadline=Rml::GetSystemInterface()->GetElapsedTime()+12;
    modernPanel="capture";modernOverlay();
  }
  void modernCaptured(int binding) {
    if(modernCapture==-2) {
      modernCapture=-1;modernSelected=binding;modernPanel="picker";modernOverlay();
    } else if(modernCapture>=0)modernBind(modernCapture,binding);
  }
  bool modernMessage(UINT message,WPARAM key,LPARAM position) {
    if(current!="controllers")return false;
    if(message==WM_KEYDOWN && key==VK_TAB && (devicePicker || (!modernPanel.empty() && modernPanel!="capture"))) {
      modernFocusMove((GetKeyState(VK_SHIFT)&0x8000)?-1:1);return true;
    }
    if(modernPanel.empty())return false;
    if(message==WM_KILLFOCUS || (message==WM_ACTIVATEAPP && !key)) {
      modernCancel("Editing cancelled because the window lost focus. No pending binding was saved.");return false;
    }
    if((message==WM_KEYDOWN || message==WM_SYSKEYDOWN) && key==VK_ESCAPE) {
      modernCancel("Cancelled. No pending binding was changed.");return true;
    }
    if(modernPanel!="capture")return false;
    // A real Cancel click is routed to RmlUi; it must not bind Mouse left.
    if(message==WM_LBUTTONDOWN || message==WM_LBUTTONUP) {
      const Rml::Vector2f point{float(short(LOWORD(position))),float(short(HIWORD(position)))};
      auto* target=doc->GetContext()->GetElementAtPoint(point);
      while(target) {
        if(target->GetId()=="action-mc-cancel" || target->GetId()=="action-mc-dismiss")return false;
        target=target->GetParentNode();
      }
    }
    if(!pcKeyboard())return message==WM_KEYDOWN || message==WM_SYSKEYDOWN ||
      message==WM_KEYUP || message==WM_SYSKEYUP || message==WM_CHAR;
    int binding=-1;
    if(message==WM_KEYDOWN || message==WM_SYSKEYDOWN)binding=int(key);
    else if(message==WM_LBUTTONDOWN)binding=VK_LBUTTON;
    else if(message==WM_RBUTTONDOWN)binding=VK_RBUTTON;
    else if(message==WM_MBUTTONDOWN)binding=VK_MBUTTON;
    else if(message==WM_XBUTTONDOWN)binding=GET_XBUTTON_WPARAM(key)==XBUTTON1?VK_XBUTTON1:VK_XBUTTON2;
    else if(message==WM_MOUSEWHEEL) {
      const auto delta=GET_WHEEL_DELTA_WPARAM(key);if(!delta)return true;
      binding=delta>0?jfg::kPcWheelUp:jfg::kPcWheelDown;
    }
    if(binding>=0) {
      if(jfg::modern_arrow_key(binding)) {
        label("mc-capture-status","Use a letter, number or function key. Movement keys navigate menus.");
        return true;
      }
      if(modernReleased && binding!=VK_F11 && modernHasFocus())modernCaptured(binding);
      return true;
    }
    return message==WM_KEYUP || message==WM_SYSKEYUP || message==WM_CHAR ||
      message==WM_LBUTTONUP || message==WM_RBUTTONUP || message==WM_MBUTTONUP || message==WM_XBUTTONUP;
  }
  std::array<bool,27> modernPadActive() {
    XINPUT_STATE state{};std::array<bool,27> result{};
    if(devices[player]<0 || readController(DWORD(devices[player]),&state)!=ERROR_SUCCESS)return result;
    jfg::StandardControllerSample pad{};
    if(const auto* info=controllerCatalog.get(devices[player]))pad=info->sample;
    else {
      auto active=sample(state);for(int i=0;i<15;++i)pad.buttons[i]=active[i];
      pad.axes={state.Gamepad.sThumbLX,-int(state.Gamepad.sThumbLY),
        state.Gamepad.sThumbRX,-int(state.Gamepad.sThumbRY),
        state.Gamepad.bLeftTrigger*32767/255,state.Gamepad.bRightTrigger*32767/255};
    }
    for(int b=0;b<27;++b)result[b]=jfg::modern_pad_down(modern[player],pad,b);
    return result;
  }
  std::array<bool,27> modernNavPrevious{};
  bool modernNavReady=false;
  void modernFocusMove(int direction) {
    auto* root=el(!modernPanel.empty()?"modern-overlay":devicePicker?"device-picker":"dialog");if(!root)return;
    Rml::ElementList controls;root->QuerySelectorAll(controls,"button, input, select");
    controls.erase(std::remove_if(controls.begin(),controls.end(),[](Rml::Element* e) {
      return !e->IsVisible(true) || e->HasAttribute("disabled");
    }),controls.end());
    if(controls.empty())return;
    auto it=std::find(controls.begin(),controls.end(),doc->GetContext()->GetFocusElement());
    int index=it==controls.end()?(direction<0?0:-1):int(it-controls.begin());
    index=(index+direction+int(controls.size()))%int(controls.size());
    controls[index]->Focus(true);controls[index]->ScrollIntoView(false);
  }
  bool modernAdjustSelect(int direction) {
    auto* select=dynamic_cast<Rml::ElementFormControlSelect*>(doc->GetContext()->GetFocusElement());
    if(!select || select->HasAttribute("disabled"))return false;
    const int count=select->GetNumOptions();if(count<=0)return true;
    const int old=select->GetSelection();
    for(int step=1;step<=count;++step) {
      const int next=(old+direction*step+count*2)%count;
      if(auto* option=select->GetOption(next);option && !option->HasAttribute("disabled")) {
        select->SetSelection(next);return true;
      }
    }
    return true;
  }
  void modernPoll() {
    if(current!="controllers" || learning>=0 || keyLearning>=0){modernNavReady=false;return;}
    if(!modernHasFocus()) {
      modernNavReady=false;modernNavPrevious={};
      if(modernPanel=="capture")modernCancel("Capture cancelled because the window lost focus.");
      return;
    }
    if(modernPanel=="capture") {
      if(player!=modernCapturePlayer || devices[player]!=modernCaptureDevice || !modernOnline()) {
        modernCancel("Capture cancelled because the selected device changed or disconnected.");return;
      }
      if(Rml::GetSystemInterface()->GetElapsedTime()>modernDeadline) {
        modernCancel("Capture timed out. No binding was changed.");return;
      }
      const double remaining=std::max(0.0,modernDeadline-Rml::GetSystemInterface()->GetElapsedTime());
      label("mc-capture-count",std::to_string(int(std::ceil(remaining)))+"s");
      if(auto* progress=el("mc-capture-progress"))progress->SetProperty("width",std::to_string(remaining*100.0/12.0)+"%");
      const auto active=pcKeyboard()?std::array<bool,27>{}:modernPadActive();
      const bool neutral=pcKeyboard()?modernKeysNeutral():std::none_of(active.begin(),active.end(),[](bool b){return b;});
      if(!modernReleased && neutral) {
        modernReleased=true;
        if(el("mc-capture-status"))label("mc-capture-status",pcKeyboard()?
          "Press a key, mouse button, or scroll the wheel.":"Press a button, pull a trigger, or move a stick.");
      } else if(modernReleased && !pcKeyboard())
        for(int b=0;b<27;++b)if(active[b]){modernCaptured(b);break;}
      modernNavReady=false;modernNavPrevious=active;return;
    }
    if(pcKeyboard() || devices[player]<0 || !connected[devices[player]]){modernNavReady=false;return;}
    const auto active=modernPadActive();
    if(!modernNavReady) {
      modernNavReady=std::none_of(active.begin(),active.end(),[](bool b){return b;});
      modernNavPrevious=active;return;
    }
    const auto pressed=[&](int b){return active[b]&&!modernNavPrevious[b];};
    if(pressed(11))modernFocusMove(-1);
    else if(pressed(12))modernFocusMove(1);
    else if(pressed(13) || pressed(14)) {
      auto* focused=doc->GetContext()->GetFocusElement();
      if(focused && focused->GetTagName()=="input" && focused->GetAttribute<Rml::String>("type","")=="range") {
        const int old=std::stoi(focused->GetAttribute<Rml::String>("value","0"));
        const int minimum=std::stoi(focused->GetAttribute<Rml::String>("min","0"));
        const int maximum=std::stoi(focused->GetAttribute<Rml::String>("max","100"));
        const int step=std::stoi(focused->GetAttribute<Rml::String>("step","5"));
        Rml::Dictionary parameters;parameters["value"]=std::clamp(old+(pressed(14)?step:-step),minimum,maximum);
        focused->DispatchEvent("change",parameters);
      } else if(!modernAdjustSelect(pressed(14)?1:-1))modernFocusMove(pressed(14)?1:-1);
    } else if(pressed(0)) {
      auto* focused=doc->GetContext()->GetFocusElement();
      if(focused && focused->GetTagName()=="button" && !focused->HasAttribute("disabled"))focused->Click();
      else modernAdjustSelect(1);
    } else if(pressed(1)) {
      if(!modernPanel.empty())modernCancel("Cancelled.");
      else if(devicePicker){devicePicker=false;modal("controllers");}
      else close();
    }
    modernNavPrevious=active;
  }
  bool modernEvent(Rml::Event& event,Rml::Element* target,const std::string& a) {
    if(current!="controllers")return false;
    if(!modernPanel.empty() && a.rfind("mc-",0)!=0)return true;
    if(a.rfind("mc-",0)!=0) {
      if(event.GetType()=="click" && (a.rfind("player",0)==0 || a.rfind("device-",0)==0)) {
        modernSelected=-100;modernNotice.clear();modernNavReady=false;
      }
      return false;
    }
    if(!modernPanel.empty()) {
      auto* parent=target;bool inside=false;
      while(parent){if(parent->GetId()=="modern-overlay"){inside=true;break;}parent=parent->GetParentNode();}
      if(!inside)return true;
    }
    if(event.GetType()=="change") {
      if(a=="mc-glyph-select") {
        auto c=modern[player];c.glyph_style=static_cast<jfg::ModernGlyphStyle>(std::clamp(event.GetParameter<int>("value",0),0,2));
        if(saveModern(c))modernDeferredRefresh=true;return true;
      }
      if(a.rfind("mc-tune-",0)!=0 || !modernMode())return true;
      auto c=modern[player];auto& look=c.look;const bool keyboard=pcKeyboard();
      const std::string id=a.substr(8);int* value=nullptr;int minimum=10,maximum=300;
      if(id=="camera"){value=keyboard?&look.mouse_sensitivity:&look.camera_sensitivity;maximum=keyboard?500:300;}
      else if(id=="aim"){value=keyboard?&look.mouse_aim_sensitivity:&look.aim_sensitivity;maximum=keyboard?500:300;}
      else if(id=="scope")value=keyboard?&c.mouse_scope_sensitivity:&c.pad_scope_sensitivity;
      else if(id=="vertical")value=keyboard?&look.mouse_vertical_sensitivity:&look.stick_vertical_sensitivity;
      else if(!keyboard && id=="move-deadzone"){value=&c.movement_deadzone;minimum=0;maximum=30000;}
      else if(!keyboard && id=="look-deadzone"){value=&look.aim_deadzone;minimum=0;maximum=30000;}
      else if(!keyboard && id=="axis"){value=&c.axis_threshold;minimum=1000;maximum=32000;}
      else if(!keyboard && id=="trigger"){value=&c.trigger_threshold;minimum=1000;maximum=32000;}
      else if(!keyboard && id=="rumble" && devices[player]>=0 && devices[player]<4){value=&look.rumble_strength;minimum=0;maximum=100;}
      if(!value)return true;
      *value=std::clamp(event.GetParameter<int>("value",*value),minimum,maximum);
      const int savedValue=*value;
      if(saveModern(c)) {
        target->SetAttribute("value",std::to_string(savedValue));
        label(("mc-value-"+id).c_str(),std::to_string(maximum>500?(savedValue*100+16383)/32767:savedValue)+"%");
        label("dialog-note",modernNotice);
      } else {
        // Keep displayed sliders consistent with the configuration still active on disk.
        modal("controllers");
      }
      return true;
    }
    if(event.GetType()!="click")return true;
    if(modernPanel.empty())modernReturnFocus=target->GetId();
    if(a=="mc-cancel" || a=="mc-dismiss"){modernCancel("Cancelled. No pending changes were saved.");return true;}
    if(a=="mc-save-retry") {
      if(modernRetry) {const auto c=*modernRetry;if(saveModern(c))modal("controllers");}
      return true;
    }
    if(a=="mc-repair"){modernPanel="repair";modernOverlay();return true;}
    if(a=="mc-repair-confirm") {
      std::error_code ec;const auto original=profile/modernFile(player);
      const auto backup=profile/(modernFile(player)+".unreadable-"+std::to_string(GetTickCount64()));
      if(!fs::copy_file(original,backup,fs::copy_options::none,ec)) {
        modernNotice="Could not back up the unreadable profile. No settings were changed.";modernPanel.clear();modal("controllers");return true;
      }
      modernInvalid[player]=false;
      if(saveModern(jfg::ModernControlsConfig{}))modal("controllers");
      return true;
    }
    if(modernInvalid[player])return true;
    if(a=="mc-style-modern" || a=="mc-style-mapping") {
      if(player!=0 && a=="mc-style-modern")return true;
      auto c=modern[player];c.enabled=a=="mc-style-modern";
      if(saveModern(c)){learning=keyLearning=-1;modernSelected=-100;modal("controllers");}
      return true;
    }
    if(!modernMode()) {
      if(a.rfind("mc-select-",0)==0){modernSelected=std::stoi(a.substr(10));modal("controllers");}
      return true;
    }
    if(a=="mc-tab-buttons" || a=="mc-tab-settings" || a=="mc-tab-advanced"){modernSettings=a=="mc-tab-settings";modernAdvanced=a=="mc-tab-advanced";modal("controllers");return true;}
    if(a=="mc-view-list" || a=="mc-view-diagram"){modernDiagram=a=="mc-view-diagram";modal("controllers");return true;}
    if(a.rfind("mc-device-pick-",0)==0){
      if(assignDevice(std::stoi(a.substr(15))))if(auto* input=el("action-device-open"))input->Focus(true);
      return true;
    }
    if(a=="mc-mouse-settings"){modernSettings=true;modernAdvanced=false;modal("controllers");return true;}
    if(a=="mc-bind-menu" || a=="mc-scope-separate"){modernPanel=a=="mc-scope-separate"?"scope-bindings":"palette";modernOverlay();return true;}
    if(a=="mc-scope-cycle"){modernBeginCapture(7);return true;}
    if(a.rfind("mc-key-other-",0)==0 || a.rfind("mc-mouse-select-",0)==0) {
      modernSelected=std::stoi(a.substr(a.rfind("mc-key-other-",0)==0?13:16));modal("controllers");return true;
    }
    if(a.rfind("mc-free-",0)==0) {
      const int binding=std::stoi(a.substr(8));
      if(binding>=0) {
        auto c=modern[player];auto& binds=pcKeyboard()?c.keys:c.pad;
        for(std::size_t i=0;i<binds.size();++i)if(binds[i]==binding && (jfg::kModernActions[i].contexts&modernShownContext()))binds[i]=pcKeyboard()?0:-1;
        if(saveModern(c))modal("controllers");
      }
      return true;
    }
    if(a=="mc-overview"){modernSelected=-100;modal("controllers");return true;}
    if(a.rfind("mc-select-",0)==0){modernSelected=std::stoi(a.substr(10));modal("controllers");return true;}
    if(a.rfind("mc-input-",0)==0){modernSelected=std::stoi(a.substr(9));if(modernSelected==-12){modernSettings=true;modernAdvanced=false;modal("controllers");return true;}modernPanel="picker";modernOverlay();return true;}
    if(a.rfind("mc-palette-",0)==0){const int i=std::stoi(a.substr(11));if(i>=0 && i<int(jfg::kModernActionCount))modernBeginCapture(i);return true;}
    if(a=="mc-add-key"){modernBeginCapture(-2);return true;}
    if(a.rfind("mc-capture-",0)==0){const int i=std::stoi(a.substr(11));if(i>=0 && i<int(jfg::kModernActionCount))modernBeginCapture(i);return true;}
    if(a.rfind("mc-assign-",0)==0) {
      const int index=std::stoi(a.substr(10));
      const auto& bindings=pcKeyboard()?modern[player].keys:modern[player].pad;
      if(index>=0 && index<int(jfg::kModernActionCount))
        modernBind(index,bindings[index]==modernSelected?(pcKeyboard()?0:-1):modernSelected);
      return true;
    }
    if(a.rfind("mc-clear-",0)==0){modernBind(std::stoi(a.substr(9)),pcKeyboard()?0:-1);return true;}
    if(a=="mc-conflict-replace" || a=="mc-conflict-both") {
      modernBind(modernPendingAction,modernPendingBinding,a=="mc-conflict-replace"?jfg::ModernBindPolicy::replace_conflicts:jfg::ModernBindPolicy::keep_both);return true;
    }
    if(a=="mc-conflict-swap") {
      if(modernCanSwap()) {
        auto c=modern[player];auto& bindings=pcKeyboard()?c.keys:c.pad;
        const auto conflicts=jfg::modern_binding_conflicts(c,pcKeyboard(),static_cast<jfg::ModernAction>(modernPendingAction),modernPendingBinding);
        const int old=bindings[modernPendingAction];bindings[modernPendingAction]=modernPendingBinding;
        for(std::size_t i=0;i<conflicts.size();++i)if(conflicts[i])bindings[i]=old;
        if(saveModern(c))modal("controllers");
      }
      return true;
    }
    if(a=="mc-reset"){modernResetTuning=modernSettings || modernAdvanced;modernPanel="reset";modernOverlay();return true;}
    auto c=modern[player];const jfg::ModernControlsConfig defaults;
    if(a=="mc-reset-confirm") {
      c.aim_toggle=defaults.aim_toggle;
      if(pcKeyboard()) {
        c.mouse_original_acceleration=defaults.mouse_original_acceleration;
        if(!modernResetTuning)c.keys=defaults.keys;c.mouse_scope_sensitivity=100;c.look.mouse_sensitivity=defaults.look.mouse_sensitivity;
        c.look.mouse_aim_sensitivity=defaults.look.mouse_aim_sensitivity;c.look.mouse_vertical_sensitivity=100;
        c.look.mouse_invert_x=defaults.look.mouse_invert_x;c.look.mouse_invert_y=defaults.look.mouse_invert_y;
      } else {
        if(!modernResetTuning){c.pad=defaults.pad;c.movement_stick=defaults.movement_stick;}c.pad_scope_sensitivity=100;c.movement_deadzone=defaults.movement_deadzone;
        c.axis_threshold=defaults.axis_threshold;c.trigger_threshold=defaults.trigger_threshold;
        c.look.camera_sensitivity=c.look.aim_sensitivity=c.look.stick_vertical_sensitivity=100;
        c.look.aim_deadzone=defaults.look.aim_deadzone;c.look.aim_curve=c.look.aim_invert_x=c.look.aim_invert_y=0;c.look.rumble_strength=0;
      }
    } else if(a=="mc-stick-move" || a=="mc-stick-look") {
      const int selected=modernSelected==-10?0:1;
      c.movement_stick=a=="mc-stick-move"?selected:1-selected;
    } else if(a=="mc-aim-hold" || a=="mc-aim-toggle")c.aim_toggle=a=="mc-aim-toggle";
    else if(a=="mc-invert-x"){auto& value=pcKeyboard()?c.look.mouse_invert_x:c.look.aim_invert_x;value=!value;}
    else if(a=="mc-invert-y"){auto& value=pcKeyboard()?c.look.mouse_invert_y:c.look.aim_invert_y;value=!value;}
    else if(a=="mc-original-acceleration" && pcKeyboard())c.mouse_original_acceleration=!c.mouse_original_acceleration;
    else if(a.rfind("mc-curve-",0)==0)c.look.aim_curve=std::clamp(std::stoi(a.substr(9)),0,2);
    // Select clicks open its popup; only its change event changes glyphs.
    else return true;
    if(saveModern(c))modal("controllers");
    return true;
  }
