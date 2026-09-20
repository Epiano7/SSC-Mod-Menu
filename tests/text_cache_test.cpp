#include "../runtime/overlay.cpp"
#include <cassert>
// Prior uncached production rasterizer, retained as the pixel-equivalence oracle.
void reference_text(int x,int y,const wchar_t* value,COLORREF color=ink,bool title=false,int custom_size=0,float rainbow_phase=-1) {
    int size=custom_size?custom_size:title?22:15;auto& f=face(title||custom_size!=0);
    if(f.atlas.empty()){SelectObject(canvas,title?title_font:font);SetTextColor(canvas,color);TextOutW(canvas,x,y,value,lstrlenW(value));return;}
    GdiFlush();float cursor=x*raster_scale,scale=size*raster_scale/f.cap_height;auto dest=static_cast<unsigned char*>(pixels);
    auto coverage=[&](float fx,float fy) {
        fx=std::max(0.f,std::min(float(f.w-1),fx));fy=std::max(0.f,std::min(float(f.h-1),fy));
        int ax=std::min(f.w-2,int(fx)),ay=std::min(f.h-2,int(fy));float tx=fx-ax,ty=fy-ay;
        auto a=[&](int xx,int yy){return f.atlas[(yy*f.w+xx)*2+1]/255.f;};
        return (a(ax,ay)*(1-tx)+a(ax+1,ay)*tx)*(1-ty)+(a(ax,ay+1)*(1-tx)+a(ax+1,ay+1)*tx)*ty;
    };
    for(const wchar_t* s=value;*s;++s) {
        if(*s==L' '){cursor+=size*.4f*raster_scale;continue;}
        int c=int(*s);if(c<f.start||c>f.end)c='?';if(c<f.start||c>f.end)continue;
        auto& g=f.glyphs[c-f.start];float gw=std::max(0.f,g.width)*f.w*scale,gh=g.height*f.h*scale;
        if(gw<=0)continue;
        float top=y*raster_scale-f.cap_top*scale,sx=g.u0*f.w,sy=g.v0*f.h;
        std::array<float,3> left{},right{};if(rainbow_phase>=0){float n=float(lstrlenW(value)),index=float(s-value);left=rainbow_rgb(index/n+rainbow_phase,.2f);right=rainbow_rgb((index+.75f)/n+rainbow_phase,.1f);}
        for(int dy=std::max(0,int(std::floor(top)));dy<std::min(raster_h,int(std::ceil(top+gh)));++dy)
            for(int dx=std::max(0,int(std::floor(cursor)));dx<std::min(raster_w,int(std::ceil(cursor+gw)));++dx) {
                float a=0;
                // Integrate the atlas footprint instead of taking one undersampled point.
                for(int oy=0;oy<4;++oy)for(int ox=0;ox<4;++ox){float px=dx+(ox+.5f)/4,py=dy+(oy+.5f)/4;
                    if(px>=cursor&&px<cursor+gw&&py>=top&&py<top+gh)a+=coverage(sx+(px-cursor)/scale-.5f,sy+(py-top)/scale-.5f)/16.f;}
                auto pixel=dest+(dy*raster_w+dx)*4;unsigned char rgb[]={GetBValue(color),GetGValue(color),GetRValue(color)};
                for(int k=0;k<3;++k){float channel=rgb[k];if(rainbow_phase>=0){float t=unit((dx+.5f-cursor)/gw);channel=255*(left[2-k]*(1-t)+right[2-k]*t);}pixel[k]=static_cast<unsigned char>(pixel[k]*(1-a)+channel*a);}
                pixel[3]=255;
            }
        cursor+=gw; // Preserve native advances; do not round each glyph or add tracking.
    }
}
void compare(int x,int y,const wchar_t* value,COLORREF color,bool heavy,int size,float phase){
    const size_t bytes=size_t(raster_w)*raster_h*4;
    GdiFlush();std::memset(pixels,37,bytes);reference_text(x,y,value,color,heavy,size,phase);
    std::vector<unsigned char> expected(static_cast<unsigned char*>(pixels),static_cast<unsigned char*>(pixels)+bytes);
    for(int pass=0;pass<2;++pass){std::memset(pixels,37,bytes);text(x,y,value,color,heavy,size,phase);assert(std::memcmp(pixels,expected.data(),bytes)==0);}
}
int main(int argc,char** argv){
    if(argc>1){
        auto dir=std::filesystem::path(argv[1])/"data/translations";
        assert(local_font.load(dir/"black128_60.glf"));assert(heavy_font.load(dir/"thick128_60.glf"));
    }else{
        local_font.w=local_font.h=64;local_font.start=32;local_font.end=126;local_font.cap_height=16;local_font.cap_top=2;
        local_font.atlas.resize(64*64*2);for(size_t i=0;i<local_font.atlas.size();++i)local_font.atlas[i]=unsigned(i*31%256);
        local_font.glyphs.assign(95,{.2f,.4f,.1f,.1f,.3f,.5f});heavy_font=local_font;
    }
    for(float scale:{.713f,.85f,1.f,1.15f}){
        raster_scale=scale;prepare_canvas();assert(pixels);
        for(bool heavy:{false,true})for(float phase:{-1.f,.17f,.78f}){
            compare(11,35,L"SSC Mod Menu 123!",RGB(89,210,130),heavy,15,phase);
            compare(-12,-2,L"Clipped text",RGB(255,190,30),heavy,22,phase);
            compare(1098,705,L"Edge",RGB(70,80,220),heavy,18,phase);
            compare(0,0,L"",ink,heavy,15,phase);
        }
    }
    // Color/animation changes reuse coverage, never cached colors.
    raster_scale=1;prepare_canvas();compare(10,30,L"Repeat",RGB(255,0,0),true,18,-1);
    const size_t count=text_cache.size();compare(10,30,L"Repeat",RGB(0,255,0),true,18,.23f);assert(text_cache.size()==count);
    // Reloading an atlas at the same address must invalidate old coverage.
    ++heavy_font.revision;for(size_t i=1;i<heavy_font.atlas.size();i+=2)heavy_font.atlas[i]=255-heavy_font.atlas[i];
    compare(10,30,L"Repeat",ink,true,18,-1);
    // Changing counters/search text cannot grow the cache indefinitely.
    for(int i=0;i<400;++i){auto label=L"Counter "+std::to_wstring(i);text(20,50,label.c_str());assert(text_cache.size()<=text_cache_entries&&text_cache_bytes<=text_cache_limit);}
    std::puts("PASS: text cache pixel equivalence, scaling, clipping, rainbow/color changes, font reload and bounded eviction");
}
