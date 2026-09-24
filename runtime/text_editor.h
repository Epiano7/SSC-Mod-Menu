#pragma once
#include <algorithm>
#include <string>
namespace ssc_edit {
struct Cursor {
 size_t pos=0,anchor=0,view=0;
 void begin(size_t n){pos=anchor=n;view=0;}
 template<class S> void clamp(const S& s){pos=std::min(pos,s.size());anchor=std::min(anchor,s.size());view=std::min(view,s.size());}
 bool selected()const{return pos!=anchor;}
 template<class S> void erase(S& s){clamp(s);auto first=std::min(pos,anchor),last=std::max(pos,anchor);s.erase(first,last-first);pos=anchor=first;}
 template<class S> bool insert(S& s,const S& value,size_t limit){clamp(s);if(s.size()-std::max(pos,anchor)+std::min(pos,anchor)+value.size()>limit)return false;erase(s);s.insert(pos,value);pos+=value.size();anchor=pos;return true;}
 template<class S> void move(const S& s,int key,bool shift,bool control){clamp(s);if(key==VK_HOME)pos=0;else if(key==VK_END)pos=s.size();else if(!shift&&selected()){pos=key==VK_LEFT?std::min(pos,anchor):std::max(pos,anchor);}else if(key==VK_LEFT){if(pos)--pos;if(control)while(pos&&s[pos-1]!=' ')--pos;}else if(key==VK_RIGHT){if(pos<s.size())++pos;if(control)while(pos<s.size()&&s[pos]!=' ')++pos;}if(!shift)anchor=pos;}
 template<class S> void backspace(S& s){clamp(s);if(selected())erase(s);else if(pos){s.erase(--pos,1);anchor=pos;}}
 template<class S> void del(S& s){clamp(s);if(selected())erase(s);else if(pos<s.size())s.erase(pos,1);}
};
}
