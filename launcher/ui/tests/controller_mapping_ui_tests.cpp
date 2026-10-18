// Isolated original mapping/device regression; no desktop input injection.
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include "../native_ui.cpp"
#include <stdexcept>
static fs::path home;
static int checks=0;
static void demand(bool pass,const char* msg){++checks;if(!pass)throw std::runtime_error(msg);}
static void frame(){MSG m{};while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}FrontendUiFrame({});}
static void wait(){for(int i=0;i<5;++i){frame();Sleep(20);}}
static Rml::Element* element(const char* id){auto e=!surface->ui.popup() && std::string(id).rfind("action-",0)==0?surface->ui.doc->QuerySelector(std::string("#dialog #")+id):surface->ui.doc->GetElementById(id);demand(e!=nullptr,id);return e;}
static void click(const char* id){element(id)->Click();wait();}
static void capture(const char* name="pc-settings.tga"){
 auto& s=*surface;frame();ComPtr<ID3D11Resource> resource;s.target->GetResource(&resource);ComPtr<ID3D11Texture2D> target;check(resource.As(&target));D3D11_TEXTURE2D_DESC desc{};target->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;ComPtr<ID3D11Texture2D> staging;check(s.device->CreateTexture2D(&desc,nullptr,&staging));s.gpu->CopyResource(staging.Get(),target.Get());D3D11_MAPPED_SUBRESOURCE mapped{};check(s.gpu->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped));
 std::ofstream file(home/name,std::ios::binary);unsigned char header[18]{};header[2]=2;header[12]=static_cast<unsigned char>(desc.Width);header[13]=static_cast<unsigned char>(desc.Width>>8);header[14]=static_cast<unsigned char>(desc.Height);header[15]=static_cast<unsigned char>(desc.Height>>8);header[16]=32;header[17]=40;file.write(reinterpret_cast<char*>(header),18);for(unsigned y=0;y<desc.Height;y++)file.write(static_cast<char*>(mapped.pData)+y*mapped.RowPitch,desc.Width*4);s.gpu->Unmap(staging.Get(),0);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int){
 HWND window=nullptr;bool ready=false;
 try {
  int argc=0;auto** argv=CommandLineToArgvW(GetCommandLineW(),&argc);
  if(argc!=2){LocalFree(argv);return 2;}home=argv[1];LocalFree(argv);
  if(fs::exists(home))return 3;fs::create_directories(home);
  std::ofstream(home/"mapping-window-ui-result.json")<<"{\"passed\":false}";
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  WNDCLASSW cls{};cls.lpfnWndProc=DefWindowProcW;cls.hInstance=instance;cls.lpszClassName=L"JfgControllerSetupCheck";RegisterClassW(&cls);
  window=CreateWindowExW(WS_EX_TOOLWINDOW,cls.lpszClassName,L"Controller setup UI check",WS_OVERLAPPEDWINDOW,-30000,-30000,1000,820,nullptr,nullptr,instance,nullptr);
  const auto profile=home/"controller-setup-check-profile-final";fs::create_directories(profile);
  {std::ofstream f(profile/"experimental-controls.ini");f<<jfg::serialize_pc_experiments(false);}
  std::string routed;
  demand(FrontendUiInit(window,profile,[&](const std::string& a){routed=a;},"controllers"),"standalone UI init");ready=true;
  auto& ui=surface->ui;
  std::vector<jfg::ControllerDeviceInfo> extra;ui.sdlDiscovery=[&]{return extra;};
  const fs::path unicodeRuntime=home/L"Game \u65e5\u672c"/L"jfg-native-boot.exe";
  {std::ofstream(profile/"frontend-runtime.txt",std::ios::binary)<<u8(unicodeRuntime.wstring())<<"\r\n";}
  demand(ui.controllerSdkDirectory()==unicodeRuntime.parent_path(),"separate mapping process resolves profile Unicode runtime");
  fs::remove(profile/"frontend-runtime.txt");
  std::array<bool,4> hardware{true,true,false,false};
  std::array<XINPUT_STATE,4> inputs{};
  ui.controllerState=[&](DWORD d,XINPUT_STATE* s)->DWORD{if(d>=inputs.size())return ERROR_DEVICE_NOT_CONNECTED;*s=inputs[d];return hardware[d]?ERROR_SUCCESS:ERROR_DEVICE_NOT_CONNECTED;};
  for(int i=0;i<4;++i){ui.player=i;ui.mappings[i].device=-2;ui.saveController();}
  ui.player=0;ui.assignments=jfg::ControllerPorts{};ui.loadControllers();ui.discoverDevices();ui.modal("controllers");
  ShowWindow(window,SW_SHOWNOACTIVATE);FrontendUiResize();wait();
  auto exists=[&](const char* selector){return ui.doc->QuerySelector(selector)!=nullptr;};
  auto change=[&](const char* selector,int value){auto e=ui.doc->QuerySelector(selector);demand(e!=nullptr,selector);Rml::Dictionary d;d["value"]=value;e->DispatchEvent("change",d);wait();};
  auto option=[&](int value){auto e=dynamic_cast<Rml::ElementFormControlSelect*>(ui.el("available-device"));if(!e)return false;for(int i=0;i<e->GetNumOptions();++i)if(e->GetOption(i)->GetAttribute<int>("value",-99)==value)return true;return false;};
  auto choose=[&](int device){if(device==-2){click("action-device-remove");return;}click("action-device-open");change("#input-type",device<0?device:0);if(device>=0){demand(option(device),"requested device is selectable");change("#available-device",device);}click("action-device-apply");};
  demand(!exists("#action-pc-experimental") && !exists("#action-learn0"),"disconnected player has no binding controls");
  const auto initial=ui.read("controller.ini");click("action-device-open");
  demand(option(0)&&option(1)&&!option(2)&&!option(3),"picker only lists connected controllers");
  change("#available-device",1);click("action-device-cancel");
  demand(ui.read("controller.ini")==initial,"cancel does not change saved device");
  choose(-3);
  demand(exists("#action-pc-bind2")&&!exists("#action-learn2"),"keyboard-specific content");
  demand(!element("action-pc-bind2")->HasAttribute("disabled"),"keyboard bindings editable without experiment");
  click("action-pc-normal");
  demand(ui.pcInput[0].keys[0]==VK_SPACE&&ui.pcInput[0].keys[10]==jfg::kPcWheelUp&&ui.pcInput[0].keys[11]==jfg::kPcWheelDown&&ui.pcInput[0].keys[1]==VK_CONTROL&&ui.pcInput[0].keys[19]=='E',"Normal keyboard jump and menu confirm");
  click("action-pc-expert");demand(ui.pcInput[0].keys[10]==VK_SPACE&&ui.pcInput[0].keys[0]==jfg::kPcWheelDown&&ui.pcInput[0].keys[1]==jfg::kPcWheelUp&&ui.pcInput[0].keys[11]==VK_CONTROL&&ui.pcInput[0].keys[19]=='E',"Expert keyboard jump and confirm");click("action-pc-normal");
  click("action-pc-bind14");ui.keyReleased=true;FrontendUiMessage(WM_KEYDOWN,'T',0);wait();
  demand(!ui.modernMode()&&ui.pcInput[0].keys[14]=='T',"rebind with experiment off preserves opt-out");
  ui.loadPcInput();demand(ui.pcInput[0].keys[14]=='T',"off-state binding persisted");
  click("action-pc-unbind14");demand(ui.pcInput[0].keys[14]==0,"clear works without experiment");
  click("action-pc-reset");demand(ui.pcInput[0].keys[14]=='W',"reset works without experiment");
  demand(!exists("#action-pc-dual")&&!exists("#action-stick"),"keyboard has no controller experiments");
  click("action-pc-normal");
  jfg::PcInputConfig saved;
  demand(jfg::parse_pc_input(ui.read("pc-input.ini"),saved)&&saved.modern==1&&saved.keys[2]==1,"inline keyboard preset saved");
  click("action-pc-bind2");ui.keyReleased=false;
  FrontendUiMessage(WM_XBUTTONDOWN,static_cast<WPARAM>(XBUTTON1)<<16,0);
  demand(ui.keyLearning==2&&ui.pcInput[0].keys[2]==1,"capture waits for release");
  ui.keyReleased=true;FrontendUiMessage(WM_XBUTTONDOWN,static_cast<WPARAM>(XBUTTON1)<<16,0);wait();
  demand(ui.keyLearning<0&&jfg::parse_pc_input(ui.read("pc-input.ini"),saved)&&saved.keys[2]==5,"mouse side button binding persisted");
  click("action-pc-bind1");ui.keyReleased=true;FrontendUiMessage(WM_MOUSEWHEEL,static_cast<WPARAM>(WHEEL_DELTA)<<16,0);wait();
  demand(ui.pcInput[0].keys[1]==256,"mouse wheel binding captured");
  click("action-pc-bind14");ui.keyReleased=true;FrontendUiMessage(WM_KEYDOWN,'I',0);wait();
  demand(ui.pcInput[0].keys[14]=='I',"keyboard key captured");
  click("action-pc-bind14");ui.keyReleased=true;FrontendUiMessage(WM_KEYDOWN,VK_F11,0);
  demand(ui.keyLearning==14&&ui.pcInput[0].keys[14]=='I',"reserved shortcut not bound");
  FrontendUiMessage(WM_KEYDOWN,VK_ESCAPE,0);wait();demand(ui.keyLearning<0&&ui.pcInput[0].keys[14]=='I',"Escape cancels capture without closing");
  click("action-pc-bind14");FrontendUiMessage(WM_KILLFOCUS,0,0);wait();demand(ui.keyLearning<0,"focus loss cancels capture");
  click("action-mapping-tab-settings");
  demand(exists("#controller-settings")&&!exists("#action-pc-mouse"),"original keyboard settings never advertise mouse look");
  click("action-mapping-tab-buttons");capture("mapping-window-keyboard.tga");
  demand(element("action-pc-bind19")->IsVisible(true),"last keyboard binding reachable");
  choose(0);
  demand(element("dialog-body")->GetScrollTop()==0,"device switch resets binding scroll");
  demand(exists("#action-learn2")&&!exists("#action-pc-bind2"),"controller-specific content");
  demand(!exists("#action-pc-experimental")&&!exists("#action-pc-dual")&&!exists("#action-mc-experimental"),"mode selection replaces experiment switches");
  demand(!element("dialog-note")->IsVisible(true),"original mapping has no persistent saved status");
  click("action-mapping-tab-settings");
  demand(exists("input[data-action=tune-deadzone]")&&!exists("#action-learn0"),"settings tab replaces mapping cards");
  const auto bindings=ui.mappings[0].bindings;
  change("input[data-action=tune-deadzone]",12000);click("action-invertx");
  click("action-mapping-tab-advanced");change("input[data-action=tune-trigger]",5000);
  demand(ui.mappings[0].deadzone==12000&&ui.mappings[0].trigger==5000&&ui.mappings[0].invert_x==1,"original tuning saves");
  click("action-mapping-reset-tuning");
  demand(ui.mappings[0].bindings==bindings&&ui.mappings[0].deadzone==jfg::ControllerMapping{}.deadzone,"tuning reset preserves bindings");
  click("action-mapping-tab-buttons");click("action-mapping-stick-right");
  demand(ui.mappings[0].stick==1,"right movement stick selectable");click("action-mapping-stick-left");
  demand(ui.mappings[0].stick==0,"left movement stick selectable");
  click("action-pc-pad-normal");
  demand(ui.mappings[0].bindings[2]==25&&ui.mappings[0].bindings[0]==0&&ui.mappings[0].bindings[10]==2,"Normal controller jump and confirm");
  click("action-pc-pad-expert");demand(ui.mappings[0].bindings[0]==2&&ui.mappings[0].bindings[10]==0,"Expert controller jump and confirm");click("action-pc-pad-normal");
  inputs[0].Gamepad.wButtons=XINPUT_GAMEPAD_A;wait();click("action-learn1");
  demand(ui.learning==1&&!ui.learnReleased,"gamepad binding waits for release");
  demand(element("dialog-note")->IsVisible(true),"release-before-capture guidance remains visible");
  inputs[0].Gamepad.wButtons=0;wait();demand(ui.learnReleased,"neutral gamepad arms capture");
  inputs[0].Gamepad.wButtons=XINPUT_GAMEPAD_Y;wait();inputs[0].Gamepad.wButtons=0;
  demand(ui.learning<0&&ui.mappings[0].bindings[1]==3,"click binding captures gamepad button");
  click("action-learn1");FrontendUiMessage(WM_KILLFOCUS,0,0);wait();demand(ui.learning<0,"gamepad capture cancels on focus loss");
  {
    RECT frame{0,0,1583,1095};AdjustWindowRectExForDpi(&frame,WS_OVERLAPPEDWINDOW,FALSE,WS_EX_TOOLWINDOW,GetDpiForWindow(window));
    SetWindowPos(window,nullptr,-30000,-30000,frame.right-frame.left,frame.bottom-frame.top,SWP_NOZORDER|SWP_NOACTIVATE);
    FrontendUiResize();surface->context->SetDensityIndependentPixelRatio(1.25f);wait();
    capture("mapping-owner-size.tga");
    demand(element("action-learn0")->QuerySelector(".mc-prototype-chip")!=nullptr,"original binding shows physical glyph");
    demand(element("action-learn12")->QuerySelector(".mapping-unbound")->GetClientWidth()<35,"unbound glyph does not stretch the row");
    auto* first=element("binding-0");auto* card=first->GetParentNode();
    demand(first->GetAbsoluteOffset().x>card->GetAbsoluteOffset().x,"mapping card contains left padding");
    auto* symbols=ui.doc->QuerySelector(".mc-symbols-label");auto* select=element("mc-glyph-select");
    demand(symbols->GetClientWidth()>60&&symbols->GetFirstChild()->GetClientWidth()>60,"button symbols label has visible text width");
    demand(element("action-device-remove")->GetFirstChild()->GetClientWidth()>25,"Remove has visible text width");
    auto* deviceName=ui.doc->QuerySelector(".mc-device-text strong");auto* assignment=element("assignment");
    const float descriptionGap=assignment->GetAbsoluteOffset().y-deviceName->GetAbsoluteOffset().y-deviceName->GetClientHeight();
    demand(descriptionGap>=0&&descriptionGap<=4,"device title and status form one compact block");
    demand(std::abs((symbols->GetAbsoluteOffset().y+symbols->GetClientHeight()/2)-(select->GetAbsoluteOffset().y+select->GetBox().GetSize(Rml::BoxArea::Border).y/2))<2,"symbol label vertically centered");
    click("action-mapping-diagram");demand(exists(".mapping-art"),"original controller diagram available");
    auto* art=ui.doc->QuerySelector(".mapping-art .mc-pad-art");demand(std::abs(art->GetClientWidth()/art->GetClientHeight()-904.f/680.f)<0.01f,"original diagram preserves controller aspect ratio");
    click("mc-art-hit-0");demand(ui.modernSelected==0&&element("binding-0")->IsClassSet("selected"),"original diagram selects the corresponding mapping");capture("mapping-owner-diagram.tga");
    click("action-mapping-diagram");
    click("action-mapping-tab-settings");capture("mapping-owner-settings.tga");
    click("action-mapping-tab-advanced");capture("mapping-owner-advanced.tga");
    click("action-mapping-tab-buttons");
    const auto original=ui.read("controller.ini");const auto keymap=ui.read("pc-input.ini");
    click("action-mc-style-modern");demand(ui.modernMode()&&!exists("#action-mc-experimental"),"one selection enables modern");
    click("action-mc-style-mapping");
    demand(!ui.modernMode()&&ui.read("controller.ini")==original&&ui.read("pc-input.ini")==keymap,"switching styles preserves original mappings");
    surface->context->SetDensityIndependentPixelRatio(1.f);wait();
  }
  capture("mapping-window-controller.tga");
  click("action-player1");click("action-device-open");
  demand(!option(0)&&option(1)&&!option(2),"other player's connected controller excluded");
  capture("mapping-window-device-picker.tga");
  change("#available-device",1);const auto player2Before=ui.read("controller-2.ini");
  hardware[1]=false;element("action-device-apply")->Click();wait();
  demand(ui.mappings[1].device==-2&&ui.read("controller-2.ini")==player2Before,"unplug between selection and apply rejected");
  demand(!option(1)&&element("action-device-apply")->HasAttribute("disabled"),"empty device picker disables add");
  hardware[1]=true;wait();demand(option(1)&&!element("action-device-apply")->HasAttribute("disabled"),"reconnect refreshes picker");
  click("action-device-apply");demand(ui.mappings[1].device==1,"available gamepad added");
  click("action-pc-pad-expert");choose(-2);
  demand(ui.pcInput[0].keys[2]==5,"other player changes retain first player key bindings");
  click("action-player0");choose(-1);demand(ui.devices[0]==0&&!ui.pcKeyboard(),"automatic follows assigned gamepad");
  click("action-player1");click("action-device-open");demand(!option(0)&&option(1),"automatic ownership excludes controller for other players");
  click("action-device-cancel");click("action-player0");
  const auto savedAuto=ui.read("controller.ini");hardware[0]=false;wait();
  demand(ui.read("controller.ini")==savedAuto&&ui.devices[0]==0,"unplug preserves automatic assignment and mappings");
  demand(element("action-learn0")->HasAttribute("disabled"),"disconnected controller cannot capture bindings");
  click("action-device-open");demand(!option(0)&&option(1),"offline saved device absent from picker");
  hardware[0]=true;wait();demand(option(0)&&option(1),"own reconnected controller remains selectable");
  click("action-device-cancel");choose(-3);
  demand(jfg::parse_pc_input(ui.read("pc-input.ini"),saved)&&saved.keys[2]==5,"switching devices retains mouse settings");

  click("action-pc-normal");
  click("action-player1");demand(ui.pcInput[1].rumble_strength==0,"rumble setting isolated per player");click("action-device-open");change("#input-type",-3);
  demand(element("action-device-apply")->HasAttribute("disabled"),"keyboard cannot belong to two players");
  click("action-device-cancel");click("action-player0");
  demand(!ui.modernMode()&&!element("action-pc-bind2")->HasAttribute("disabled")&&ui.pcInput[0].keys[2]==1,"opt-out retains editable keyboard mappings");
  choose(-2);hardware.fill(false);ui.assignments=jfg::ControllerPorts{};ui.loadControllers();wait();choose(-1);
  demand(ui.pcKeyboard()&&exists("#action-pc-bind2"),"automatic keyboard fallback deterministic without hardware");
  choose(-2);
  jfg::ControllerDeviceInfo transient{"sdl:"+std::string(64,'c'),"No stable identity"};transient.connected=true;transient.session_only=true;
  extra={transient};wait();click("action-device-open");
  demand(!option(ui.controllerCatalog.find(transient.key)),"unidentifiable device offered for a cross-process assignment");click("action-device-cancel");
  jfg::ControllerDeviceInfo first{"sdl:"+std::string(64,'a'),"Test <Sony> pad"};first.connected=true;
  jfg::ControllerDeviceInfo second{"sdl:"+std::string(64,'b'),"Test Nintendo pad"};second.connected=true;
  extra={first,second};wait();
  const int sony=ui.controllerCatalog.find(first.key),nintendo=ui.controllerCatalog.find(second.key);
  demand(sony>=4&&nintendo>=4&&sony!=nintendo,"SDL device slots");
  choose(sony);demand(ui.mappings[0].device==-4&&ui.mappings[0].device_key==first.key,"SDL selection persisted by identity");
  jfg::ControllerMapping selected;demand(jfg::parse_controller_mapping(ui.read("controller.ini"),selected)&&selected.device_key==first.key,"SDL profile parser agreement");
  demand(exists("#action-learn2")&&!exists("#action-pc-bind2"),"SDL gamepad content");
  click("action-pc-pad-normal");click("action-mapping-tab-settings");
  demand(!exists("#action-pc-dual")&&!exists("[data-action=pc-tune7]"),"original SDL mapping exposes original tuning only");
  click("action-mapping-tab-buttons");
  demand(ui.mappings[0].device_key==first.key,"preset cleared SDL identity");
  click("action-reset");demand(ui.mappings[0].device_key==first.key,"reset cleared SDL identity");
  click("action-learn1");wait();extra[0].sample.buttons[5]=true;wait();
  demand(ui.learning<0&&ui.mappings[0].bindings[1]==5,"SDL Guide button capture");extra[0].sample={};wait();
  click("action-player1");click("action-device-open");demand(!option(sony)&&option(nintendo),"assigned SDL device excluded");
  click("action-device-cancel");choose(nintendo);click("action-player0");
  const auto sdlSaved=ui.read("controller.ini");extra={second};wait();
  demand(ui.devices[0]==sony&&!ui.connected[sony]&&ui.read("controller.ini")==sdlSaved,"SDL unplug lost assignment");
  click("action-device-open");demand(!option(sony)&&!option(nintendo),"offline or owned SDL offered");click("action-device-cancel");
  extra={second,first};wait();demand(ui.devices[0]==sony&&ui.connected[sony],"reordered SDL reconnect changed identity");
  ui.assignments=jfg::ControllerPorts{};ui.loadControllers();wait();
  demand(ui.devices[0]==sony&&ui.devices[1]==nintendo,"saved SDL assignments reload");
  capture("sdl-controller-settings.tga");
  RECT size{};GetClientRect(window,&size);demand(element("dialog")->GetClientWidth()>float(size.right)*.95F,"RmlUi mapping fills separate window");
  SetWindowPos(window,nullptr,-30000,-30000,740,760,SWP_NOZORDER|SWP_NOACTIVATE);FrontendUiResize();wait();
  demand(element("action-player3")->GetAbsoluteOffset(Rml::BoxArea::Border).x<700,"player tabs fit minimum window width");
  ui.controllerWindow=false;ui.modal("menu-controllers");wait();click("action-controllers");
  demand(routed=="controllers"&&ui.current.empty(),"launcher menu opens separate controller window");
  std::ofstream(home/"mapping-window-ui-result.json")<<"{\"passed\":true,\"device_add_remove\":true,\"connected_available_filter\":true,\"automatic_ownership\":true,\"unplug_reconnect\":true,\"stale_selection_rejected\":true,\"keyboard_mouse_capture\":true,\"controller_capture\":true,\"mode_selection\":true,\"player_isolation\":true,\"minimum_window_layout\":true}";
  std::ofstream(home/"check-count.txt")<<checks;FrontendUiShutdown();DestroyWindow(window);return 0;
 }catch(const std::exception& e){if(ready){capture("mapping-window-failed.tga");std::ofstream(home/"failed-ui.rml")<<surface->ui.el("dialog-body")->GetInnerRML();}std::ofstream(home/"mapping-window-ui-error.txt")<<e.what();if(ready)FrontendUiShutdown();if(window)DestroyWindow(window);return 1;}
}
