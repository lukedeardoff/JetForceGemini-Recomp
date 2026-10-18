// Injected devices and an offscreen, nonactivating HWND. Never sends desktop input.
// Usage: jfg_modern_ui_tests.exe <new-artifact-directory>.
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include "../native_ui.cpp"
#include <stdexcept>
static fs::path home;
static int checks=0;
static void demand(bool pass,const char* message) {
  ++checks;if(!pass)throw std::runtime_error(message);
}
static void settle() {for(int i=0;i<3;++i)FrontendUiFrame({});}
static Rml::Element* element(const std::string& id) {
  auto* e=surface->ui.el(id.c_str());demand(e!=nullptr,id.c_str());return e;
}
static void click(const std::string& id) {
  std::ofstream(home/"events.txt",std::ios::app)<<id<<"\n";
  auto* e=element(id);demand(!e->HasAttribute("disabled"),("enabled "+id).c_str());e->Click();settle();
}
static void editInput(int binding) {
  const auto value=std::to_string(binding);
  if(surface->ui.modernDiagram) {
    click("action-mc-select-"+value);click("action-mc-input-"+value);
  } else click("mc-row-"+value);
}
static void change(const std::string& id,int value) {
  Rml::Dictionary event;event["value"]=value;element(id)->DispatchEvent("change",event);settle();
}
static void captureAction(int action) {
  surface->ui.modernReturnFocus="action-mc-tab-buttons";
  surface->ui.modernBeginCapture(action);settle();
}

static void pointerClick(const std::string& id) {
  auto* control=element(id);
  auto point=control->GetAbsoluteOffset(Rml::BoxArea::Border)+Rml::Vector2f(control->GetClientWidth()/2,control->GetClientHeight()/2);
  auto projected=point;demand(control->Project(projected),"project native hit target");point+=point-projected;
  auto* hit=surface->context->GetElementAtPoint(point);
  while(hit && hit!=control)hit=hit->GetParentNode();
  demand(hit==control,("native hit target "+id).c_str());
  surface->context->ProcessMouseMove(int(point.x),int(point.y),0);
  surface->context->ProcessMouseButtonDown(0,0);surface->context->ProcessMouseButtonUp(0,0);settle();
}

static void capture(const char* name) {
  auto& s=*surface;settle();ComPtr<ID3D11Resource> resource;s.target->GetResource(&resource);
  ComPtr<ID3D11Texture2D> target;check(resource.As(&target));D3D11_TEXTURE2D_DESC desc{};target->GetDesc(&desc);
  desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
  ComPtr<ID3D11Texture2D> staging;check(s.device->CreateTexture2D(&desc,nullptr,&staging));s.gpu->CopyResource(staging.Get(),target.Get());
  D3D11_MAPPED_SUBRESOURCE mapped{};check(s.gpu->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped));
  std::ofstream file(home/name,std::ios::binary);unsigned char header[18]{};
  header[2]=2;header[12]=static_cast<unsigned char>(desc.Width);header[13]=static_cast<unsigned char>(desc.Width>>8);
  header[14]=static_cast<unsigned char>(desc.Height);header[15]=static_cast<unsigned char>(desc.Height>>8);header[16]=32;header[17]=40;
  file.write(reinterpret_cast<char*>(header),18);
  for(unsigned y=0;y<desc.Height;++y)file.write(static_cast<char*>(mapped.pData)+y*mapped.RowPitch,desc.Width*4);
  s.gpu->Unmap(staging.Get(),0);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int) {
  HWND window=nullptr;bool ready=false;
  try {
    int argc=0;auto** argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    if(argc!=2){LocalFree(argv);return 2;}home=argv[1];LocalFree(argv);
    if(fs::exists(home))return 3;fs::create_directories(home);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    WNDCLASSW cls{};cls.lpfnWndProc=DefWindowProcW;cls.hInstance=instance;cls.lpszClassName=L"JfgModernControlsTests";RegisterClassW(&cls);
    window=CreateWindowExW(WS_EX_TOOLWINDOW,cls.lpszClassName,L"Offscreen Modern Controls checks",
      WS_OVERLAPPEDWINDOW,-30000,-30000,1280,1000,nullptr,nullptr,instance,nullptr);
    const auto profile=home/"profile";fs::create_directories(profile);
    std::ofstream(profile/"experimental-controls.ini")<<jfg::serialize_pc_experiments(false);
    demand(FrontendUiInit(window,profile,[](const std::string&){},"controllers"),"native UI initializes");ready=true;
    auto& ui=surface->ui;std::array<bool,4> hardware{true,true,false,false};std::array<XINPUT_STATE,4> inputs{};
    std::vector<jfg::ControllerDeviceInfo> extra;ui.sdlDiscovery=[&]{return extra;};
    ui.controllerState=[&](DWORD d,XINPUT_STATE* state)->DWORD {
      if(d>=inputs.size())return ERROR_DEVICE_NOT_CONNECTED;*state=inputs[d];return hardware[d]?ERROR_SUCCESS:ERROR_DEVICE_NOT_CONNECTED;
    };
    bool focused=true,neutral=true;ui.modernHasFocus=[&]{return focused;};ui.modernKeysNeutral=[&]{return neutral;};
    for(int p=0;p<4;++p){ui.player=p;ui.mappings[p].device=-2;ui.saveController();}
    ui.player=0;ui.loadControllers();ui.discoverDevices();ui.assignDevice(0);
    RECT frame{0,0,1280,920};AdjustWindowRectExForDpi(&frame,WS_OVERLAPPEDWINDOW,FALSE,WS_EX_TOOLWINDOW,GetDpiForWindow(window));
    SetWindowPos(window,nullptr,-30000,-30000,frame.right-frame.left,frame.bottom-frame.top,SWP_NOZORDER|SWP_NOACTIVATE);
    ShowWindow(window,SW_SHOWNOACTIVATE);FrontendUiResize();surface->context->SetDensityIndependentPixelRatio(1.f);settle();
    ui.mappings[0].bindings[1]=3;ui.saveController();const auto original=ui.read("controller.ini");const auto legacySwitch=ui.read("experimental-controls.ini");
    demand(!ui.modernMode(),"original mapping is the default");click("action-mc-style-modern");
    demand(ui.modernMode(),"Modern Controls is the single opt-in");
    demand(ui.read("experimental-controls.ini")==legacySwitch,"mode selection leaves obsolete switch file untouched");
    demand(ui.read("controller.ini")==original,"style selection preserves original bindings");
    demand(ui.el("modern-content")&&!ui.el("mapping-content"),"modern content replaces mapping content only");
    demand(!ui.el("action-mc-experimental")&&!ui.el("action-pc-experimental"),"obsolete experimental switch removed");
    demand(ui.modern[0].pad[2]==0,"Xbox A defaults to Jump");
    demand(element("mc-row-0")->GetInnerRML().find("Confirm")!=std::string::npos && element("mc-row-2")->GetInnerRML().find("Save game")!=std::string::npos,"controller shared actions are visible in existing groups");
    demand(!ui.el("mc-context-select")&&!ui.el("action-mc-bind-menu"),"redundant context and rebinding controls removed");
    editInput(8);demand(ui.el("action-mc-assign-8")&&ui.el("action-mc-assign-9"),"row picker retains separate zoom actions");click("action-mc-cancel");
    click("action-mc-free-0");
    demand(ui.modern[0].pad[2]==-1&&ui.modern[0].pad[11]==-1,"clearing a physical input clears all its visible actions");
    click("mc-row-0");click("action-mc-assign-2");demand(ui.modern[0].pad[2]==0,"free button can select its action again");
    editInput(0);click("action-mc-assign-11");demand(ui.modern[0].pad[2]==0&&ui.modern[0].pad[11]==0,"Confirm can share the restored Jump input");
    const auto contextProfile=ui.read("modern-controls.ini");
    pointerClick("mc-glyph-select");pointerClick("mc-glyph-select");
    MSG pendingClose{};demand(!PeekMessageW(&pendingClose,window,WM_CLOSE,WM_CLOSE,PM_NOREMOVE),"repeated select clicks do not close controller window");
    demand(ui.read("modern-controls.ini")==contextProfile&&ui.modernMode(),"presentation changes preserve selected mode");capture("xbox-list.tga");
    demand(element("mc-column-0")->GetClientHeight()>400,"binding columns have a real viewport");
    demand(element("mc-row-0")->GetClientWidth()>100&&element("mc-row-0")->GetClientHeight()>25,"action rows remain usable");
    demand(element("action-mc-tab-buttons")->GetClientWidth()>35,"tabs have visible text width");
    click("action-mc-view-diagram");
    demand(element("dialog-body")->GetScrollTop()==0,"diagram keeps compact header in view");
    capture("xbox-diagram.tga");
    element("mc-diagram-list")->SetScrollTop(100);settle();
    click("action-mc-select-0");demand(ui.modernSelected==0,"diagram selects physical input");
    demand(element("mc-diagram-list")->GetScrollTop()>90,"diagram selection preserves binding-list scroll");
    click("action-mc-select-1");demand(ui.modernSelected==1,"diagram and binding list share selection");
    editInput(0);demand(ui.modernPanel=="picker","diagram opens picker");capture("action-picker.tga");
    demand(ui.modernMessage(WM_KEYDOWN,VK_TAB,0),"Tab is confined to open action picker");
    auto* focusedPicker=surface->context->GetFocusElement();
    bool inPicker=false;for(auto* node=focusedPicker;node;node=node->GetParentNode())if(node->GetId()=="modern-overlay")inPicker=true;
    demand(inPicker,"picker Tab cannot focus the obscured form");
    demand(ui.read("modern-controls.ini")==contextProfile,"opening picker does not save a binding");
    click("action-mc-assign-4");demand(ui.modernPanel=="conflict","overlapping action requires resolution");demand(element("mc-overlay-body")->GetClientHeight()>130,"conflict explanation has room");
    capture("binding-conflict.tga");
    demand(ui.modernCanSwap(),"single conflict can swap");click("action-mc-conflict-swap");
    demand(ui.modern[0].pad[2]==2&&ui.modern[0].pad[4]==0,"swap exchanges gameplay bindings");
    demand(ui.modern[0].pad[11]==0,"swap preserves menu Confirm");
    editInput(0);click("action-mc-assign-2");click("action-mc-conflict-both");
    demand(ui.modern[0].pad[2]==0&&ui.modern[0].pad[4]==0,"keep both shares binding");
    editInput(0);click("action-mc-assign-1");click("action-mc-conflict-replace");
    demand(ui.modern[0].pad[1]==0&&ui.modern[0].pad[2]==-1&&ui.modern[0].pad[4]==-1,"replace clears overlapping actions");
    demand(ui.modern[0].pad[11]==0,"replace preserves menu reuse");
    editInput(-11);click("action-mc-stick-move");demand(ui.modern[0].movement_stick==1,"stick role swaps");
    click("action-mc-tab-settings");
    // GitHub #13: the GUI toggle must affect camera look as well as aiming,
    // survive reload, and leave the independent mouse preference intact.
    jfg::StandardControllerSample inversionPad{};
    inversionPad.axes[ui.modern[0].movement_stick==0?3:1]=-32767;
    const auto inversionBefore=ui.modern[0].look;
    const auto cameraBefore=jfg::pc_stick_look(inversionBefore,inversionPad,ui.modern[0].movement_stick,false);
    const auto aimBefore=jfg::pc_stick_look(inversionBefore,inversionPad,ui.modern[0].movement_stick,true);
    demand(cameraBefore.y!=0&&aimBefore.y!=0,"controller vertical fixture has actual look input");
    click("action-mc-invert-y");ui.loadModern();
    const auto& inversionAfter=ui.modern[0].look;
    demand(inversionAfter.aim_invert_y!=inversionBefore.aim_invert_y,"controller vertical toggle persists after reload");
    demand(inversionAfter.mouse_invert_y==inversionBefore.mouse_invert_y,"controller inversion preserves mouse preference");
    demand(jfg::pc_stick_look(inversionAfter,inversionPad,ui.modern[0].movement_stick,false).y==-cameraBefore.y,"controller inversion reverses camera vertical look");
    demand(jfg::pc_stick_look(inversionAfter,inversionPad,ui.modern[0].movement_stick,true).y==-aimBefore.y,"controller inversion reverses aimed vertical look");
    click("action-mc-invert-y");ui.loadModern();
    demand(ui.modern[0].look.aim_invert_y==inversionBefore.aim_invert_y,"controller inversion toggles back");
    change("mc-range-scope",65);
    demand(ui.modern[0].pad_scope_sensitivity==65&&ui.modern[0].mouse_scope_sensitivity==100,"controller scope independent");
    change("mc-range-scope",999);demand(ui.modern[0].pad_scope_sensitivity==300,"scope clamp");
    change("mc-range-scope",65);
    click("action-mc-scope-separate");demand(ui.el("action-mc-palette-8")&&ui.el("action-mc-palette-9")&&!ui.el("action-mc-palette-2"),"scope binding menu contains only scope actions");
    click("action-mc-palette-8");demand(ui.modernCapture==8,"scope menu captures an increase binding");click("action-mc-cancel");
    change("mc-range-camera",170);click("action-mc-curve-2");demand(ui.modern[0].look.aim_curve==2,"cubic saved");
    click("action-mc-aim-toggle");demand(ui.modern[0].aim_toggle,"toggle aim saved");
    change("mc-glyph-select",2);demand(ui.modernInputName(0)=="Cross"&&ui.modernInputName(25)=="R2","PlayStation names");capture("controller-settings.tga");
    change("mc-glyph-select",0);click("action-mc-tab-buttons");
    inputs[0].Gamepad.wButtons=XINPUT_GAMEPAD_A;captureAction(2);demand(!ui.modernReleased,"capture waits release");capture("input-capture.tga");
    inputs[0]={};settle();demand(ui.modernReleased,"neutral arms capture");
    inputs[0].Gamepad.wButtons=XINPUT_GAMEPAD_Y;settle();inputs[0]={};
    demand(ui.modern[0].pad[2]==3&&ui.modernPanel.empty(),"gamepad captured and saved");
    captureAction(2);hardware[0]=false;settle();demand(ui.modernPanel.empty()&&ui.modernCapture==-1,"unplug cancels");
    hardware[0]=true;settle();captureAction(2);FrontendUiMessage(WM_KILLFOCUS,0,0);settle();
    demand(ui.modernCapture==-1&&ui.modernPanel.empty(),"focus loss cancels");
    captureAction(2);ui.modernDeadline=-1;settle();demand(ui.modernPanel.empty(),"timeout cancels");
    ui.assignDevice(-3);settle();demand(ui.pcKeyboard(),"keyboard assignment");
    demand(ui.modernSelected==-100,"device change clears prior diagram selection");
    pointerClick("action-mc-tab-advanced");
    demand(ui.el("mc-movement-card")&&!ui.el("mc-range-axis")&&!ui.el("mc-range-vertical"),"keyboard advanced shows movement without controller-only settings");
    demand(!ui.modern[0].mouse_original_acceleration,"keyboard original acceleration starts off");
    pointerClick("action-mc-original-acceleration");
    ui.loadModern();demand(ui.modern[0].mouse_original_acceleration,"actual RmlUi toggle saves acceleration preference");capture("keyboard-advanced.tga");
    ui.assignDevice(0);settle();click("action-mc-tab-advanced");
    demand(!ui.el("action-mc-original-acceleration")&&ui.el("mc-range-axis"),"controller advanced hides keyboard acceleration");
    demand(ui.modern[0].mouse_original_acceleration,"device switch preserves keyboard acceleration");
    ui.assignDevice(-3);settle();click("action-mc-tab-settings");change("mc-range-scope",45);
    demand(ui.modern[0].mouse_scope_sensitivity==45&&ui.modern[0].pad_scope_sensitivity==65,"mouse scope independent");
    demand(!ui.el("mc-range-rumble")&&!ui.el("mc-range-look-deadzone"),"keyboard hides pad tuning");capture("keyboard-settings.tga");
    click("action-mc-tab-buttons");capture("keyboard-diagram.tga");
    captureAction(1);neutral=false;ui.modernReleased=false;
    FrontendUiMessage(WM_XBUTTONDOWN,static_cast<WPARAM>(XBUTTON1)<<16,0);demand(ui.modernCapture==1,"held input cannot bind");
    neutral=true;settle();FrontendUiMessage(WM_XBUTTONDOWN,static_cast<WPARAM>(XBUTTON1)<<16,0);settle();
    demand(ui.modern[0].keys[1]==5&&ui.modernCapture==-1,"side mouse button saves");
    captureAction(2);settle();
    {
      auto* cancel=element("action-mc-cancel");
      auto point=cancel->GetAbsoluteOffset(Rml::BoxArea::Border)+Rml::Vector2f(cancel->GetClientWidth()/2,cancel->GetClientHeight()/2);
      // The dialog uses a CSS translation. Layout offsets do not include it.
      auto projected=point;demand(cancel->Project(projected),"project cancel position");
      point+=point-projected;
      auto* hit=surface->context->GetElementAtPoint(point);
      while(hit && hit!=cancel)hit=hit->GetParentNode();
      demand(hit==cancel,"capture fixture targets the rendered Cancel button");
      const auto packed=MAKELPARAM(int(point.x),int(point.y));
      demand(!ui.modernMessage(WM_LBUTTONDOWN,MK_LBUTTON,packed),"Cancel down bypasses binding capture");
      // Use RmlUi directly: Win32 mouse routing would call global SetCapture.
      surface->context->ProcessMouseMove(int(point.x),int(point.y),0);
      surface->context->ProcessMouseButtonDown(0,0);
      demand(!ui.modernMessage(WM_LBUTTONUP,0,packed),"Cancel up bypasses binding capture");
      surface->context->ProcessMouseButtonUp(0,0);settle();
      demand(ui.modernPanel.empty()&&ui.modern[0].keys[2]==VK_SPACE,"Cancel click cannot bind Mouse left");
      demand(surface->context->GetFocusElement()==ui.el("action-mc-tab-buttons"),"cancel restores originating control focus");
    }
    captureAction(2);settle();FrontendUiMessage(WM_KEYDOWN,VK_F11,0);demand(ui.modernCapture==2,"F11 reserved");
    FrontendUiMessage(WM_KEYDOWN,VK_ESCAPE,0);settle();demand(ui.modernPanel.empty(),"Escape cancels");
    captureAction(7);settle();
    FrontendUiMessage(WM_MOUSEWHEEL,0,0);demand(ui.modernCapture==7,"zero wheel message does not bind");
    FrontendUiMessage(WM_MOUSEWHEEL,static_cast<WPARAM>(WHEEL_DELTA)<<16,0);settle();
    demand(ui.modernPanel=="conflict","wheel capture checks conflicts");click("action-mc-conflict-replace");
    demand(ui.modern[0].keys[7]==256&&ui.modern[0].keys[6]==0,"wheel scope replaces weapon");
    click("action-mc-view-list");click("action-mc-add-key");settle();FrontendUiMessage(WM_KEYDOWN,'T',0);settle();
    demand(ui.modernPanel=="picker"&&ui.modernSelected=='T',"new key opens picker");
    click("action-mc-assign-4");demand(ui.modern[0].keys[4]=='T',"new key selects action");
    click("action-mc-tab-settings");
    const auto beforeError=ui.read("modern-controls.ini");const int beforeSensitivity=ui.modern[0].mouse_scope_sensitivity;
    HANDLE locked=CreateFileW((profile/"modern-controls.ini").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    demand(locked!=INVALID_HANDLE_VALUE,"profile locked for failure fixture");change("mc-range-scope",85);
    demand(ui.modernPanel=="save-error"&&ui.modern[0].mouse_scope_sensitivity==beforeSensitivity,"save failure retains model");
    demand(ui.read("modern-controls.ini")==beforeError,"save failure retains file");capture("save-error.tga");CloseHandle(locked);
    click("action-mc-save-retry");demand(ui.modern[0].mouse_scope_sensitivity==85,"retry applies pending change");
    ui.loadModern();demand(ui.modern[0].mouse_scope_sensitivity==85&&ui.modern[0].keys[4]=='T',"reload retains saved state");
    if(ui.modern[0].look.mouse_invert_y)click("action-mc-invert-y");
    demand(!ui.modern[0].look.mouse_invert_y,"explicit noninverted mouse choice accepted");
    const auto padInvertBefore=ui.modern[0].look.aim_invert_y;
    const auto padBefore=ui.modern[0].pad;click("action-mc-reset");click("action-mc-cancel");
    demand(ui.modern[0].keys[4]=='T'&&!ui.modern[0].look.mouse_invert_y,"reset cancellation retains bindings and inversion");
    click("action-mc-reset");click("action-mc-reset-confirm");
    demand(ui.modern[0].keys[4]=='T'&&ui.modern[0].mouse_scope_sensitivity==100&&!ui.modern[0].aim_toggle,"settings reset preserves bindings and resets tuning");
    demand(ui.modern[0].look.mouse_invert_y==1&&ui.modern[0].look.aim_invert_y==padInvertBefore,"mouse reset restores inverted vertical and retains controller choice");
    ui.loadModern();demand(ui.modern[0].look.mouse_invert_y==1,"reset inversion is saved");
    demand(!ui.modern[0].mouse_original_acceleration,"keyboard tuning reset saves acceleration off");
    click("action-mc-tab-buttons");click("action-mc-reset");click("action-mc-reset-confirm");
    demand(ui.modern[0].keys[4]=='E',"button tab reset restores bindings");
    demand(ui.modern[0].pad==padBefore&&ui.modern[0].pad_scope_sensitivity==65,"keyboard reset preserves controller");
    demand(ui.modernMode(),"editing preserves modern mode");
    click("action-mc-style-mapping");demand(ui.el("mapping-content")&&ui.el("action-pc-bind2"),"legacy keyboard mapping retained");
    demand(!element("action-pc-bind2")->HasAttribute("disabled"),"legacy rebind works experiments off");
    click("action-mc-style-modern");demand(ui.modern[0].pad==padBefore,"style switch retains modern layout");
    jfg::ControllerDeviceInfo ps;ps.key="sdl:"+std::string(64,'a');ps.name="DualSense";ps.connected=true;ps.family=jfg::ControllerFamily::dualsense;
    extra.push_back(ps);ui.discoverDevices();ui.assignDevice(4);settle();
    demand(ui.modernInputName(0)=="Cross","DualSense detected");click("action-mc-tab-buttons");click("action-mc-view-diagram");capture("dualsense-diagram.tga");
    click("action-mc-tab-settings");demand(!ui.el("mc-range-rumble"),"unsupported SDL rumble hidden");
    extra[0].family=jfg::ControllerFamily::generic;ui.discoverDevices();ui.modal("controllers");settle();
    demand(ui.modernInputName(0)=="South button","unknown controller generic labels");click("action-mc-tab-buttons");capture("generic-diagram.tga");
    click("action-player1");demand(element("action-mc-style-modern")->HasAttribute("disabled"),"Player2 modern unavailable");click("action-player0");
    ui.assignDevice(0);settle();ui.modernFocusMove(1);settle();auto* first=surface->context->GetFocusElement();
    inputs[0]={};settle();inputs[0].Gamepad.wButtons=XINPUT_GAMEPAD_DPAD_DOWN;settle();
    demand(surface->context->GetFocusElement()!=first,"D-pad moves focus");inputs[0]={};settle();
    editInput(0);inputs[0].Gamepad.wButtons=XINPUT_GAMEPAD_B;settle();inputs[0]={};demand(ui.modernPanel.empty(),"B cancels picker");
    ui.mappings[1].device=1;ui.assignments.mappings[1]=ui.mappings[1];ui.discoverDevices();
    click("action-device-open");
    demand(!ui.el("action-mc-device-pick-1")&&!ui.el("action-mc-device-pick-2"),"picker hides controllers assigned elsewhere and offline slots");
    click("action-device-cancel");ui.mappings[1].device=-2;ui.assignments.mappings[1]=ui.mappings[1];ui.discoverDevices();
    click("action-device-open");element("action-mc-device-pick-0")->Focus(true);settle();
    auto* deviceFocus=surface->context->GetFocusElement();
    inputs[0].Gamepad.wButtons=XINPUT_GAMEPAD_DPAD_DOWN;settle();inputs[0]={};settle();
    demand(surface->context->GetFocusElement()!=deviceFocus,"controller navigates prototype device choices");
    capture("device-picker.tga");
    click("action-device-cancel");demand(ui.devices[0]==0,"cancelled pad device navigation preserves assignment");
    click("action-device-open");click("action-mc-device-pick-1");demand(ui.devices[0]==1,"prototype device choice applies real assignment");
    click("action-device-open");click("action-mc-device-pick-0");demand(ui.devices[0]==0,"prototype device choice can restore controller");
    click("action-mc-tab-settings");element("mc-range-scope")->Focus(true);settle();
    const int beforePadScope=ui.modern[0].pad_scope_sensitivity;
    inputs[0].Gamepad.wButtons=XINPUT_GAMEPAD_DPAD_RIGHT;settle();inputs[0]={};settle();
    demand(ui.modern[0].pad_scope_sensitivity==beforePadScope+5,"controller adjusts sensitivity slider");
    SetWindowPos(window,nullptr,-30000,-30000,1024,720,SWP_NOZORDER|SWP_NOACTIVATE);FrontendUiResize();settle();
    ui.el("dialog-body")->SetScrollTop(0);capture("minimum-window.tga");demand(element("dialog-body")->GetClientHeight()>200,"minimum usable body");
    click("action-mc-tab-settings");click("action-mc-tab-advanced");ui.el("mc-settings-scroll")->SetScrollTop(ui.el("mc-settings-scroll")->GetScrollHeight());settle();
    demand(element("mc-range-trigger")->IsVisible(true),"last setting reachable");
    surface->context->SetDensityIndependentPixelRatio(1.5f);settle();ui.el("dialog-body")->SetScrollTop(0);capture("scaled-window.tga");
    std::ofstream(profile/"modern-controls.ini",std::ios::binary)<<"bad-profile";
    ui.loadModern();ui.modal("controllers");settle();demand(ui.modernInvalid[0]&&ui.read("modern-controls.ini")=="bad-profile","invalid profile preserved");
    click("action-mc-repair");click("action-mc-repair-confirm");demand(!ui.modernInvalid[0]&&!ui.modernMode(),"repair defaults opt-out");
    bool backup=false;for(const auto& entry:fs::directory_iterator(profile))if(entry.path().filename().string().starts_with("modern-controls.ini.unreadable-"))backup=true;
    demand(backup,"unreadable profile backed up");

    // Review views use freshly restored fixture defaults, never the owner's profile.
    frame={0,0,1280,920};AdjustWindowRectExForDpi(&frame,WS_OVERLAPPEDWINDOW,FALSE,WS_EX_TOOLWINDOW,GetDpiForWindow(window));
    SetWindowPos(window,nullptr,-30000,-30000,frame.right-frame.left,frame.bottom-frame.top,SWP_NOZORDER|SWP_NOACTIVATE);
    FrontendUiResize();surface->context->SetDensityIndependentPixelRatio(1.f);settle();
    ui.modernAdvanced=false;ui.modernContext=1;ui.modernSettings=false;ui.modernDiagram=false;
    ui.assignDevice(0);click("action-mc-style-modern");
    demand(ui.modernMode(),"modern selection enables behavior");
    capture("review-xbox-list.tga");
    click("action-mc-view-diagram");pointerClick("mc-art-hit-0");
    demand(ui.modernSelected==0,"controller art receives native pointer input");
    capture("review-xbox-diagram.tga");
    pointerClick("mc-art-hit-23");demand(ui.modernSelected==23,"trigger art hit target selects exact input");capture("review-xbox-trigger.tga");
    editInput(0);
    {
      auto* close=element("action-mc-dismiss");auto* text=close->GetFirstChild();
      auto box=close->GetBox().GetSize(Rml::BoxArea::Border);
      auto center=close->GetAbsoluteOffset(Rml::BoxArea::Border)+box*0.5f;
      auto label=text->GetAbsoluteOffset(Rml::BoxArea::Border)+text->GetBox().GetSize(Rml::BoxArea::Border)*0.5f;
      demand(std::abs(box.x-box.y)<1,"picker close button is square");
      demand(std::abs(center.x-label.x)<2&&std::abs(center.y-label.y)<2,"picker close glyph centered");
      capture("review-action-picker.tga");pointerClick("action-mc-dismiss");
      demand(ui.modernPanel.empty(),"picker close button dismisses through native pointer");
    }
    click("action-mc-tab-settings");capture("review-xbox-settings.tga");
    {
      auto* camera=element("mc-camera-card");auto* sticks=element("mc-sticks-card");
      const float gap=sticks->GetAbsoluteOffset().y-camera->GetAbsoluteOffset().y-camera->GetBox().GetSize(Rml::BoxArea::Border).y;
      demand(gap>=12 && gap<=16,"Sticks follows Camera with a single card gap");
      auto* done=element("controller-done");auto* doneText=done->GetFirstChild();
      demand(doneText && std::abs((doneText->GetAbsoluteOffset(Rml::BoxArea::Border).x+doneText->GetBox().GetSize(Rml::BoxArea::Border).x/2)-(done->GetAbsoluteOffset(Rml::BoxArea::Border).x+done->GetBox().GetSize(Rml::BoxArea::Border).x/2))<2,"Done text centered horizontally");
      demand(!ui.el("dialog-note")->IsVisible(true),"footer save status removed");
      ui.label("dialog-note","Could not save the experimental setting.");settle();
      demand(ui.el("dialog-note")->IsVisible(true),"real save failure remains visible");
      ui.label("dialog-note","Saved");settle();
      demand(!ui.el("dialog-note")->IsVisible(true),"success label stays removed after recovery");
      for(const char* id:{"action-mc-aim-hold","action-mc-scope-cycle","action-mc-curve-0"}) {
        auto* button=element(id);auto* text=button->GetFirstChild();
        auto a=button->GetAbsoluteOffset(Rml::BoxArea::Border)+button->GetBox().GetSize(Rml::BoxArea::Border)*0.5f;
        auto b=text->GetAbsoluteOffset(Rml::BoxArea::Border)+text->GetBox().GetSize(Rml::BoxArea::Border)*0.5f;
        demand(std::abs(a.x-b.x)<2 && std::abs(a.y-b.y)<2,"segment text centered in both axes");
      }
      demand(!ui.el("dialog")->QuerySelector(".dialog-header")->IsVisible(true),"redundant Controls banner removed");
      demand(ui.modernAutomaticGlyphLabel()=="Automatic · Xbox","automatic symbols show detected controller family");
      demand(ui.deviceName(0).find("Xbox")!=std::string::npos,"XInput assignment identifies controller type");
      auto* symbols=ui.doc->QuerySelector(".mc-symbols-label");demand(symbols&&symbols->GetClientWidth()>60&&symbols->GetFirstChild()->GetClientWidth()>60,"symbol label retains visible centered text");
      demand(element("action-device-remove")->GetFirstChild()->GetClientWidth()>25,"header Remove label remains visible");
      click("action-mc-tab-advanced");
      demand(ui.el("mc-range-axis")&&ui.el("mc-range-trigger")&&!ui.el("mc-range-camera"),"Advanced tab has dedicated content");
      capture("review-advanced.tga");
      click("action-mc-tab-settings");
      surface->context->SetDensityIndependentPixelRatio(1.25f);settle();capture("review-settings-125.tga");
      click("action-mc-tab-buttons");click("action-mc-view-list");
      ui.modernSelected=11;ui.modal("controllers");settle();capture("review-selection-125.tga");
      surface->context->SetDensityIndependentPixelRatio(1.5f);settle();capture("review-selection-150.tga");
      // Match the owner's captured client size at 125% desktop scaling.
      frame={0,0,1583,1095};AdjustWindowRectExForDpi(&frame,WS_OVERLAPPEDWINDOW,FALSE,WS_EX_TOOLWINDOW,GetDpiForWindow(window));
      SetWindowPos(window,nullptr,-30000,-30000,frame.right-frame.left,frame.bottom-frame.top,SWP_NOZORDER|SWP_NOACTIVATE);
      FrontendUiResize();surface->context->SetDensityIndependentPixelRatio(1.25f);settle();
      capture("feedback-owner-size-bindings.tga");
      pointerClick("action-mc-tab-settings");capture("feedback-owner-size-settings.tga");
      auto* advanced=element("action-mc-tab-advanced");
      demand(advanced->GetClientWidth()>80 && advanced->GetFirstChild()->GetClientWidth()>80,"Advanced label retains its width at owner display scale");
      std::ofstream(home/"owner-tabs.rml")<<ui.el("mc-toolbar")->GetInnerRML();
      pointerClick("action-mc-tab-advanced");capture("feedback-owner-size-advanced.tga");
      click("action-mc-tab-buttons");
      frame={0,0,1280,920};AdjustWindowRectExForDpi(&frame,WS_OVERLAPPEDWINDOW,FALSE,WS_EX_TOOLWINDOW,GetDpiForWindow(window));
      SetWindowPos(window,nullptr,-30000,-30000,frame.right-frame.left,frame.bottom-frame.top,SWP_NOZORDER|SWP_NOACTIVATE);
      FrontendUiResize();surface->context->SetDensityIndependentPixelRatio(1.f);settle();click("action-mc-view-diagram");
    }
    extra[0].family=jfg::ControllerFamily::dualsense;ui.discoverDevices();ui.assignDevice(4);click("action-mc-tab-buttons");
    capture("review-dualsense-diagram.tga");
    pointerClick("mc-art-hit-11");capture("review-dualsense-dpad.tga");
    click("action-mc-view-list");capture("review-dualsense-list.tga");
    click("action-mc-tab-settings");capture("review-dualsense-settings.tga");
    ui.assignDevice(-3);click("action-mc-tab-buttons");
    demand(!ui.el("mc-menu-bindings"),"no separate menu/dialogue card");
    const auto savedMenuProfile=ui.modern[0];
    const auto inKeys=[&](int key){return element("mc-gameplay-keys")->QuerySelector("#mc-row-"+std::to_string(key))!=nullptr;};
    demand(element("mc-column-0")->GetInnerRML().find("MOUSE")!=std::string::npos &&
        element("mc-column-0")->GetInnerRML().find("MOVEMENT KEYS")!=std::string::npos,"left mouse and movement lists retained");
    demand(inKeys('Q')&&inKeys(VK_F5),"Back and Save live in Keys");
    for(int key=37;key<=40;++key)demand(!ui.el(("mc-row-"+std::to_string(key)).c_str()),"no arrow rows");
    auto retained=savedMenuProfile;retained.keys[11]=VK_RETURN;retained.look.mouse_invert_y=0;
    demand(ui.saveModern(retained),"retained profile fixture saved");ui.loadModern();ui.modal("controllers");settle();
    demand(inKeys(VK_RETURN),"old Enter Confirm is visible in Keys");
    click("action-mc-add-key");settle();FrontendUiMessage(WM_KEYDOWN,'F',0);settle();click("action-mc-assign-11");
    demand(ui.modern[0].keys[11]=='F'&&inKeys('F'),"new F Confirm appears in Keys");
    ui.loadModern();ui.modal("controllers");settle();
    demand(inKeys('F')&&ui.modern[0].keys[4]=='E'&&!ui.modern[0].look.mouse_invert_y,"reload retains new row and other preferences");
    capture("unified-keys-f.tga");
    click("action-mc-add-key");settle();FrontendUiMessage(WM_KEYDOWN,'G',0);settle();click("action-mc-assign-18");
    demand(inKeys('G'),"new menu navigation key goes to Keys");
    click("action-mc-add-key");settle();FrontendUiMessage(WM_KEYDOWN,'I',0);settle();click("action-mc-assign-13");
    demand(inKeys('I')&&element("mc-column-0")->QuerySelector("#mc-row-87"),"new movement key goes to Keys and left W remains");
    click("action-mc-add-key");settle();FrontendUiMessage(WM_KEYDOWN,VK_UP,0);settle();
    demand(ui.modernPanel=="capture"&&ui.modernCapture==-2,"arrow key cannot be added");click("action-mc-cancel");
    editInput('E');click("action-mc-assign-11");
    demand(ui.modernPanel.empty()&&ui.modern[0].keys[11]=='E'&&ui.modern[0].keys[4]=='E',"E shares Confirm and Interact without replacement");
    ui.loadModern();ui.modal("controllers");settle();
    const auto shared=element("mc-row-69")->GetInnerRML();
    demand(inKeys('E')&&shared.find("Interact")!=std::string::npos&&shared.find("Confirm")!=std::string::npos,"both E actions visible after reload");
    editInput('E');click("action-mc-assign-11");
    demand(ui.modern[0].keys[11]==0&&ui.modern[0].keys[4]=='E',"unassigning Confirm keeps shared Interact");
    editInput('E');click("action-mc-assign-11");
    demand(ui.modern[0].keys[11]=='E'&&ui.modern[0].keys[4]=='E',"Confirm can be restored to shared E");
    editInput('E');click("action-mc-assign-1");demand(ui.modernPanel=="conflict","same-context duplicate offers choice");
    click("action-mc-conflict-both");
    demand(ui.modern[0].keys[1]=='E'&&ui.modern[0].keys[4]=='E'&&ui.modern[0].keys[11]=='E',"Keep both retains every shared action");
    ui.loadModern();ui.modal("controllers");settle();
    demand(ui.modern[0].keys[1]=='E'&&ui.modern[0].keys[4]=='E'&&ui.modern[0].keys[11]=='E',"same-context duplicates persist");
    click("action-mc-view-diagram");
    demand(element("mc-gameplay-keys")->QuerySelector("#action-mc-select-73") &&
        element("mc-gameplay-keys")->QuerySelector("#action-mc-select-71"),"diagram Keys includes new movement and menu keys");
    demand(!ui.el("mc-menu-bindings"),"diagram has no separate menu card");
    {
      Rml::ElementList ids;ui.doc->QuerySelectorAll(ids,"[id]");std::vector<std::string> unique;
      for(auto* e:ids){demand(std::find(unique.begin(),unique.end(),e->GetId())==unique.end(),"diagram IDs remain unique with shared keys");unique.push_back(e->GetId());}
    }
    click("action-mc-view-list");
    demand(ui.saveModern(savedMenuProfile),"restore only test profile");ui.modal("controllers");settle();
    capture("unified-keys-shared-e.tga");
    demand(!element("mc-gameplay-keys")->QuerySelector(".mc-binding-value.free"),"Keys contains only bound inputs");
    editInput(VK_SPACE);
    demand(ui.el("action-mc-assign-11")&&ui.el("action-mc-assign-18"),"menu bindings remain accessible in the action picker");
    click("action-mc-cancel");
    click("action-mc-free-32");
    demand(!ui.el("mc-row-32")&&ui.modern[0].keys[2]==0,"clearing a key removes its row");
    click("action-mc-add-key");settle();
    demand(ui.modernCapture==-2,"Add a key remains available when a row is removed");
    FrontendUiMessage(WM_KEYDOWN,'Q',0);settle();
    demand(ui.modernPanel=="picker"&&ui.modernSelected=='Q',"added key opens action picker");
    click("action-mc-assign-2");
    demand(ui.el("mc-row-81")&&ui.modern[0].keys[2]=='Q',"newly bound key appears in the list");
    ui.modern[0].keys[2]=VK_SPACE;ui.saveModern(ui.modern[0]);ui.modal("controllers");settle();
    frame={0,0,1583,1095};AdjustWindowRectExForDpi(&frame,WS_OVERLAPPEDWINDOW,FALSE,WS_EX_TOOLWINDOW,GetDpiForWindow(window));
    SetWindowPos(window,nullptr,-30000,-30000,frame.right-frame.left,frame.bottom-frame.top,SWP_NOZORDER|SWP_NOACTIVATE);
    FrontendUiResize();
    for(float dpi:{1.f,1.25f,1.5f}) {
      surface->context->SetDensityIndependentPixelRatio(dpi);settle();
      float labelX=-1;
      Rml::ElementList rows;ui.doc->QuerySelectorAll(rows,"#mc-gameplay-keys .mc-binding-row");
      demand(rows.size()==7,"Keys includes gameplay plus Back and Save with shared E");
      for(auto* row:rows) {
        auto* chip=row->QuerySelector(".mc-chip");auto* name=row->QuerySelector(".mc-physical-name");
        demand(chip&&name,"key row retains keycap and name");
        if(name->IsVisible(true)) {
          float x=name->GetAbsoluteOffset(Rml::BoxArea::Border).x;
          if(labelX<0)labelX=x;
          demand(std::abs(x-labelX)<1,"all keyboard names share one left edge");
          demand(chip->GetAbsoluteOffset(Rml::BoxArea::Border).x+chip->GetBox().GetSize(Rml::BoxArea::Border).x+3*dpi<=x,"wide keycap does not overlap name");
        }
        auto* clear=row->QuerySelector(".mc-row-clear");auto size=clear->GetBox().GetSize(Rml::BoxArea::Border);
        demand(std::abs(size.x-size.y)<1,"row clear hit area and hover background are square");
        auto* icon=clear->GetFirstChild();
        auto center=clear->GetAbsoluteOffset(Rml::BoxArea::Border)+size*0.5f;
        auto glyph=icon->GetAbsoluteOffset(Rml::BoxArea::Border)+icon->GetBox().GetSize(Rml::BoxArea::Border)*0.5f;
        demand(std::abs(center.x-glyph.x)<1&&std::abs(center.y-glyph.y)<1,"clear cross geometry centered");
      }
      if(dpi==1.25f) {
        auto* clear=element("action-mc-free-32");auto pt=clear->GetAbsoluteOffset(Rml::BoxArea::Border)+clear->GetBox().GetSize(Rml::BoxArea::Border)*0.5f;
        surface->context->ProcessMouseMove(int(pt.x),int(pt.y),0);settle();capture("review-keyboard-clear-hover-125.tga");
        editInput(VK_SPACE);surface->context->ProcessMouseMove(0,0,0);settle();
        auto* close=element("action-mc-dismiss");
        demand(close->GetProperty<Rml::Colourb>("background-color").alpha==0,"modal close has no permanent background");
        auto size=close->GetBox().GetSize(Rml::BoxArea::Border);
        auto center=close->GetAbsoluteOffset(Rml::BoxArea::Border)+size*0.5f;
        auto* icon=close->GetFirstChild();auto glyph=icon->GetAbsoluteOffset(Rml::BoxArea::Border)+icon->GetBox().GetSize(Rml::BoxArea::Border)*0.5f;
        demand(std::abs(size.x-size.y)<1&&std::abs(center.x-glyph.x)<1&&std::abs(center.y-glyph.y)<1,"modal close square and cross centered at 125 percent");
        capture("review-keyboard-picker-125.tga");pointerClick("action-mc-dismiss");
      }
    }
    frame={0,0,1280,920};AdjustWindowRectExForDpi(&frame,WS_OVERLAPPEDWINDOW,FALSE,WS_EX_TOOLWINDOW,GetDpiForWindow(window));
    SetWindowPos(window,nullptr,-30000,-30000,frame.right-frame.left,frame.bottom-frame.top,SWP_NOZORDER|SWP_NOACTIVATE);
    FrontendUiResize();surface->context->SetDensityIndependentPixelRatio(1.f);settle();capture("review-keyboard-list.tga");
    click("action-mc-view-diagram");pointerClick("mc-mouse-hit-1");
    demand(ui.modernSelected==1,"mouse diagram receives native pointer input");capture("review-keyboard-diagram.tga");
    pointerClick("mc-key-87");demand(ui.modernSelected=='W',"keycap receives native pointer input");
    click("action-mc-mouse-select--12");demand(ui.modernSelected==-12,"mouse movement legend preserves virtual-input identity");
    click("action-mc-mouse-settings");demand(ui.modernSettings&&ui.el("mc-range-camera"),"mouse movement opens its settings");
    capture("review-keyboard-settings.tga");
    Rml::ElementList ids;ui.doc->QuerySelectorAll(ids,"[id]");std::vector<std::string> seenIds;
    for(auto* node:ids) {
      const auto id=node->GetId();demand(std::find(seenIds.begin(),seenIds.end(),id)==seenIds.end(),("unique RmlUi element ID "+id).c_str());
      seenIds.push_back(id);
    }
    // The v1.1 home flow must survive integration with the controls editor.
    // No ROM is opened here: the helper separately verifies/imports it.
    FrontendUiShutdown();ready=false;
    const auto homeProfile=home/"home-profile";fs::create_directories(homeProfile);
    demand(FrontendUiInit(window,homeProfile,[](const std::string&){}),"home UI initializes");ready=true;
    FrontendUiState homeState;
    for(int i=0;i<3;++i)FrontendUiFrame(homeState);
    demand(element("rom-card")->IsVisible(true),"first run shows ROM import card");
    homeState.rom=(homeProfile/"roms/jfg-us.z64").wstring();
    for(int i=0;i<3;++i)FrontendUiFrame(homeState);
    demand(!element("rom-card")->IsVisible(true),"imported ROM removes card from home");
    demand(surface->ui.doc->IsClassSet("rom-imported"),"home recognizes imported profile");
    homeState.rom.clear();
    for(int i=0;i<3;++i)FrontendUiFrame(homeState);
    demand(element("rom-card")->IsVisible(true),"missing ROM restores import entry");
    std::ofstream(home/"result.json")<<"{\"passed\":true,\"checks\":"<<checks<<",\"physical_desktop_input\":false}";
    FrontendUiShutdown();ready=false;DestroyWindow(window);return 0;
  } catch(const std::exception& error) {
    if(!home.empty())std::ofstream(home/"result.json")<<"{\"passed\":false,\"checks\":"<<checks<<",\"error\":\""<<escape(error.what())<<"\"}";
    if(ready)FrontendUiShutdown();if(window)DestroyWindow(window);return 1;
  }
}
