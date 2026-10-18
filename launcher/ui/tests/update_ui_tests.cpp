// Injected devices and an offscreen, nonactivating HWND. Never sends desktop input.
// Usage: jfg_update_ui_tests.exe <new-artifact-directory>.
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include "../native_ui.cpp"
#include <stdexcept>
static fs::path home;
static int checks=0;
static void demand(bool pass,const char* message) {
  ++checks;if(!pass)throw std::runtime_error(message);
}
static FrontendUiState frameState;
static void settle() {for(int i=0;i<3;++i)FrontendUiFrame(frameState);}
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
    WNDCLASSW cls{};cls.lpfnWndProc=DefWindowProcW;cls.hInstance=instance;cls.lpszClassName=L"JfgUpdateUiTests";RegisterClassW(&cls);
    RECT rect{0,0,1280,780};AdjustWindowRectExForDpi(&rect,WS_OVERLAPPEDWINDOW,FALSE,WS_EX_TOOLWINDOW,96);
    window=CreateWindowExW(WS_EX_TOOLWINDOW,cls.lpszClassName,L"Offscreen launcher update checks",WS_OVERLAPPEDWINDOW,-30000,-30000,rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,instance,nullptr);
    const auto profile=home/"profile";fs::create_directories(profile);
    int requests=0;std::string lastAction;
    demand(FrontendUiInit(window,profile,[&](const std::string& a){lastAction=a;if(a=="updates"){++requests;std::ofstream(profile/"frontend-update.txt",std::ios::trunc);}}),"native UI initializes");ready=true;
    auto& ui=surface->ui;ui.sdlDiscovery=[] {return std::vector<jfg::ControllerDeviceInfo>{};};
    ui.controllerState=[](DWORD,XINPUT_STATE*)->DWORD{return ERROR_DEVICE_NOT_CONNECTED;};
    ShowWindow(window,SW_SHOWNOACTIVATE);FrontendUiResize();surface->context->SetDensityIndependentPixelRatio(1.f);
    auto result=[&](const std::string& wire){std::ofstream(profile/"frontend-update.txt")<<wire;ui.updatePoll=0;settle();};
    demand(requests==1 && ui.updatePending,"startup starts one background check");
    demand(ui.current.empty(),"startup check does not open a modal");
    ui.status("Import your ROM once, then click Play. No build tools are required.");
    settle();capture("startup-screen.tga");
    result("available\nv1.3.0\nA newer stable release is ready to download.\n");
    demand(!ui.updatePending && ui.availableUpdate=="v1.3.0","newer release accepted");
    demand(element("update-notice")->IsVisible(true),"persistent notification visible");
    demand(ui.current.empty(),"notification does not interrupt with modal");
    demand(element("update-notice")->GetParentNode()->IsClassSet("content"),"update banner lives below Play");
    auto bannerPoint=element("update-notice")->GetAbsoluteOffset(Rml::BoxArea::Border);
    auto* play=ui.doc->QuerySelector(".play");
    demand(bannerPoint.y>=play->GetAbsoluteOffset(Rml::BoxArea::Border).y+play->GetClientHeight(),"banner follows Play vertically");
    demand(element("update-notice")->GetClientWidth()>=600 && element("update-notice")->GetClientHeight()>=72,"banner is a large click target");
    demand(element("update-current")->GetInnerRML().find(jfg::frontend::version)!=std::string::npos,"banner shows installed version");
    pointerClick("update-download");demand(lastAction=="releases","Update now opens fixed GitHub release page");
    capture("update-available-home.tga");pointerClick("update-notes");
    demand(lastAction=="project" && ui.current.empty(),"Whats new opens main repository without a modal");
    ui.modal("updates");settle();
    demand(ui.current=="updates","notification opens update details");
    demand(element("dialog-body")->GetInnerRML().find("v1.3.0")!=std::string::npos,"latest version displayed");
    click("action-releases");demand(lastAction=="releases","download action routes to fixed release page");
    capture("update-details.tga");ui.close();ui.modal("menu-help");
    demand(element("action-check-updates")->GetInnerRML().find("Check for updates")!=std::string::npos,"Help menu entry present");
    click("action-check-updates");demand(requests==2 && ui.updatePending,"manual check starts fresh request");
    ui.beginUpdateCheck();demand(requests==2,"in-flight checks deduplicated");
    demand(element("action-retry-updates")->HasAttribute("disabled"),"retry disabled while checking");
    result("current\nv1.2.0\nNo newer stable release is available.\n");
    demand(ui.updateState=="current" && ui.availableUpdate.empty(),"current release state");
    demand(!element("update-notice")->IsVisible(true),"current version has no update notice");
    capture("update-current.tga");click("action-retry-updates");
    result("error\n\nCould not reach GitHub. You can keep playing offline.\n");
    demand(ui.updateState=="error" && !ui.updatePending,"offline request resolves");
    demand(element("dialog-body")->GetInnerRML().find("offline")!=std::string::npos,"offline explanation visible");
    capture("update-offline.tga");click("action-retry-updates");
    ui.updateDeadline=-1;ui.updatePoll=0;settle();demand(ui.updateState=="error" && !ui.updatePending,"missing helper response times out");
    ui.close();demand(!FrontendUiCapturing(),"update panel closes normally");
    ui.beginUpdateCheck();result("available\nv1.3.0\nNew version.\n");
    ui.beginUpdateCheck();result("error\n\nOffline.\n");
    demand(ui.availableUpdate=="v1.3.0","failed recheck preserves previously discovered update");
    FrontendUiState playing;playing.playing=true;FrontendUiFrame(playing);
    demand(!element("update-notice")->IsVisible(true),"home notice stays off gameplay");
    demand(ui.current.empty(),"background result never captures gameplay");
    // Network text is rendered as text, never interpreted as UI markup.
    FrontendUiFrame({});ui.beginUpdateCheck();result("error\n\n<evil> & text\n<evil>release notes</evil>\nsecond line");ui.modal("updates");settle();
    demand(surface->ui.doc->QuerySelector("evil")==nullptr,"release text is escaped");
    SetWindowPos(window,nullptr,-30000,-30000,850,650,SWP_NOZORDER|SWP_NOACTIVATE);FrontendUiResize();settle();
    auto* body=element("dialog-body");demand(body->GetScrollWidth()<=body->GetClientWidth()+1,"compact update panel has no horizontal overflow");
    capture("update-compact.tga");
    ui.close();
    const int requestsBeforePreview=requests;
    ui.updatePreview=true;ui.beginUpdateCheck();frameState.rom=L"C:/fixture/Jet Force Gemini.z64";FrontendUiFrame(frameState);settle();
    demand(requests==requestsBeforePreview && !ui.updatePending,"forced preview never queries GitHub");
    demand(element("update-notice")->IsVisible(true),"forced update banner visible");
    auto* pill=element("update-notice");auto pos=pill->GetAbsoluteOffset(Rml::BoxArea::Border);
    demand(pos.x>=0 && pos.x+pill->GetClientWidth()<=surface->width,"compact banner stays in window");
    pointerClick("update-notes");demand(lastAction=="project","preview Whats new uses main repository too");ui.modal("updates");settle();
    demand(element("dialog-body")->GetInnerRML().find("sample update")!=std::string::npos,"preview clearly labels simulated update");
    capture("forced-update-details.tga");ui.close();
    surface->context->SetDensityIndependentPixelRatio(1.5f);settle();
    // A physically larger client represents an ordinary 150% DPI launcher.
    SetWindowPos(window,nullptr,-30000,-30000,1280,780,SWP_NOZORDER|SWP_NOACTIVATE);FrontendUiResize();
    surface->context->SetDensityIndependentPixelRatio(1.5f);settle();
    pointerClick("update-notes");demand(lastAction=="project","DPI layout preserves repository action");ui.close();capture("update-150-percent.tga");
    auto bannerBottom=element("update-notice")->GetAbsoluteOffset(Rml::BoxArea::Border).y+element("update-notice")->GetClientHeight();
    demand(bannerBottom<element("status")->GetAbsoluteOffset(Rml::BoxArea::Border).y,"imported-ROM banner fits above footer at 150 percent DPI");
    ui.updatePreview=false;ui.beginUpdateCheck();result("current\nv1.2.0\nNo newer stable release is available.\n");
    demand(!element("update-notice")->IsVisible(true),"normal result removes forced state when preview disabled");
    std::ofstream(home/"result.json")<<"{\"passed\":true,\"checks\":"<<checks<<",\"background_requests\":"<<requests<<"}";
    FrontendUiShutdown();ready=false;DestroyWindow(window);return 0;
  } catch(const std::exception& error) {
    std::ofstream(home/"error.txt")<<error.what();if(ready)FrontendUiShutdown();if(window)DestroyWindow(window);return 1;
  }
}
