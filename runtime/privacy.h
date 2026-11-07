#pragma once
#include <atomic>
#include <cstdint>
namespace ssc_privacy {
enum class Detection { unavailable,off,on };
using Reader=Detection(*)();
inline std::atomic<Reader> reader{nullptr};
inline std::atomic<bool> respect{true},manual{false},uploads_held{false};
inline std::atomic<bool> effective{false};
inline std::atomic<uint64_t> generation{0};
inline Detection detection(){auto probe=reader.load();return probe?probe():Detection::unavailable;}
inline bool blocked(){
 const bool next=manual.load()||(respect.load()&&detection()!=Detection::off);
 if(effective.exchange(next)!=next){++generation;if(next)uploads_held=true;}
 return next;
}
inline bool uploads_allowed(){return !blocked()&&!uploads_held.load();}
inline bool resume_uploads(){if(blocked())return false;uploads_held=false;return true;}
inline const wchar_t* status(){if(manual)return L"Privacy protection on (manual)";auto native=detection();if(respect&&native==Detection::unavailable)return L"Anonymous-mode detection unavailable; protection on";if(respect&&native==Detection::on)return L"Privacy protection on (anonymous mode)";return uploads_held?L"Privacy off; uploads paused until resumed":L"Privacy protection inactive";}
}
