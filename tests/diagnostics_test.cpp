#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cassert>
#include <iostream>
#include "../runtime/diagnostics.h"
int main(int argc,char** argv){
    assert(argc==2);ssc_diagnostics::initialize(argv[1]);assert(ssc_diagnostics::crash_text[0]);
    // Exercise the reporting path with a synthetic exception without crashing the game.
    EXCEPTION_RECORD record{};record.ExceptionCode=0xe0424242;record.ExceptionAddress=reinterpret_cast<void*>(&main);
    CONTEXT context{};RtlCaptureContext(&context);EXCEPTION_POINTERS info{&record,&context};
    assert(ssc_diagnostics::crashed(&info)==EXCEPTION_CONTINUE_SEARCH);
    assert(std::filesystem::file_size(ssc_diagnostics::crash_text)>0);
    assert(std::filesystem::file_size(ssc_diagnostics::crash_dump)>0);
    std::cout<<"Crash text and minidump generated; exception remains unhandled\n";
}
