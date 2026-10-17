#include "../runtime/freecam.h"
#include "freecam_session_fixture.h"
#include <cassert>
#include <iostream>
int main(int argc,char** argv){
 using namespace ssc_freecam;using namespace ssc_freecam::capture;
 assert(argc==2);std::filesystem::path root=argv[1];assert(!std::filesystem::exists(root));
 {FreecamSessionFixture fixture(false,1000);state.active=true;fly=true;camera.seeded=true;key(WM_KEYDOWN,capture_key,false);assert(!session.active());key(WM_KEYUP,capture_key,false);stop();}
 {FreecamSessionFixture fixture(true,1000);assert(request_start(1000));fly=true;camera.seeded=true;camera.eye={1,2,30};key(WM_KEYDOWN,capture_key,false);assert(session.active());suspend_focus();assert(!session.active());resume_focus();stop();key(WM_KEYUP,capture_key,false);}
 WNDCLASSW wc{};wc.style=CS_OWNDC;wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"SSCCaptureTest";assert(RegisterClassW(&wc));
 HWND window=CreateWindowW(wc.lpszClassName,L"Capture fixture",WS_OVERLAPPEDWINDOW,0,0,128,128,nullptr,nullptr,wc.hInstance,nullptr);assert(window);
 HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR descriptor{};descriptor.nSize=sizeof descriptor;descriptor.nVersion=1;descriptor.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;descriptor.iPixelType=PFD_TYPE_RGBA;descriptor.cColorBits=24;descriptor.cDepthBits=24;
 int format=ChoosePixelFormat(dc,&descriptor);assert(format&&SetPixelFormat(dc,format,&descriptor));HGLRC context=wglCreateContext(dc);assert(context&&wglMakeCurrent(dc,context));glViewport(0,0,16,16);
 // Clean view collapses label geometry in both camera modes, and the native
 // matrix pop restores unrelated world geometry immediately afterwards.
 for(bool flying:{false,true}){
  state.active=true;fly=flying;camera.seeded=true;hide_all_gui=true;
  glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(-1,1,-1,1,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();glDisable(GL_DEPTH_TEST);glDisable(GL_TEXTURE_2D);glColor4f(1,1,1,1);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
  auto quad=[](){glBegin(GL_QUADS);glVertex2f(-.8f,-.8f);glVertex2f(.8f,-.8f);glVertex2f(.8f,.8f);glVertex2f(-.8f,.8f);glEnd();};
  glPushMatrix();billboard_pitch(30,1,0,0);quad();glPopMatrix();unsigned char pixel[4]{};glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(pixel[0]==0);
  quad();glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(pixel[0]==255);
  state.active=false;assert(!clean_view());hide_all_gui=false;
 }
 // Floor mode preserves the working 3D projection, pose and orientation.
 for(bool ortho:{false,true}){
  state.active=true;state.max_x=state.max_y=2048;fps=true;fly=true;base=0;camera.seeded=true;camera.eye={100,100,24};camera.right={1,0,0};camera.up={0,0,1};camera.back={0,-1,0};
  glMatrixMode(GL_PROJECTION);glLoadIdentity();if(ortho)glOrtho(-100,100,-100,100,-1,4096);else glFrustum(-50,50,-50,50,100,4096);glGetFloatv(GL_PROJECTION_MATRIX,saved_projection.data());glMatrixMode(GL_MODELVIEW);glLoadIdentity();
  view(0,0,0);float actual[16];glGetFloatv(GL_PROJECTION_MATRIX,actual);assert(actual[11]==saved_projection[11]&&actual[15]==saved_projection[15]);assert(actual[0]==saved_projection[0]&&actual[5]==saved_projection[5]);assert(camera.back.y==-1&&camera.up.z==1);assert(camera.eye.z==24);
 }
 state.active=false;fps=false;camera.reset();
 Camera c;c.seeded=true;c.eye={4,5,30};assert(session.begin(c,1,2,1000));assert(!session.begin(c,1,2,1000));session.package.manifest["synthetic_fixture"]=true;
 auto projection=perspective(75,1,1,1000);assert(projection);
 for(int face=0;face<7;++face){Camera rendered=c;auto p=*projection;assert(session.view(rendered,p.data()));glDrawBuffer(GL_BACK);glClearColor(1,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_SCISSOR_TEST);glScissor(0,8,16,8);glClearColor(0,1,0,1);glClear(GL_COLOR_BUFFER_BIT);glDisable(GL_SCISSOR_TEST);
  glPixelStorei(GL_PACK_ALIGNMENT,8);glPixelStorei(GL_PACK_ROW_LENGTH,23);glPixelTransferf(GL_RED_SCALE,.5f);
  assert(session.read_frame(root));GLint alignment=0,row=0;GLfloat scale=0;glGetIntegerv(GL_PACK_ALIGNMENT,&alignment);glGetIntegerv(GL_PACK_ROW_LENGTH,&row);glGetFloatv(GL_RED_SCALE,&scale);assert(alignment==8&&row==23&&scale==.5f);
 }
 assert(!session.active());for(int i=0;i<500&&writer().busy;++i)Sleep(10);assert(!writer().busy&&writer().result==1);
 std::filesystem::path folder;for(const auto& entry:std::filesystem::directory_iterator(root)){assert(folder.empty());folder=entry.path();}assert(folder.extension()!=L".partial");
 std::ifstream input(folder/L"manifest.json");Json manifest;input>>manifest;assert(manifest["media"].size()==7&&manifest["calibration_id"].is_null()&&manifest["game_executable_sha256"].get<std::string>().size()==64);
 assert(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)));
 Com<IWICImagingFactory> factory;assert(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_IWICImagingFactory,reinterpret_cast<void**>(factory.put()))));
 for(const auto& item:manifest["media"]){Com<IWICBitmapDecoder> decoder;Com<IWICBitmapFrameDecode> frame;Com<IWICFormatConverter> converter;
  assert(SUCCEEDED(factory->CreateDecoderFromFilename((folder/item["file"].get<std::string>()).c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,decoder.put()))&&SUCCEEDED(decoder->GetFrame(0,frame.put()))&&SUCCEEDED(factory->CreateFormatConverter(converter.put())));
  assert(SUCCEEDED(converter->Initialize(frame.p,GUID_WICPixelFormat24bppRGB,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)));
  unsigned char pixels[16*16*3]{};assert(SUCCEEDED(converter->CopyPixels(nullptr,48,sizeof pixels,pixels)));assert(pixels[0]==0&&pixels[1]==255&&pixels[15*48]==255&&pixels[15*48+1]==0);
 }
 // Resizing invalidates the whole capture rather than mixing projections.
 assert(session.begin(c,1,2,2000));Camera rendered=c;auto p=*projection;assert(session.view(rendered,p.data()));glViewport(0,0,17,16);assert(!session.view(rendered,p.data())&&!session.active());
 wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);
 std::cout<<"PASS: capture gating/focus cancellation, seven PNGs, metadata, upright pixels, GL state restoration and resize cancellation\n";
}
