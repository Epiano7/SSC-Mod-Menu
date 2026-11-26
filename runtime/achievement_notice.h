#pragma once
#include "achievement_audio.h"
#include "achievements.h"
#include "hud_editor.h"
#include <GL/gl.h>
namespace ssc_achievements {
inline void update_popup_bounds(int width,int height);
inline constexpr int notice_width=320,notice_height=68;
inline constexpr float max_display_scale=1.5f;
inline constexpr float max_notice_scale=max_display_scale*ssc_hud::popup_max_scale;
struct NoticeSize {int width,height;};
inline NoticeSize notice_size(int screen_width,int screen_height,float popup_scale=1.f){
 if(screen_width<120||screen_height<120)return {};
 const float display_scale=std::clamp(screen_height/1080.f,.75f,max_display_scale);
 const float scale=std::min({display_scale*popup_scale,(screen_width-24.f)/notice_width,float(screen_height)/notice_height});
 return {int(std::lround(notice_width*scale)),int(std::lround(notice_height*scale))};
}
inline void update_popup_bounds(int width,int height){
 auto& item=ssc_hud::items[ssc_hud::achievement_popup];
 item.scale=ssc_hud::bounded_scale(item);
 const auto size=notice_size(width,height,item.scale);
 if(size.width&&size.height){
  // HUD hit testing multiplies these base dimensions by the stored scale.
  item.w=float(size.width)/width/item.scale;
  item.h=float(size.height)/height/item.scale;
 }
 ssc_hud::constrain(item);
}
inline float notice_top(int height,uint64_t age,float anchor,int screen_height){
 constexpr float slide_ms=250.f;
 const float edge=anchor+height*.5f>=screen_height*.5f?float(screen_height):-float(height);
 const float elapsed=float(std::min(age,notice_lifetime_ms));
 const float entering=1.f-std::min(1.f,elapsed/slide_ms);
 const float leaving=1.f-std::min(1.f,(notice_lifetime_ms-elapsed)/slide_ms);
 const float hidden=std::max(entering,leaving);
 return std::round(anchor+(edge-anchor)*hidden*hidden*hidden);
}
inline constexpr int notice_raster_scale=2;
struct NoticeLayout {
 NoticeSize texture{},body{};
 RECT title{};
 int title_width=0;
};
struct NoticeRaster {
 NoticeLayout layout;
 std::vector<unsigned char> pixels;
};
inline float title_offset(const NoticeLayout& layout,uint64_t age){
 constexpr uint64_t delay_ms=1000,scroll_window_ms=4000;
 const int overflow=std::max(0,layout.title_width-int(layout.title.right-layout.title.left));
 if(!overflow||age<=delay_ms)return 0;
 const float elapsed=float(std::min(age-delay_ms,scroll_window_ms));
 const float speed=std::max(layout.body.width*70.f/(notice_width*1000.f),overflow/float(scroll_window_ms));
 return std::min(float(overflow),elapsed*speed);
}
inline COLORREF rarity_color(Rarity rarity){
 constexpr COLORREF colors[]={RGB(194,207,220),RGB(130,231,156),RGB(96,188,255),RGB(209,147,255),RGB(255,201,107),RGB(255,126,164)};
 return colors[size_t(rarity)];
}
inline NoticeRaster raster_notice(size_t item,bool preview,NoticeSize size={notice_width,notice_height}){
 if(item>=catalog.size()||size.width<1||size.height<1||
    size.width>notice_width*max_notice_scale||size.height>notice_height*max_notice_scale)return {};
 const NoticeSize body{size.width*notice_raster_scale,size.height*notice_raster_scale};
 const auto x=[body](int logical){return int(std::lround(logical*body.width/float(notice_width)));};
 const auto y=[body](int logical){return int(std::lround(logical*body.height/float(notice_height)));};
 const RECT title{x(16),y(25),body.width-x(14),y(61)};
 const int title_height=int(title.bottom-title.top);
 const int title_clip_width=int(title.right-title.left);
 if(title_height<1||title_clip_width<1)return {};

 std::string label=preview?"This does not unlock an achievement":catalog[item].name;
 if(!preview&&catalog[item].tier)label+=" / Tier "+std::to_string(catalog[item].tier);
 const std::wstring value(label.begin(),label.end());

 HDC dc=CreateCompatibleDC(nullptr);
 if(!dc)return {};
 auto make_font=[](int height){
  return CreateFontW(-std::max(1,height),0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
                     OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");
 };
 HFONT small=make_font(y(11)),large=make_font(y(18));
 if(!small||!large){DeleteObject(small);DeleteObject(large);DeleteDC(dc);return {};}
 auto original_font=SelectObject(dc,large);
 SIZE extent{};
 if(!GetTextExtentPoint32W(dc,value.data(),int(value.size()),&extent)||extent.cx>4096){
  SelectObject(dc,original_font);DeleteObject(small);DeleteObject(large);DeleteDC(dc);return {};
 }
 const int title_width=int(extent.cx);
 const NoticeSize texture{std::max(body.width,title_width),body.height+title_height};
 BITMAPINFO info{};
 info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
 info.bmiHeader.biWidth=texture.width;
 info.bmiHeader.biHeight=-texture.height;
 info.bmiHeader.biPlanes=1;
 info.bmiHeader.biBitCount=32;
 info.bmiHeader.biCompression=BI_RGB;
 void* bits=nullptr;
 HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
 if(!bitmap){SelectObject(dc,original_font);DeleteObject(small);DeleteObject(large);DeleteDC(dc);return {};}
 auto original_bitmap=SelectObject(dc,bitmap);
 auto* rgba=static_cast<unsigned char*>(bits);
 const size_t body_bytes=size_t(texture.width)*body.height*4;
 const size_t total_bytes=size_t(texture.width)*texture.height*4;
 std::memset(rgba,0,total_bytes);
 const int accent_width=std::max(1,x(3));
 const COLORREF accent_color=preview?RGB(60,208,255):rarity_color(catalog[item].rarity);
 for(int row=0;row<body.height;++row){
  auto* pixel=rgba+size_t(row)*texture.width*4;
  for(int column=0;column<body.width;++column,pixel+=4){
   const bool accent=column<accent_width;
   pixel[0]=accent?GetBValue(accent_color):55;pixel[1]=accent?GetGValue(accent_color):32;pixel[2]=accent?GetRValue(accent_color):16;
  }
 }
 SetBkMode(dc,TRANSPARENT);
 SelectObject(dc,small);
 SetTextColor(dc,RGB(74,211,255));
 const wchar_t* header=preview?L"NOTIFICATION PREVIEW":L"ACHIEVEMENT UNLOCKED";
 TextOutW(dc,x(16),y(10),header,lstrlenW(header));

 // One cached texture carries the card and a separate alpha-masked title strip.
 SelectObject(dc,large);
 SetTextColor(dc,RGB(255,255,255));
 RECT strip{0,body.height,title_width,texture.height};
 DrawTextW(dc,value.c_str(),int(value.size()),&strip,DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
 GdiFlush();
 for(int row=0;row<body.height;++row)
  for(int column=0;column<body.width;++column)rgba[(size_t(row)*texture.width+column)*4+3]=245;
 for(size_t i=body_bytes;i<total_bytes;i+=4){
  rgba[i+3]=rgba[i];rgba[i]=255;rgba[i+1]=245;rgba[i+2]=238;
 }
 NoticeRaster result{{texture,body,title,title_width},std::vector<unsigned char>(rgba,rgba+total_bytes)};
 SelectObject(dc,original_bitmap);DeleteObject(bitmap);
 SelectObject(dc,original_font);DeleteObject(small);DeleteObject(large);DeleteDC(dc);
 return result;
}
inline void notice_quad(float x,float y,float width,float height,float u0,float v0,float u1,float v1){
 glBegin(GL_QUADS);
 glTexCoord2f(u0,v0);glVertex2f(x,y);glTexCoord2f(u1,v0);glVertex2f(x+width,y);
 glTexCoord2f(u1,v1);glVertex2f(x+width,y+height);glTexCoord2f(u0,v1);glVertex2f(x,y+height);
 glEnd();
}
inline void draw_notice(int screen_width,int screen_height,uint64_t now,bool obscured){
 const bool editing=ssc_hud::editing;
 const Notice sample{0,now,true};
 if(obscured)return;
 auto* queued=editing?nullptr:engine.notice(now);
 const auto* notice=editing?&sample:queued;if(!notice)return;
 const auto& anchor=ssc_hud::items[ssc_hud::achievement_popup];
 const auto size=notice_size(screen_width,screen_height,anchor.scale);if(!size.width||!size.height)return;
 static HGLRC context=nullptr;auto current=wglGetCurrentContext();if(!current)return;
 static GLuint texture=0;if(context!=current){context=current;texture=0;}
 static size_t cached=catalog.size();static bool preview=false;static NoticeSize cached_size{};
 static NoticeLayout layout{};
 if(!texture||cached!=notice->item||preview!=notice->preview||cached_size.width!=size.width||cached_size.height!=size.height){
  auto raster=raster_notice(notice->item,notice->preview,size);if(raster.pixels.empty())return;
  cached=notice->item;preview=notice->preview;cached_size=size;layout=raster.layout;
  if(!texture)glGenTextures(1,&texture);
  glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812F);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812F);
  glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,layout.texture.width,layout.texture.height,0,0x80E1,GL_UNSIGNED_BYTE,raster.pixels.data());
 }
 if(queued&&!queued->sound_played){queued->sound_played=true;audio::play_unlock();}
 const uint64_t age=now>=notice->started?now-notice->started:0;
 const float x=std::round(anchor.x*screen_width),top=std::round(anchor.y*screen_height);
 const float y=editing?top:notice_top(size.height,age,top,screen_height);
 const float inv_w=1.f/layout.texture.width,inv_h=1.f/layout.texture.height;
 glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);glColor4f(1,1,1,1);
 const float strip_top=layout.body.height*inv_h;
 notice_quad(x,y,float(size.width),float(size.height),0,0,layout.body.width*inv_w,strip_top);
 const int clipped_width=std::min(layout.title_width,int(layout.title.right-layout.title.left));
 const float offset=title_offset(layout,age);
 notice_quad(x+float(layout.title.left)/notice_raster_scale,y+float(layout.title.top)/notice_raster_scale,
             float(clipped_width)/notice_raster_scale,float(layout.title.bottom-layout.title.top)/notice_raster_scale,
             offset*inv_w,strip_top,(offset+clipped_width)*inv_w,1);
}
}
