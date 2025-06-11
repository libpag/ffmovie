///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2023 Tencent. All rights reserved.
//
//  This library is free software; you can redistribute it and/or modify it under the terms of the
//  GNU Lesser General Public License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
//  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
//  the GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License along with this
//  library; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  Boston, MA  02110-1301  USA
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "AudioFormatConverter.h"
#include "audio/AudioUtils.h"

namespace ffmovie {
AudioFormatConverter::AudioFormatConverter(std::shared_ptr<PCMOutputConfig> pcmOutputConfig)
    : pcmOutputConfig(std::move(pcmOutputConfig)) {
}

AudioFormatConverter::~AudioFormatConverter() {
  av_freep(&pConvertBuff);
  swr_free(&pSwrContext);
}

SampleData AudioFormatConverter::convert(AVFrame* frame) {
  if (frame == nullptr) {
    return {};
  }
  if (preFramePCMOutputConfig == nullptr) {
    preFramePCMOutputConfig = std::make_shared<PCMOutputConfig>();
  }
  int nbSample =
      static_cast<int>(frame->nb_samples * pcmOutputConfig->sampleRate / frame->sample_rate) + 256;
  if (nbSample > outputSamples) {
    if (pConvertBuff != nullptr) {
      av_freep(&pConvertBuff);
      pConvertBuff = nullptr;
    }
    outputSamples = nbSample;
  }
  if (pConvertBuff == nullptr) {
    if (av_samples_alloc(&pConvertBuff, nullptr, pcmOutputConfig->channels, outputSamples,
                         static_cast<AVSampleFormat>(pcmOutputConfig->format), 0) < 0) {
      return {};
    }
  }
  if (!IsSampleConfig(*preFramePCMOutputConfig, *frame) && pSwrContext != nullptr) {
    swr_free(&pSwrContext);
    pSwrContext = nullptr;
  }
  if (!pSwrContext) {
    AVChannelLayout out_ch_layout={}, in_ch_layout={};
    av_channel_layout_from_mask(&out_ch_layout, pcmOutputConfig->channelLayout);
    av_channel_layout_from_mask(&in_ch_layout, frame->ch_layout.u.mask);

    preFramePCMOutputConfig->sampleRate = frame->sample_rate;
    preFramePCMOutputConfig->format = frame->format;
    preFramePCMOutputConfig->channels = frame->ch_layout.nb_channels;
    preFramePCMOutputConfig->channelLayout = frame->ch_layout.u.mask;

    int ret = swr_alloc_set_opts2(&pSwrContext,
                  &out_ch_layout, static_cast<AVSampleFormat>(pcmOutputConfig->format),
                  pcmOutputConfig->sampleRate, &in_ch_layout,
                  static_cast<AVSampleFormat>(frame->format),
                  frame->sample_rate,
                  0, nullptr);
    if (ret < 0) {
      av_channel_layout_uninit(&out_ch_layout);
      av_channel_layout_uninit(&in_ch_layout);
      if (pConvertBuff) {
        av_freep(&pConvertBuff);
        pConvertBuff = nullptr;
      }
      if(pSwrContext) {
        swr_free(&pSwrContext);
        pSwrContext = nullptr;
      }
      return {};
    }
    swr_init(pSwrContext);
    av_channel_layout_uninit(&out_ch_layout);
    av_channel_layout_uninit(&in_ch_layout);
  }

  auto newNbSamples = swr_convert(pSwrContext, &pConvertBuff, outputSamples,
                                  (const uint8_t**)frame->data, frame->nb_samples);
  if (newNbSamples <= 0) {
    if (pConvertBuff) {
      av_freep(&pConvertBuff);
      pConvertBuff = nullptr;
    }
    if(pSwrContext) {
      swr_free(&pSwrContext);
      pSwrContext = nullptr;
    }
    return {};
  }
  return {pConvertBuff, SampleCountToLength(newNbSamples, pcmOutputConfig.get())};
}
}  // namespace pag
