#include "audio_codec.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <vorbis/vorbisfile.h>
#pragma GCC diagnostic pop
#include <vorbis/vorbisenc.h>
#include <memory>
namespace ssc_audio {
namespace {
struct Reader {
 OggVorbis_File stream{};bool opened=false;
 Reader(const std::filesystem::path& path){
  if(!std::filesystem::is_regular_file(path)||std::filesystem::file_size(path)>limit)throw std::runtime_error("Invalid or oversized OGG");
  FILE* f=_wfopen(path.c_str(),L"rb");if(!f)throw std::runtime_error("Could not open OGG");
  if(ov_open_callbacks(f,&stream,nullptr,0,OV_CALLBACKS_DEFAULT)<0){fclose(f);throw std::runtime_error("Use Ogg Vorbis audio");}opened=true;
 }
 ~Reader(){if(opened)ov_clear(&stream);}
 OggInfo info(){auto* i=ov_info(&stream,-1);double s=ov_time_total(&stream,-1);
  if(!i||ov_streams(&stream)!=1||i->channels<1||i->channels>2||i->rate<8000||i->rate>192000||!std::isfinite(s)||s<=0)throw std::runtime_error("Unsupported OGG stream");
  return {unsigned(i->rate),unsigned(i->channels),s};}
};
}
OggInfo ogg_info(const std::filesystem::path& path){Reader reader(path);return reader.info();}
Wave read_ogg(const std::filesystem::path& path){
 Reader reader(path);auto info=reader.info();if(info.seconds*info.rate*info.channels*2>limit)throw std::runtime_error("Decoded audio exceeds 64 MiB limit");
 Wave wave{info.channels,info.rate,16,{}};char bytes[16384];int section=0;
 for(;;){long n=ov_read(&reader.stream,bytes,sizeof(bytes),0,2,1,&section);if(!n)break;if(n<0||size_t(n)>limit-wave.raw.size())throw std::runtime_error("Invalid or oversized decoded OGG");wave.raw.insert(wave.raw.end(),bytes,bytes+n);}
 if(wave.raw.empty()||wave.raw.size()%(wave.channels*2))throw std::runtime_error("Incomplete OGG audio");
 return wave;
}
void write_ogg(const std::filesystem::path& path,const Wave& wave){
 if(wave.bits!=16||wave.channels<1||wave.channels>2||wave.rate<8000||wave.rate>192000||wave.raw.empty()||wave.raw.size()>limit||wave.raw.size()%(2*wave.channels))throw std::runtime_error("Invalid music audio");
 struct Encoder {vorbis_info info{};vorbis_comment comment{};vorbis_dsp_state dsp{};vorbis_block block{};ogg_stream_state stream{};bool d=false,b=false,s=false;
  Encoder(){vorbis_info_init(&info);vorbis_comment_init(&comment);}
  ~Encoder(){if(s)ogg_stream_clear(&stream);if(b)vorbis_block_clear(&block);if(d)vorbis_dsp_clear(&dsp);vorbis_comment_clear(&comment);vorbis_info_clear(&info);}
 } e;
 if(vorbis_encode_init_vbr(&e.info,wave.channels,wave.rate,.5f))throw std::runtime_error("Could not encode music");
 if(vorbis_analysis_init(&e.dsp,&e.info))throw std::runtime_error("Could not initialize music encoder");
 e.d=true;
 if(vorbis_block_init(&e.dsp,&e.block))throw std::runtime_error("Could not initialize music block");
 e.b=true;
 if(ogg_stream_init(&e.stream,0x535343))throw std::runtime_error("Could not initialize music stream");
 e.s=true;
 std::ofstream out(path,std::ios::binary|std::ios::trunc);ogg_page page;ogg_packet header,comments,books,packet;
 auto write_page=[&](){out.write(reinterpret_cast<char*>(page.header),page.header_len);out.write(reinterpret_cast<char*>(page.body),page.body_len);if(!out)throw std::runtime_error("Could not save music");};
 vorbis_analysis_headerout(&e.dsp,&e.comment,&header,&comments,&books);ogg_stream_packetin(&e.stream,&header);ogg_stream_packetin(&e.stream,&comments);ogg_stream_packetin(&e.stream,&books);
 while(ogg_stream_flush(&e.stream,&page))write_page();
 size_t frames=wave.raw.size()/(wave.channels*2),position=0;
 do{size_t count=std::min(size_t(4096),frames-position);float** buffer=vorbis_analysis_buffer(&e.dsp,int(count?count:1));if(!buffer)throw std::runtime_error("Music encoder allocation failed");
  for(size_t i=0;i<count;++i)for(unsigned c=0;c<wave.channels;++c)buffer[c][i]=int16_t(u16(wave.raw.data()+((position+i)*wave.channels+c)*2))/32768.f;
  position+=count;vorbis_analysis_wrote(&e.dsp,int(count));
  while(vorbis_analysis_blockout(&e.dsp,&e.block)==1){vorbis_analysis(&e.block,nullptr);vorbis_bitrate_addblock(&e.block);while(vorbis_bitrate_flushpacket(&e.dsp,&packet)){ogg_stream_packetin(&e.stream,&packet);while(ogg_stream_pageout(&e.stream,&page))write_page();}}
  if(!count)break;
 }while(true);
 out.close();if(!out)throw std::runtime_error("Could not finish saving music");
}
}
