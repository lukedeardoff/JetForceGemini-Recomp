
// Original N64 mappings share the prototype's presentation, but keep their own bindings.
  std::string mappingRange(const std::string& key,const std::string& title,int value) {
    return "<div class=\"mc-setting\"><div class=\"mc-setting-title\"><strong>"+title+"</strong><span id=\"value-"+key+"\">"+
      std::to_string((value*100+16383)/32767)+"%</span></div><input class=\"mc-range\" type=\"range\" min=\""+
      std::string(key=="deadzone"?"0":"1000")+"\" max=\""+std::string(key=="deadzone"?"30000":"32000")+
      "\" step=\"100\" value=\""+std::to_string(value)+"\" data-action=\"tune-"+key+"\"/></div>";
  }
  std::string mappingHtml() {
    const auto& m=mappings[player];const bool keyboard=pcKeyboard();
    const auto& c=modern[player];
    std::string html="<div id=\"mapping-content\" class=\"mc-content\"><div class=\"mc-toolbar\"><div class=\"mc-tabs\">"+
      mcControl("mapping-tab-buttons",keyboard?"Keys &amp; buttons":"Buttons","mc-tab",!controllerSettings)+
      mcControl("mapping-tab-settings","Sensitivity &amp; behavior","mc-tab",controllerSettings&&!mappingAdvanced)+
      (keyboard?"":mcControl("mapping-tab-advanced","Advanced settings","mc-tab",controllerSettings&&mappingAdvanced))+
      "</div><div class=\"mc-toolbar-options\">";
    if(!keyboard) {
      html+="<span class=\"mc-symbols-label\"><span>Button symbols</span></span><select id=\"mc-glyph-select\" data-action=\"mc-glyph-select\">";
      for(int i=0;i<3;++i)html+="<option value=\""+std::to_string(i)+"\""+(int(c.glyph_style)==i?" selected=\"selected\"":"")+">"+
        (i==0?modernAutomaticGlyphLabel():std::string(i==1?"Xbox":"PlayStation"))+"</option>";
      html+="</select>";
      if(!controllerSettings)html+=mcControl("mapping-diagram","<img class=\"mc-pad-icon\" src=\"controls-art/toolbar-pad.tga\"/><span>"+
        std::string(mappingDiagram?"Hide":"Show")+" controller diagram</span>","mc-view-toggle",mappingDiagram);
    }
    html+="</div></div><div class=\"mc-workspace mapping-workspace\">";
    if(m.device==-2)return html+"<div class=\"mc-offline-note\">Choose mouse and keyboard or a controller to configure bindings.</div></div></div>";
    if(controllerSettings) {
      html+="<div class=\"mc-settings-scroll\"><div class=\"mc-settings-grid\"><section id=\"controller-settings\" class=\"mc-settings-card\"><div class=\"mc-section-title\">"+
        std::string(keyboard?"KEYBOARD MOVEMENT":mappingAdvanced?"BINDING THRESHOLDS":"MOVEMENT")+"</div>";
      if(keyboard)html+="<p>Keys map to the original game's controls. Movement speed uses the Full movement binding.</p><p class=\"mc-hint\">Choose Modern Controls for mouse look, camera and aiming sensitivity.</p>";
      else if(mappingAdvanced)html+=mappingRange("threshold","Stick button threshold",m.threshold)+mappingRange("trigger","Trigger threshold",m.trigger);
      else html+=mappingRange("deadzone","Movement deadzone",m.deadzone)+
        modernSwitch("invertx","Invert horizontal","Push right to move left",m.invert_x)+
        modernSwitch("inverty","Invert vertical","Push up to move backward",m.invert_y);
      return html+"</section></div></div></div></div>";
    }
    html+="<div class=\"mapping-presets\">";
    if(!keyboard)html+="<span class=\"mc-hint\">Movement</span><div class=\"mc-segment\">"+
      mcControl("mapping-stick-left","Left stick","",!m.stick)+mcControl("mapping-stick-right","Right stick","",m.stick)+"</div><span class=\"mapping-divider\"/>";
    html+=mcControl(keyboard?"pc-normal":"pc-pad-normal","Normal preset")+mcControl(keyboard?"pc-expert":"pc-pad-expert","Expert preset")+
      "<span class=\"mc-hint mapping-help\">Normal jumps with A. Expert jumps with C up. Sniper zoom: hold aim, then use the jump or crouch control.</span></div>";
    if(!keyboard && mappingDiagram)html+="<div class=\"mapping-art\">"+modernPadArt()+"</div>";
    static constexpr const char* names[]{"A / confirm","B","Z / fire","Start","Up","Down","Left","Right","L","R / aim","C up","C down","C left","C right","Move forward","Move back","Move left","Move right","Full movement","Alternate A"};
    const std::array<std::vector<int>,4> groups{{{0,1,2,9,8,3},{10,11,12,13},{4,5,6,7},{14,15,16,17,18,19}}};
    constexpr const char* titles[]{"BUTTONS","C BUTTONS","D-PAD","MOVEMENT"};
    html+="<div class=\"mapping-cards\">";
    for(int col=0;col<3;++col) {
      html+="<div class=\"mapping-column\">";
      for(int g=col;g<4;g+=3) {
        if(g==3 && !keyboard)continue;
        html+="<section class=\"mc-binding-group mapping-card\"><div class=\"mc-section-title\">"+std::string(titles[g])+"</div>";
        for(int i:groups[g]) {
          const auto id=std::to_string(i);const int binding=keyboard?pcInput[player].keys[i]:m.bindings[i];
          const bool free=keyboard?binding==0:binding<0;
          const bool selected=(keyboard?keyLearning:learning)==i || (!keyboard && mappingDiagram && modernSelected==binding && !free);
          html+="<div id=\"binding-"+id+"\" class=\"mc-binding-row mapping-row"+(selected?" selected":"")+"\"><span class=\"mapping-label\">"+names[i]+"</span>"+
            mcControl((keyboard?"pc-bind":"learn")+id,(free?"<span class=\"mapping-unbound\">&#8211;</span>":modernChip(binding,selected,24))+
            "<span class=\"mapping-binding-name\">"+escape(free?"Unbound":keyboard?pcKeyName(binding):modernInputDescription(binding))+"</span>",
            "mc-binding-value mapping-value"+std::string(free?" free":""),false,!keyboard && devices[player]<0)+
            mcControl((keyboard?"pc-unbind":"clear")+id,"&#215;","mc-row-clear"+std::string(free?" invisible":""))+"</div>";
        }
        html+="</section>";
      }
      html+="</div>";
    }
    return html+"</div></div></div>";
  }
