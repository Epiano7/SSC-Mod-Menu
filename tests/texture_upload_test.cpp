#include "../runtime/overlay.cpp"
#include <cassert>
int main(){
    WNDCLASSW wc{};wc.style=CS_OWNDC;wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"SSCTextureTest";assert(RegisterClassW(&wc));
    // Hidden test surface: never activates a window or launches the game.
    HWND window=CreateWindowW(wc.lpszClassName,L"",WS_POPUP,0,0,64,64,nullptr,nullptr,wc.hInstance,nullptr);assert(window);
    HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR pfd{};pfd.nSize=sizeof(pfd);pfd.nVersion=1;pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=32;
    int format=ChoosePixelFormat(dc,&pfd);assert(format&&SetPixelFormat(dc,format,&pfd));
    HGLRC gl=wglCreateContext(dc);assert(gl&&wglMakeCurrent(dc,gl));
    glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    for(int size:{16,16,32,32,16}){
        raster_w=size;raster_h=size/2;std::vector<unsigned char> input(size_t(raster_w)*raster_h*4),output(input.size());
        for(size_t i=0;i<input.size();++i)input[i]=unsigned((i*17+size*3)%256);
        pixels=input.data();upload_canvas_texture();assert(glGetError()==GL_NO_ERROR);
        glGetTexImage(GL_TEXTURE_2D,0,0x80E1,GL_UNSIGNED_BYTE,output.data());assert(glGetError()==GL_NO_ERROR&&input==output);
        for(auto& b:input){b^=0x5a;}upload_canvas_texture();glGetTexImage(GL_TEXTURE_2D,0,0x80E1,GL_UNSIGNED_BYTE,output.data());assert(input==output);
    }
    glDeleteTextures(1,&texture);wglMakeCurrent(nullptr,nullptr);wglDeleteContext(gl);ReleaseDC(window,dc);DestroyWindow(window);UnregisterClassW(wc.lpszClassName,wc.hInstance);
    std::puts("PASS: texture allocation, same-size updates, resize and pixel readback on hidden GL surface");
}
