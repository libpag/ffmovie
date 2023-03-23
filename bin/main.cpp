#include <iostream>
#include "include/ffmovie/movie.h"

int main() {
  std::cout << "Hello, World!" << std::endl;

  auto demuxers = ffmovie::FFMediaDemuxer::SupportDemuxers();
  std::cout << "demuxers:";
  for (const std::string& str : demuxers) {
    std::cout << str << ",";
  }
  std::cout << std::endl;

  auto decoders = ffmovie::FFMediaDecoder::SupportDecoders();
  std::cout << "decoders:";
  for (const std::string& str : decoders) {
    std::cout << str << ",";
  }
  std::cout << std::endl;
  
  return 0;

  return 0;
}
