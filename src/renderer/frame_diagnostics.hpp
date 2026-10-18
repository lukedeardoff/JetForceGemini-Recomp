#pragma once
#include "jfg/renderer/frame_diagnostics_state.hpp"
#include "common/rt64_plume.h"
#include "render/rt64_descriptor_sets.h"
#include "render/rt64_shader_library.h"
#include "shared/rt64_video_interface.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <memory>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#if defined(_WIN32)
#include <Windows.h>
#endif
namespace jfg {
// Swapchain diagnostics only: never draw into guest feedback or block the render
// thread on disk I/O. One producer, one writer, bounded queue and rotated files.
#if defined(_WIN32)
class FrameDiagnostics : public FramePresentationObserver {
    struct Sample { std::uint64_t us, frame, interval, scenes, age, vi, dropped; };
    static constexpr std::size_t capacity = 8192;
    std::array<Sample, capacity> queue{};
    std::atomic<std::size_t> head{0}, tail{0};
    std::atomic<bool> stopping{false}, failed{false}, accepting{true};
    std::atomic<unsigned> producing{0};
    std::thread writer;
    std::filesystem::path directory;
    std::string stem, settings;
    HWND window = nullptr;
    bool enabled = false, key_down = false, overlay = true;
    std::uint64_t start = 0, last = 0, frame = 0, dropped = 0;
    std::uint64_t window_start = 0, window_frames = 0, window_scenes = 0, max_us = 0;
    double fps = 0, scene_fps = 0, ms = 0, maximum_ms = 0;
    bool generated_output = false;
    static constexpr unsigned panel_width = 168, panel_height = 53, row_pitch = 768;
    std::unique_ptr<RenderTexture> panel_texture;
    std::unique_ptr<RenderBuffer> panel_upload;
    std::unique_ptr<RT64::VideoInterfaceDescriptorSet> panel_descriptor;
    std::array<std::array<char,64>,5> cached_lines{};
    bool panel_valid = false;
    static std::array<unsigned,7> glyph(char c) noexcept {
        switch(c) {
        case '0':return {14,17,19,21,25,17,14}; case '1':return {4,12,4,4,4,4,14};
        case '2':return {14,17,1,2,4,8,31}; case '3':return {30,1,1,14,1,1,30};
        case '4':return {2,6,10,18,31,2,2}; case '5':return {31,16,16,30,1,1,30};
        case '6':return {14,16,16,30,17,17,14}; case '7':return {31,1,2,4,8,8,8};
        case '8':return {14,17,17,14,17,17,14}; case '9':return {14,17,17,15,1,1,14};
        case 'A':return {14,17,17,31,17,17,17}; case 'E':return {31,16,16,30,16,16,31};
        case 'B':return {30,17,17,30,17,17,30}; case 'C':return {14,17,16,16,16,17,14};
        case 'D':return {30,17,17,17,17,17,30}; case 'H':return {17,17,17,31,17,17,17};
        case 'U':return {17,17,17,17,17,17,14}; case '/':return {1,1,2,4,8,16,16};
        case 'F':return {31,16,16,30,16,16,16}; case 'G':return {14,17,16,23,17,17,15};
        case 'I':return {14,4,4,4,4,4,14}; case 'L':return {16,16,16,16,16,16,31};
        case 'M':return {17,27,21,21,17,17,17}; case 'N':return {17,25,25,21,19,19,17};
        case 'O':return {14,17,17,17,17,17,14}; case 'P':return {30,17,17,30,16,16,16};
        case 'R':return {30,17,17,30,20,18,17}; case 'S':return {15,16,16,14,1,1,30};
        case 'T':return {31,4,4,4,4,4,4}; case 'X':return {17,17,10,4,10,17,17};
        case '.':return {0,0,0,0,0,12,12}; case '-':return {0,0,0,31,0,0,0};
        default:return {};
        }
    }
    void write_loop() noexcept {
        try {
            std::ofstream file;
            std::size_t bytes = 0, segment = 0;
            std::uint64_t flushed = frame_clock_us();
            while (!stopping.load(std::memory_order_acquire) ||
                   tail.load(std::memory_order_relaxed) != head.load(std::memory_order_acquire)) {
                auto read = tail.load(std::memory_order_relaxed);
                const auto end = head.load(std::memory_order_acquire);
                while (read != end) {
                    const auto sample = queue[read];
                    read = (read + 1) % capacity;
                    tail.store(read, std::memory_order_release);
                    if (!file.is_open() || bytes >= 16U*1024U*1024U) {
                        if (file.is_open()) { file.close(); ++segment; }
                        std::filesystem::create_directories(directory);
                        file.open(directory/(stem+"-"+std::to_string(segment%4)+".csv"),std::ios::binary|std::ios::trunc);
                        if (!file) { failed.store(true); return; }
                        file << "# JFG frame diagnostics schema 2; successful present-return intervals, not monitor scanout\n"
                             << "# SCENES counts distinct screen-scene VI ticks; extra passes share one tick; excludes 64x64 shadows\n"
                             << "# " << settings << "\n"
                             << "# Four rotating 16 MiB segments; queue overflow counted, never blocks rendering\n"
                             << "elapsed_us,render_frame,interval_us,scene_frames,scene_age_us,vi,queue_dropped\n";
                        bytes=512;
                    }
                    char line[256]{};
                    const int count=std::snprintf(line,sizeof(line),"%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
                        static_cast<unsigned long long>(sample.us),static_cast<unsigned long long>(sample.frame),
                        static_cast<unsigned long long>(sample.interval),static_cast<unsigned long long>(sample.scenes),
                        static_cast<unsigned long long>(sample.age),static_cast<unsigned long long>(sample.vi),
                        static_cast<unsigned long long>(sample.dropped));
                    if (count>0 && count<static_cast<int>(sizeof(line))) { file.write(line,count); bytes+=static_cast<std::size_t>(count); }
                    if (!file) { failed.store(true); return; }
                }
                const auto now=frame_clock_us();
                if (file.is_open() && now-flushed>=1000000) { file.flush(); flushed=now; }
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
            if(file.is_open()) {
                file<<"# completed queue_dropped="<<dropped<<" writer_failed="<<unsigned(failed.load())<<'\n';
                file.flush();
                if(!file) failed.store(true);
            }
        } catch (...) { failed.store(true); }
    }
public:
    ~FrameDiagnostics() { finish(); }
    void finish() noexcept override {
        FramePresentationObserver* expected=this;
        frame_diagnostic_state().observer.compare_exchange_strong(expected,nullptr);
        accepting.store(false);
        // A callback may already have loaded the observer. Keep this object
        // alive, reject late callbacks, and wait for its bounded enqueue before
        // telling the writer that no more samples can arrive.
        while(producing.load()!=0U) std::this_thread::yield();
        stopping.store(true,std::memory_order_release);
        if(writer.joinable()) writer.join();
    }
    void initialize(void* native_window, unsigned cap, bool unlimited, bool vsync) {
        window=static_cast<HWND>(native_window);
        generated_output=cap!=0 || unlimited;
        wchar_t* value=nullptr; std::size_t length=0;
        (void)_wdupenv_s(&value,&length,L"JFG_VIDEO_CONFIG");
        auto profile=value ? std::filesystem::path(value).parent_path() : std::filesystem::current_path();
        std::free(value);
        directory=profile/L"performance";
        std::ifstream preferences(profile/L"performance.ini");std::string line;
        while(std::getline(preferences,line)) { if(!line.empty() && line.back()=='\r')line.pop_back(); if(line=="enabled=1")enabled=true; if(line=="overlay=0")overlay=false; }
        SYSTEMTIME utc{};GetSystemTime(&utc);char name[96]{};
        std::snprintf(name,sizeof(name),"frames-%04u%02u%02u-%02u%02u%02u-%lu",static_cast<unsigned>(utc.wYear),static_cast<unsigned>(utc.wMonth),static_cast<unsigned>(utc.wDay),static_cast<unsigned>(utc.wHour),static_cast<unsigned>(utc.wMinute),static_cast<unsigned>(utc.wSecond),GetCurrentProcessId());
        stem=name;settings="mode="+std::string(unlimited?"unlimited":cap?"custom":"original")+" target="+std::to_string(cap)+" vsync="+(vsync?"1":"0");
        settings += std::string(" overlay=")+(overlay?"1":"0")+" overlay_renderer=cached-texture";
        start=frame_clock_us();
        auto& state=frame_diagnostic_state();state.scenes=0;state.scene_time=0;state.vi=0;
        writer=std::thread([this]{write_loop();});
        state.observer.store(this,std::memory_order_release);
    }
    // RT64 destroys the present queue (and waits for its GPU work) before this
    // hook. Release descriptors/resources while its device still exists.
    void release_graphics() noexcept {
        panel_descriptor.reset(); panel_upload.reset(); panel_texture.reset(); panel_valid=false;
    }
    // Sample after the frame limiter and successful swapchain submission.
    // Render-hook work can happen early or late within a paced interval.
    void presented(std::uint64_t now) noexcept override {
        if(!accepting.load()) return;
        producing.fetch_add(1U);
        struct ProducerGuard {
            std::atomic<unsigned>& count;
            ~ProducerGuard(){count.fetch_sub(1U);}
        } active{producing};
        if(!accepting.load() || !enabled)return;
        const auto interval=last?now-last:0;last=now;++frame;
        auto& state=frame_diagnostic_state();const auto scenes=state.scenes.load(std::memory_order_relaxed);
        const auto scene_time=state.scene_time.load(std::memory_order_relaxed);
        if(!window_start){window_start=now;window_scenes=scenes;}
        else ++window_frames;
        max_us=(std::max)(max_us,interval);
        if(now-window_start>=500000){
            const double seconds=static_cast<double>(now-window_start)/1000000.;
            fps=static_cast<double>(window_frames)/seconds;scene_fps=static_cast<double>(scenes-window_scenes)/seconds;
            ms=window_frames?static_cast<double>(now-window_start)/static_cast<double>(window_frames)/1000.:0;
            maximum_ms=static_cast<double>(max_us)/1000.;window_start=now;window_frames=0;window_scenes=scenes;max_us=0;
        }
        const auto write=head.load(std::memory_order_relaxed),next=(write+1)%capacity;
        if(next!=tail.load(std::memory_order_acquire)){
            queue[write]={now-start,frame,interval,scenes,scene_time && now>=scene_time?now-scene_time:0,state.vi.load(std::memory_order_relaxed),dropped};
            head.store(next,std::memory_order_release);
        }else ++dropped;
    }
    void draw(RenderCommandList* list,unsigned width,unsigned height,
              RenderDevice* device,const RT64::ShaderLibrary* shaders) {
        if(!list || width<100 || height<80)return;
        const bool focused=window && GetAncestor(window,GA_ROOT)==GetForegroundWindow();
        const bool down=focused && (GetAsyncKeyState(VK_F8)&0x8000)!=0;
        if(down && !key_down){ enabled=!enabled; last=0;window_start=0;window_frames=0;max_us=0; }
        key_down=down;
        if(!enabled)return;
        if(!overlay)return;
        if(!device || !shaders || width<200 || height<140)return;
        std::array<std::array<char,64>,5> lines{};
        std::snprintf(lines[0].data(),lines[0].size(),"OUTPUT %.1f FPS",fps);
        std::snprintf(lines[1].data(),lines[1].size(),"SCENES %.1f FPS",scene_fps);
        std::snprintf(lines[2].data(),lines[2].size(),"MS %.1f MAX %.1f",ms,maximum_ms);
        std::snprintf(lines[3].data(),lines[3].size(),"%s",generated_output?"INTERPOLATED / REPEATED":"ORIGINAL PRESENTATION");
        std::snprintf(lines[4].data(),lines[4].size(),"LOG %s  F8 TOGGLE",failed.load()?"ERROR":dropped?"GAPS":"ON");
        if(!panel_texture) {
            panel_texture=device->createTexture(RenderTextureDesc::Texture2D(panel_width,panel_height,1,RenderFormat::R8G8B8A8_UNORM));
            panel_upload=device->createBuffer(RenderBufferDesc::UploadBuffer(row_pitch*panel_height));
            panel_descriptor=std::make_unique<RT64::VideoInterfaceDescriptorSet>(shaders->samplerLibrary.nearest.borderBorder.get(),device);
            if(!panel_texture || !panel_upload || !panel_descriptor) { release_graphics();return; }
            panel_descriptor->setTexture(panel_descriptor->gInput,panel_texture.get(),RenderTextureLayout::SHADER_READ);
        }
        if(!panel_valid || lines!=cached_lines) {
            // The present worker waits for the preceding frame's GPU fence
            // before invoking this hook again, so reusing the upload is safe.
            auto* pixels=static_cast<std::uint32_t*>(panel_upload->map());
            if(!pixels)return;
            std::fill_n(pixels,(row_pitch/4)*panel_height,0xFF0D0805U);
            for(std::size_t row=0;row<lines.size();++row) {
                unsigned x=4;const unsigned y=4+static_cast<unsigned>(row)*9;
                for(char c:lines[row]) {
                    if(!c || x+5>=panel_width)break;
                    const auto bitmap=glyph(c);
                    for(unsigned dy=0;dy<7;++dy)for(unsigned dx=0;dx<5;++dx)
                        if(bitmap[dy]&(1U<<(4-dx)))pixels[(y+dy)*(row_pitch/4)+x+dx]=0xFFD9F2F2U;
                    x+=6;
                }
            }
            panel_upload->unmap();
            list->barriers(RenderBarrierStage::COPY,RenderTextureBarrier(panel_texture.get(),RenderTextureLayout::COPY_DEST));
            list->copyTextureRegion(RenderTextureCopyLocation::Subresource(panel_texture.get()),
                RenderTextureCopyLocation::PlacedFootprint(panel_upload.get(),RenderFormat::R8G8B8A8_UNORM,panel_width,panel_height,1,row_pitch/4));
            list->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(panel_texture.get(),RenderTextureLayout::SHADER_READ));
            cached_lines=lines;panel_valid=true;
        }
        const unsigned scale=width>=480?2U:1U;
        const int left=static_cast<int>(width-panel_width*scale-8);
        list->setViewports(RenderViewport(static_cast<float>(left),8.f,static_cast<float>(panel_width*scale),static_cast<float>(panel_height*scale)));
        list->setScissors(RenderRect(left,8,static_cast<int>(width)-8,static_cast<int>(8+panel_height*scale)));
        const auto& shader=shaders->videoInterfaceNearest;
        interop::VideoInterfaceCB constants{};
        constants.videoResolution={float(panel_width),float(panel_height)};
        constants.textureResolution=constants.videoResolution;constants.gamma=1.f;
        list->setPipeline(shader.pipeline.get());
        list->setGraphicsPipelineLayout(shader.pipelineLayout.get());
        list->setGraphicsDescriptorSet(panel_descriptor->get(),0);
        list->setGraphicsPushConstants(0,&constants);
        list->setVertexBuffers(0,nullptr,0,nullptr);
        list->drawInstanced(3,1,0,0);
    }
};
#else
class FrameDiagnostics {
public:
    void initialize(void*, unsigned, bool, bool) {}
    void release_graphics() noexcept {}
    void draw(RenderCommandList*, unsigned, unsigned, RenderDevice*, const RT64::ShaderLibrary*) {}
};
#endif
} // namespace jfg
