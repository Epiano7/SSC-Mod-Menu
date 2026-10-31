#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include "color_picker.h"
#include <GL/gl.h>
#include <bcrypt.h>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include "text_editor.h"
#include "sound_module.h"
#include "cosmetic_adapter.h"
#include "beta_auth.h"
#include "cosmetics_access.h"
#include "presence_source.h"
#include "weapon_lab.h"
#include "local_recorder.h"
#include "recorder_source.h"
#include "hud_editor.h"
#include "dark_flash.h"
#include "geri_mode.h"
#include "creator_controls.h"
#include "freecam.h"
#include "draft_advisor.h"
#include "vehicle_adapter.h"
#include "quick_chat.h"
#include "auto_messages_source.h"
#include "tracking_recording.h"
#include "draft_training_source.h"
#include "statistics_reader.h"
#include "postgame_report.h"
#include "postgame_history.h"
#include "update_module.h"
#include "diagnostics.h"

namespace {
bool native_supported=false;
bool presence_supported=false;
#include "menu_ui.h"

bool attach_sound() {
    auto base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    auto dir=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    const auto size=nt->OptionalHeader.SizeOfImage;
    auto module=GetModuleHandleW(L"openal32_x64.dll");
    auto expected=module?GetProcAddress(module,"alBufferData"):nullptr;if(!expected)return false;
    if(dir.VirtualAddress>=size||dir.Size>size-dir.VirtualAddress)return false;
    for(size_t offset=0;offset+sizeof(IMAGE_IMPORT_DESCRIPTOR)<=dir.Size;offset+=sizeof(IMAGE_IMPORT_DESCRIPTOR)){
        auto d=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base+dir.VirtualAddress+offset);if(!d->Name)break;
        if(d->Name>=size||!memchr(base+d->Name,0,size-d->Name))return false;
        if(_stricmp(reinterpret_cast<char*>(base+d->Name),"openal32_x64.dll"))continue;
        if(!d->OriginalFirstThunk)return false;
        for(size_t i=0;;++i){size_t nr=d->OriginalFirstThunk+i*8,ar=d->FirstThunk+i*8;if(nr+8>size||ar+8>size)return false;
            auto n=reinterpret_cast<IMAGE_THUNK_DATA64*>(base+nr),a=reinterpret_cast<IMAGE_THUNK_DATA64*>(base+ar);if(!n->u1.AddressOfData)break;
            if(IMAGE_SNAP_BY_ORDINAL64(n->u1.Ordinal))continue;
            auto rva=n->u1.AddressOfData;if(rva+2>=size||!memchr(base+rva+2,0,size-rva-2))return false;
            if(strcmp(reinterpret_cast<char*>(base+rva+2),"alBufferData"))continue;
            if(a->u1.Function!=reinterpret_cast<ULONGLONG>(expected))return false;
            DWORD protection;
            if(!VirtualProtect(&a->u1.Function,8,PAGE_READWRITE,&protection))return false;
            ssc_sound::original=reinterpret_cast<ssc_sound::BufferData>(expected);
            auto prior=InterlockedCompareExchangePointer(reinterpret_cast<PVOID volatile*>(&a->u1.Function),reinterpret_cast<PVOID>(ssc_sound::buffer_data),reinterpret_cast<PVOID>(expected));
            DWORD unused;VirtualProtect(&a->u1.Function,8,protection,&unused);return prior==reinterpret_cast<PVOID>(expected);
        }
    }return false;
}

template<class T> T gl_proc(const char* name) {
    auto p=wglGetProcAddress(name);auto v=reinterpret_cast<intptr_t>(p);
    return (v==0||v==1||v==2||v==3||v==-1)?nullptr:reinterpret_cast<T>(p);
}
void upload_canvas_texture() {
    static GLuint allocated_texture=0;
    static int allocated_w=0,allocated_h=0;
    if(allocated_texture!=texture||allocated_w!=raster_w||allocated_h!=raster_h){
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,raster_w,raster_h,0,0x80E1,GL_UNSIGNED_BYTE,pixels);
        allocated_texture=texture;allocated_w=raster_w;allocated_h=raster_h;
    }else glTexSubImage2D(GL_TEXTURE_2D,0,0,0,raster_w,raster_h,0x80E1,GL_UNSIGNED_BYTE,pixels);
}
void render(HDC dc) {
    ssc_perf::Scope timing(ssc_perf::overlay);
    HWND window=WindowFromDC(dc); HGLRC context=wglGetCurrentContext();
    if(!window||!context) return;
    if(!game_window) {
        if(GetWindowThreadProcessId(window,nullptr)!=GetCurrentThreadId()) {log("Window/render threads differ; input hook refused");return;}
        SetLastError(0);auto old=SetWindowLongPtrW(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(window_proc));
        if(!old) {log("Window input hook failed");return;}
        previous_proc=reinterpret_cast<WNDPROC>(old);game_window=window;log("Render and input attached; menu key configurable in Interface");
    }
    ULONGLONG now=GetTickCount64();float seconds=last_frame?float(now-last_frame)/1000.f:0.f;last_frame=now;
    static double precise_frame_at=0;const double precise_now=ssc_perf::clock_ms();const double frame_seconds=precise_frame_at?(precise_now-precise_frame_at)/1000.0:0;precise_frame_at=precise_now;
    static ULONGLONG performance_at=0;static unsigned performance_frames=0,slow_frames=0;static double frame_sum=0,frame_max=0;
    if(!ssc_record::state().enabled||!ssc_record::state().match_open){performance_at=now;performance_frames=slow_frames=0;frame_sum=frame_max=0;ssc_perf::milliseconds.fill(0);ssc_perf::calls.fill(0);}
    else if(frame_seconds>0){
      ++performance_frames;frame_sum+=frame_seconds;frame_max=std::max(frame_max,frame_seconds);if(frame_seconds>.033333)++slow_frames;
      if(now-performance_at>=10000){
        ssc_record::diagnostic({{"event","performance_sample"},{"mod_overlay_ms",ssc_perf::milliseconds[ssc_perf::overlay]},{"advisor_prepare_ms",ssc_perf::milliseconds[ssc_perf::advisor]},{"shared_lookup_ms",ssc_perf::milliseconds[ssc_perf::sharing]},{"timed_calls",ssc_perf::calls},{"frames",performance_frames},{"window_ms",now-performance_at},{"average_fps",performance_frames/frame_sum},{"mean_frame_ms",1000*frame_sum/performance_frames},{"max_frame_ms",1000*frame_max},{"frames_over_33ms",slow_frames},{"focused",GetForegroundWindow()==game_window},{"menu_open",opened},{"advisor_attached",ssc_advisor::attached},{"advisor_enabled",ssc_advisor::enabled},{"shared_receive",shared_receive},{"shared_publish",shared_publish}});
        performance_at=now;performance_frames=slow_frames=0;frame_sum=frame_max=0;ssc_perf::milliseconds.fill(0);ssc_perf::calls.fill(0);
      }
    }
    ssc_names::begin_render_frame(shared_receive);
    static ULONGLONG auth_start_at=0;
    if(now-auth_start_at>=1000){auth_start_at=now;ssc_auth::service().start();}
    static ULONGLONG shared_at=0;
    if(now-shared_at>=1000){shared_at=now;
      std::string own;if(ssc_names::attached)ssc_names::account_string(ssc_names::image_base+ssc_compat::resolve(0xe041e8),own);
      ssc_shared::Style appearance;appearance.mode=ssc_names::gradient?ssc_shared::Mode::gradient:ssc_names::rainbow?ssc_shared::Mode::rainbow:ssc_shared::Mode::solid;
      appearance.count=ssc_names::gradient?unsigned(std::clamp(ssc_names::gradient_count,2,3)):ssc_names::rainbow?0:1;
      appearance.colors=ssc_names::gradient?ssc_names::gradient_colors:std::array<unsigned,3>{ssc_names::solid_rgb,0,0};
      ssc_cosmetic_access::refresh(own,now);
      ssc_shared::Style permitted;const bool can_share=ssc_cosmetic_access::publication(ssc_cosmetic_access::current,appearance,ssc_names::solid_rgb,permitted);
      if(can_share)appearance=permitted;
      ssc_shared::service().update(own,shared_receive?ssc_names::shared_accounts():std::vector<std::string>{},appearance,shared_receive&&ssc_names::attached,shared_publish&&ssc_names::cosmetics.load()&&can_share);
      if(settings_page==22||cosmetics_popup)dirty=true;
    }
    ssc_names::refresh_shared_labels(now,shared_receive||ssc_names::render_private_active());
    static uint64_t private_cleared=0;auto cleared=ssc_auth::service().cleared();if(cleared!=private_cleared){private_cleared=cleared;ssc_auth::license::wipe(private_code);private_editing=false;dirty=true;}
    static auto private_last=ssc_auth::license::State::idle;auto private_state=ssc_auth::service().state();if(private_state!=private_last){private_last=private_state;if(settings_page==19)dirty=true;}
    static bool private_was_visible=false;
    bool private_visible=ssc_names::private_visible();
    if(private_visible!=private_was_visible){private_was_visible=private_visible;dirty=true;controls.clear();pressed=hovered=keyboard_focus=0;if(!private_visible&&settings_page==18)show_manager(-1);}
    static ULONGLONG diagnostics_at=0;
    if(developer_tools&&now-diagnostics_at>15000){diagnostics_at=now;char report[160];snprintf(report,sizeof(report),"Draw diagnostics: rainbow=%u audio=%u",ssc_names::cosmetic_draws.load(),ssc_sound::substituted.load());log(report);}
    ssc_auto::tick(now,ssc_record::state().enabled);
    const auto& pg=ssc_auto::tracking_observation;
    const bool pg_fresh=ssc_auto::tracking_at&&now>=ssc_auto::tracking_at&&now-ssc_auto::tracking_at<=500;
    ssc_postgame_live::state.observe(pg.session,pg_fresh&&pg.valid,pg.active,pg.ending,pg.preparing,pg.round,pg.last_round);
    if(postgame_visible()&&!ssc_postgame_live::state.eligible&&rpc_preview.phase!=1)close_menu(true);
    // Capture only when a fresh tracking sample exists; no profile scan each frame.
    static ULONGLONG review_sample_at=0;
    if(!ssc_record::state().enabled){ssc_review_context::state.reset();review_sample_at=0;}
    else if(pg_fresh&&pg.valid&&review_sample_at!=ssc_auto::tracking_at){
        review_sample_at=ssc_auto::tracking_at;
        ssc_review_context::Rating rating;
        auto& context=ssc_review_context::state;
        if(presence_supported&&(!context.decided||(context.start_observed&&!context.starting.player_id)||(context.start_observed&&!context.starting.ranked&&pg.recording_round_seconds>=0&&pg.recording_round_seconds<=2)||context.session!=pg.session||pg.round<context.last_round)&&pg.active&&pg.round==1){
            ssc_rpc::Snapshot snapshot;snapshot.phase=2;
            ssc_rpc::sample_rating(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),snapshot,&rating);
        }
        context.observe(pg.session,pg.round,pg.active,ssc_auto::round_tracker.complete,pg.recording_round_seconds,now,rating,ssc_record::state().dropped.load());
        if(context.decided&&!context.emitted&&ssc_record::state().match_open){
            ssc_record::diagnostic({{"event","post_game_context"},{"data",context.json()}});
            context.emitted=true;
        }
    }
    // Open the asynchronous recording before the first build observation;
    // leave round/match closure until after its final snapshot is appended.
    if(pg.active)ssc_record::track_observation(now);
    ssc_training::tick(now,state_dir);

    if(postgame_history_pending&&!ssc_review_history::busy.load()){
        postgame_history_pending=false;
        auto saved=std::atomic_load(&ssc_review_history::result);
        if(postgame_visible()&&rpc_preview.phase==1&&postgame_history_revision==ssc_postgame_live::state.revision){
            postgame_history_error=saved?ssc_chat::widen(saved->error):L"Could not load saved match.";
            if(saved&&saved->error.empty()){
                ssc_postgame_art::images=saved->icons;ssc_postgame_art::states.clear();
                ssc_postgame_live::state.rounds.clear();ssc_postgame_live::state.bytes=0;
                for(const auto& round:saved->rounds)ssc_postgame_live::state.capture(round);
                postgame_round=std::max(0,int(saved->rounds.size())-1);postgame_build_step=-1;
                postgame_saved=true;if(ssc_review_online::requested)ssc_review_online::client().load(ssc_postgame_live::state.match_id);
            }dirty=true;
        }
    }
    static unsigned analysis_revision=0;static std::string analysis_match_seen;auto analysis_now=ssc_review_online::revision.load();if(analysis_now!=analysis_revision||analysis_match_seen!=ssc_postgame_live::state.match_id){analysis_revision=analysis_now;analysis_match_seen=ssc_postgame_live::state.match_id;if(ssc_review_online::requested||analysis_now){auto answer=ssc_review_online::client().snapshot(analysis_match_seen);postgame_analysis_match=answer->match;postgame_analysis_status=answer->status;postgame_analysis_message=answer->message;postgame_analysis=answer->report;}else{postgame_analysis=nullptr;postgame_analysis_match.clear();}if(postgame_visible())dirty=true;}
    if(postgame_visible()&&animations&&postgame_score_started&&now>=postgame_score_started&&now-postgame_score_started<1000)dirty=true;
    // Live advisor retired; recording collection remains separately gated.
    if(rpc_preview.phase==1)ssc_freecam::stop();
    bool was_freecam=ssc_freecam::state.active;auto pending_freecam=ssc_freecam::pending_since;
    ssc_freecam::tick(now,opened,game_window,ssc_freecam::frame_seconds());
    if(!was_freecam&&ssc_freecam::state.active)finish_freecam_entry();
    if(pending_freecam!=ssc_freecam::pending_since)dirty=true;
    ssc_vehicle::blocked=opened||ssc_freecam::state.active||GetForegroundWindow()!=game_window;
    if(!pg.active)ssc_record::track_observation(now);
    static ULONGLONG presence_at=0;
    if(now-presence_at>=1000){presence_at=now;rpc_preview=presence_supported?ssc_rpc::sample_steam(rpc_rating):ssc_rpc::Snapshot{};static int source_phase=-1;if(source_phase!=rpc_preview.phase){source_phase=rpc_preview.phase;log(source_phase==0?"RPC source: game status unavailable":source_phase==1?"RPC source: verified main menu":"RPC source: verified game session");}ssc_rpc::submit(rpc_requested&&presence_supported,rpc_id,rpc_timer,rpc_preview);refresh_dynamic_panel(now,true);if(ssc_record::state().enabled){auto observed=presence_supported?ssc_rpc::native_fallback(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))):ssc_rpc::Snapshot{};ssc_record::sample(observed.phase,observed.details,observed.state,now,observed.phase==2?ssc_record::sample_local_build(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))):ssc_record::Json(nullptr));if(opened&&manager&&settings_page==12)dirty=true;}}
    static ULONGLONG update_at=0;
    if(now-update_at>=1000){update_at=now;
        if(presence_supported&&rpc_preview.phase==1&&!welcome_seen&&!opened){opened=true;show_manager(8);}
        if(presence_supported&&rpc_preview.phase==1&&!opened&&ssc_cosmetic_access::account_id&&ssc_cosmetic_access::tier(ssc_cosmetic_access::current)!=ssc_cosmetic_access::Tier::checking&&!cosmetics_policy.known(ssc_cosmetic_access::account_id)){bool show=cosmetics_policy.consume(ssc_cosmetic_access::account_id);save();if(show){cosmetics_notice_shown=true;show_cosmetics_notice();}}
        if(update_notes_due(rpc_preview.phase)){opened=true;show_manager(14);}
        if(!ssc_update::checked)ssc_update::check(state_dir);
        if(ssc_update::poll(window,rpc_preview.phase==1||!presence_supported))dirty=true;
        if(rpc_preview.phase==2&&opened&&(update_popup||cosmetics_popup||(manager&&settings_page==7)))close_menu(true);
        if(ssc_update::notification_due(!presence_supported||rpc_preview.phase!=2)&&!opened){ssc_update::notified=true;manager=false;update_popup=true;opened=true;dirty=true;}
    }
    RECT hud_client{};GetClientRect(window,&hud_client);if(ssc_hud::finish_frame(hud_client.right,hud_client.bottom,now)&&ssc_hud::editing)dirty=true;
    if(opened&&manager&&settings_page==11&&ssc_lab::duel_animating()){ssc_lab::advance_duel(seconds);static ULONGLONG duel_paint_at=0;if(now-duel_paint_at>=16||!ssc_lab::duel_animating()){duel_paint_at=now;dirty=true;}}
    poll_live_hud_drag(window);
    advance_animation(seconds);
    bool freecam_notice=now<ssc_freecam::notice_until;
    bool advisor_border=ssc_advisor::border_ready()||ssc_advisor::stim_ready();
    if(!advisor_border||opened){ssc_advisor::border.pending=false;ssc_advisor::stim_border.pending=false;}
    if(!freecam_notice&&!opened&&visibility==0.f&&ssc_freecam::state.active&&(ssc_freecam::hide_hud||ssc_freecam::hide_all_gui))return;
    if(window!=game_window||(!opened&&visibility==0.f&&modal_visibility==0.f&&!clock_enabled&&!freecam_notice&&!advisor_border))return;
    RECT client;GetClientRect(window,&client);int width=client.right,height=client.bottom;
    if(width<=0||height<=0)return;
    layout_panel(width,height);
    if(!owner_context) owner_context=context;
    if(context!=owner_context) return; // Never reuse texture IDs across GL contexts.
    using UseProgram=void(APIENTRY*)(GLuint);
    using ActiveTexture=void(APIENTRY*)(GLenum);
    using BindFramebuffer=void(APIENTRY*)(GLenum,GLuint);
    using BindBuffer=void(APIENTRY*)(GLenum,GLuint);
    auto use_program=gl_proc<UseProgram>("glUseProgram");
    auto active_texture=gl_proc<ActiveTexture>("glActiveTexture");
    auto bind_framebuffer=gl_proc<BindFramebuffer>("glBindFramebuffer");
    auto bind_buffer=gl_proc<BindBuffer>("glBindBuffer");
    GLint unpack_buffer=0;
    if(bind_buffer) glGetIntegerv(0x88EF,&unpack_buffer);
    GLint program=0,active=0,framebuffer=0,mode=0;
    glGetIntegerv(0x8B8D,&program);glGetIntegerv(0x84E0,&active);glGetIntegerv(GL_MATRIX_MODE,&mode);
    if(bind_framebuffer) glGetIntegerv(0x8CA6,&framebuffer);
    glPushAttrib(GL_ALL_ATTRIB_BITS);glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    if(use_program) use_program(0);
    if(bind_buffer) bind_buffer(0x88EC,0);
    if(bind_framebuffer) bind_framebuffer(0x8CA9,0); // Draw framebuffer only; do not disturb read binding.
    if(active_texture) {
        GLint units=1;glGetIntegerv(0x84E2,&units);
        for(int i=0;i<units;++i) {active_texture(0x84C0+i);glDisable(GL_TEXTURE_2D);glDisable(GL_TEXTURE_1D);}
        active_texture(0x84C0);
    }
    glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);
    using BlendEquation=void(APIENTRY*)(GLenum);
    if(auto eq=gl_proc<BlendEquation>("glBlendEquation"))eq(0x8006);
    glDisable(GL_COLOR_LOGIC_OP);
    for(int i=0;i<6;++i)glDisable(GL_CLIP_PLANE0+i);
    glDisable(GL_TEXTURE_GEN_S);glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_ALPHA_TEST);glDisable(GL_LIGHTING);glDisable(GL_CULL_FACE);glDisable(GL_FOG);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glViewport(0,0,width,height);glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();glMatrixMode(GL_TEXTURE);glPushMatrix();glLoadIdentity();
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    ssc_advisor::render_border();ssc_advisor::render_stim(width,height);
    const float opacity=ease(visibility);
    if(modal_visibility>0.f&&!ssc_hud::editing) {
        glDisable(GL_TEXTURE_2D);glColor4f(.01f,.03f,.07f,.45f*ease(modal_visibility));
        glBegin(GL_QUADS);glVertex2i(0,0);glVertex2i(width,0);glVertex2i(width,height);glVertex2i(0,height);glEnd();
    }
    if(ssc_hud::editing){
        glDisable(GL_TEXTURE_2D);glLineWidth(1);
        for(int i=0;i<int(ssc_hud::items.size());++i)if(ssc_hud::visible(i,now)){
            const auto& a=ssc_hud::items[i];float x=a.x*width,y=a.y*height,w=a.w*a.scale*width,h=a.h*a.scale*height;
            if(i==ssc_hud::selected)glColor4f(.2f,.85f,1.f,1.f);else glColor4f(.7f,.8f,.9f,.65f);
            glBegin(GL_LINE_LOOP);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();
            glBegin(GL_QUADS);glVertex2f(x+w-5,y+h-5);glVertex2f(x+w+5,y+h-5);glVertex2f(x+w+5,y+h+5);glVertex2f(x+w-5,y+h+5);glEnd();
        }
    }
    if(opened||visibility>0.f||clock_enabled||freecam_notice) {
    if(!texture){glGenTextures(1,&texture);dirty=true;}
    glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812F);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812F);
    glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
    bool menu_visible=opened||visibility>0.f;static bool was_menu=true;static int clock_second=-1;
    if(menu_visible!=was_menu){dirty=true;was_menu=menu_visible;}
    refresh_dynamic_panel(now);
    if(menu_visible) {if(dirty){if(ssc_hud::editing)paint_live_hud_toolbar();else paint_panel();upload_canvas_texture();dirty=false;}}
    else {
        SYSTEMTIME time;GetLocalTime(&time);
        if(dirty||clock_second!=time.wSecond) {
            clock_second=time.wSecond;prepare_canvas();std::memset(pixels,0,raster_w*raster_h*4);
            panel_w=freecam_notice?640:310;panel_h=42;rectangle(0,0,panel_w,panel_h,RGB(8,21,41));
            wchar_t value[80];swprintf(value,80,L"TEST CLOCK  %02u:%02u:%02u",time.wHour,time.wMinute,time.wSecond);
            text(12,13,freecam_notice?ssc_freecam::entry_status:value,freecam_notice?RGB(255,190,120):ink,false,14);finish_canvas();upload_canvas_texture();
        }
    }
    float x=menu_visible?float(panel_x):24.f,y=menu_visible?float(panel_y):24.f;
    float w=menu_visible?std::round(panel_w*draw_scale):(freecam_notice?640.f:310.f),h=menu_visible?std::round(panel_h*draw_scale):42.f;
    float u=std::round((menu_visible?panel_w:(freecam_notice?640.f:310.f))*raster_scale)/raster_w,v=std::round((menu_visible?panel_h:42.f)*raster_scale)/raster_h;
    glColor4f(1,1,1,menu_visible?opacity:1.f);
    glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(x,y);glTexCoord2f(u,0);glVertex2f(x+w,y);
    glTexCoord2f(u,v);glVertex2f(x+w,y+h);glTexCoord2f(0,v);glVertex2f(x,y+h);glEnd();
    }
    glMatrixMode(GL_TEXTURE);glPopMatrix();glMatrixMode(GL_MODELVIEW);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(mode);
    glPopClientAttrib();glPopAttrib();
    if(bind_buffer) bind_buffer(0x88EC,unpack_buffer);
    if(bind_framebuffer) bind_framebuffer(0x8CA9,framebuffer);
    if(use_program) use_program(program);
    if(active_texture) active_texture(active);
}
BOOL WINAPI swap_hook(HDC dc) {
    if(ssc_freecam::capture::session.active()&&WindowFromDC(dc)==game_window){
        bool valid=!opened&&ssc_freecam::state.active&&ssc_freecam::singleplayer()&&!ssc_freecam::focus_suspended&&ssc_freecam::state.world==ssc_freecam::capture::session.world&&ssc_freecam::state.actor==ssc_freecam::capture::session.actor;
        if(!valid)ssc_freecam::capture::session.cancel();
        else try {if(!ssc_freecam::capture::session.read_frame(state_dir/L"captures")&&!ssc_freecam::capture::session.active())ssc_freecam::reject(GetTickCount64(),L"Capture interrupted. Try again with the menu closed.");}catch(...){ssc_freecam::capture::session.cancel();ssc_freecam::reject(GetTickCount64(),L"Capture failed. Try a smaller window.");}
    }
    int capture_result=ssc_freecam::capture::writer().result.exchange(0);
    if(capture_result)ssc_freecam::reject(GetTickCount64(),capture_result>0?L"Location capture saved in captures.":L"Capture could not be saved. Check disk space.");
    render(dc);return original_swap(dc);
}
void log_game_build() {
    wchar_t path[32768];if(!GetModuleFileNameW(nullptr,path,32768))return;
    std::ifstream in(std::filesystem::path(path),std::ios::binary);if(!in)return;
    BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;unsigned char digest[32];
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return;
    bool ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;char block[65536];size_t size=0;
    while(ok&&in){in.read(block,sizeof(block));auto n=in.gcount();size+=size_t(n);if(n)ok=BCryptHashData(hash,reinterpret_cast<PUCHAR>(block),ULONG(n),0)>=0;}
    if(ok)ok=BCryptFinishHash(hash,digest,32,0)>=0;
    if(hash)BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg,0);
    if(ok){char line[160];std::snprintf(line,sizeof(line),"Game bytes=%llu SHA256=",static_cast<unsigned long long>(size));std::string value=line;for(auto c:digest){char h[3];std::snprintf(h,3,"%02X",c);value+=h;}log(value.c_str());}
}
bool attach_swap() {
    auto base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || dos->e_lfanew>4096) return false;
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC) return false;
    auto size=nt->OptionalHeader.SizeOfImage;
    auto imports=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if(!imports.VirtualAddress || imports.VirtualAddress>=size || imports.Size>size-imports.VirtualAddress) return false;
    auto expected=GetProcAddress(GetModuleHandleW(L"gdi32.dll"),"SwapBuffers");
    for(DWORD offset=0;offset+sizeof(IMAGE_IMPORT_DESCRIPTOR)<=imports.Size;offset+=sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
        auto d=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base+imports.VirtualAddress+offset);
        if(!d->Name) break;
        if(d->Name>=size||!memchr(base+d->Name,0,size-d->Name)) return false;
        if(_stricmp(reinterpret_cast<char*>(base+d->Name),"GDI32.dll")) continue;
        if(!d->OriginalFirstThunk) return false;
        for(DWORD i=0;;++i) {
            size_t nr=d->OriginalFirstThunk+size_t(i)*8,ar=d->FirstThunk+size_t(i)*8;
            if(nr+8>size||ar+8>size) return false;
            auto name=reinterpret_cast<IMAGE_THUNK_DATA64*>(base+nr);auto address=reinterpret_cast<IMAGE_THUNK_DATA64*>(base+ar);
            if(!name->u1.AddressOfData) break;
            if(IMAGE_SNAP_BY_ORDINAL64(name->u1.Ordinal)) continue;
            size_t rva=name->u1.AddressOfData;
            if(rva+2>=size||!memchr(base+rva+2,0,size-rva-2)) return false;
            if(strcmp(reinterpret_cast<char*>(base+rva+2),"SwapBuffers")) continue;
            if(address->u1.Function!=reinterpret_cast<ULONGLONG>(expected)) {log("SwapBuffers already hooked; refusing conflict");return false;}
            DWORD old;if(!VirtualProtect(&address->u1.Function,8,PAGE_READWRITE,&old)) return false;
            original_swap=reinterpret_cast<Swap>(expected);
            auto prior=InterlockedCompareExchangePointer(reinterpret_cast<PVOID volatile*>(&address->u1.Function),reinterpret_cast<PVOID>(swap_hook),reinterpret_cast<PVOID>(expected));
            DWORD unused;VirtualProtect(&address->u1.Function,8,old,&unused);
            return prior==reinterpret_cast<PVOID>(expected);
        }
    }
    return false;
}
}
extern "C" __declspec(dllexport) void WINAPI SscModInitialize() {
    wchar_t path[32768];DWORD n=GetEnvironmentVariableW(L"SSCMODS_STATE_DIR",path,32768);
    if(n&&n<32768) state_dir=path;
    else {n=GetEnvironmentVariableW(L"LOCALAPPDATA",path,32768);if(!n||n>=32768)return;state_dir=std::filesystem::path(path)/L"SkillshotCityMod";}
    std::error_code error;std::filesystem::create_directories(state_dir,error);if(error)return;
    ssc_diagnostics::initialize(state_dir);ssc_auth::service().configure(state_dir);ssc_shared::service().configure(state_dir);
    if(std::filesystem::exists(state_dir/L"runtime.log",error)&&std::filesystem::file_size(state_dir/L"runtime.log",error)>2*1024*1024){std::filesystem::remove(state_dir/L"runtime.previous.log",error);std::filesystem::rename(state_dir/L"runtime.log",state_dir/L"runtime.previous.log",error);}
    ssc_dark_flash::diagnostic=log;
    log("SSC Mod Menu 0.1.31 startup");log_game_build();
    // Updates must remain available when native gameplay guards reject a build.
    ssc_update::check(state_dir);
    auto started=GetTickCount64();auto supported=ssc_compat::initialize(log);
    if(!supported){native_supported=false;log("Unsupported executable; runtime inactive");ssc_update::notify_without_overlay();return;}
    presence_supported=(supported&4)!=0;native_supported=true;
    log((supported&8)?"Weapon Lab definitions supported":"Weapon Lab unavailable on this game version");
    char result[128];std::snprintf(result,sizeof(result),"Compatibility: cosmetics=%d HUD=%d presence=%d scan=%llums",int((supported&1)!=0),int((supported&2)!=0),int(presence_supported),static_cast<unsigned long long>(GetTickCount64()-started));log(result);
    ssc_vehicle::attached=ssc_vehicle::attach();log(ssc_vehicle::attached?"Driving adapter attached":"Driving adapter unavailable");
    ssc_freecam::attached=ssc_freecam::attach();log(ssc_freecam::attached?"Freecam adapter attached":"Freecam adapter unavailable");
    load_settings();ssc_chat::attached=ssc_chat::attach();log(ssc_chat::attached?"Custom quick chat adapter attached":"Custom quick chat unavailable");
    ssc_names::cosmetics=cosmetic_requested&&ssc_compat::supports(1);
    ssc_combat::attached=ssc_combat::attach();log(ssc_combat::attached?"Combat event counters attached":"Combat event counters unavailable");
    ssc_names::attached=ssc_names::attach();ssc_names::validate_bot_layout();log(ssc_names::bot_layout?"Private module compatibility supported":"Private module unavailable: native compatibility check failed");log(ssc_names::attached?"Tab and overhead name adapters attached":"Name adapters unavailable");
    try {
        wchar_t executable[32768];DWORD length=GetModuleFileNameW(nullptr,executable,32768);
        if(length&&length<32768){ssc_sound::initialize(std::filesystem::path(executable).parent_path(),state_dir,sound_requested);ssc_sound::attached=attach_sound();log(ssc_music::attach()?"Music file adapter attached":"Music file adapter unavailable");log(ssc_sound::attached?"Audio buffer adapter attached (WAV effects / OGG music; restart applies changes)":"Audio adapter unavailable");}
    }catch(const std::exception&){log("Sound catalog unavailable; originals preserved");}
    ssc_advisor::attached=ssc_advisor::attach();log(ssc_advisor::attached?"Post-game recording adapter validated":"Post-game recording adapter unavailable");
    ssc_geri::attached=ssc_geri::attach();log(ssc_geri::attached?"Geri panel adapter attached":"Geri panel adapter unavailable");
    ssc_creator::attached=ssc_creator::attach();log(ssc_creator::attached?"Creator crosshair adapter attached":"Creator crosshair adapter unavailable");
    ssc_hud::diagnostics_draw=ssc_freecam::attached?reinterpret_cast<uintptr_t>(ssc_freecam::debug_draw):0;
    ssc_hud::attached=ssc_hud::attach();log(ssc_hud::attached?"HUD draw groups attached":"HUD adapter unavailable");
    log(attach_swap()?"Compatibility checks complete; SwapBuffers import attached":"SwapBuffers attachment refused");
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID) {if(reason==DLL_PROCESS_ATTACH)DisableThreadLibraryCalls(module);return TRUE;}


