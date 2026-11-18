#pragma once
#include <string>
namespace ssc_blind {
// The native 64px question-mark tile in menuTextures.png (column 7, row 25).
// Validate the atlas instead of borrowing an arbitrary skill's icon.
inline int question_frame(std::string path,int width,int height,int columns,int rows,int tile){for(auto& c:path){if(c=='\\')c='/';if(c>='A'&&c<='Z')c+=32;}auto slash=path.find_last_of('/');auto name=path.substr(slash==std::string::npos?0:slash+1);return (name=="menutextures"||name=="menutextures.png")&&width==2048&&height==2048&&columns==32&&rows==32&&tile==64?807:-1;}
}
