/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <array>

#include "MediaCodecsSupport.h"
#include "PlatformDecoderModule.h"
#include "mozilla/gfx/gfxVars.h"
#include "nsTHashMap.h"

using MediaCodecsSupport = mozilla::media::MediaCodecsSupport;

namespace mozilla::media {

#define CODEC_SUPPORT_LOG(msg, ...) \
  MOZ_LOG(sPDMLog, LogLevel::Debug, ("MediaCodecsSupport, " msg, ##__VA_ARGS__))

void MCSInfo::AddSupport(const MediaCodecsSupported& aSupport) {
  StaticMutexAutoLock lock(sUpdateMutex);
  MCSInfo* instance = GetInstance();
  MOZ_ASSERT(instance);
  instance->mSupport += aSupport;
}

MediaCodecsSupported MCSInfo::GetSupport() {
  StaticMutexAutoLock lock(sUpdateMutex);
  MCSInfo* instance = GetInstance();
  MOZ_ASSERT(instance);
  return instance->mSupport;
}

void MCSInfo::ResetSupport() {
  StaticMutexAutoLock lock(sUpdateMutex);
  MCSInfo* instance = GetInstance();
  MOZ_ASSERT(instance);
  instance->mSupport.clear();
}

DecodeSupportSet MCSInfo::GetDecodeSupportSet(
    const MediaCodec& aCodec, const MediaCodecsSupported& aSupported) {
  DecodeSupportSet support;
  const auto supportInfo = GetCodecDefinition(aCodec);
  if (aSupported.contains(supportInfo.swDecodeSupport)) {
    support += DecodeSupport::SoftwareDecode;
  }
  if (aSupported.contains(supportInfo.hwDecodeSupport)) {
    support += DecodeSupport::HardwareDecode;
  }
  return support;
}

MediaCodecsSupported MCSInfo::GetDecodeMediaCodecsSupported(
    const MediaCodec& aCodec, const DecodeSupportSet& aSupportSet) {
  MediaCodecsSupported support;
  const auto supportInfo = GetCodecDefinition(aCodec);
  if (aSupportSet.contains(DecodeSupport::SoftwareDecode)) {
    support += supportInfo.swDecodeSupport;
  }
  if (aSupportSet.contains(DecodeSupport::HardwareDecode)) {
    support += supportInfo.hwDecodeSupport;
  }
  return support;
}

void MCSInfo::GetMediaCodecsSupportedString(
    nsCString& aSupportString, const MediaCodecsSupported& aSupportedCodecs) {
  CodecDefinition supportInfo;
  aSupportString = "Codec support information:\n"_ns;
  MCSInfo* instance = GetInstance();
  for (const auto& it : aSupportedCodecs) {
    MOZ_ASSERT(instance);
    MOZ_ASSERT(instance->mHashTableMCS);
    if (!instance->mHashTableMCS->Get(it, &supportInfo)) {
      CODEC_SUPPORT_LOG(
          "Could not find string matching MediaCodecsSupported enum: %d",
          static_cast<int>(it));
      aSupportString.Append("Unknown codec entry found!\n"_ns);
      continue;
    }
    // Get codec name string and append SW/HW support info
    aSupportString.Append(supportInfo.commonName);
    if (it == supportInfo.swDecodeSupport) {
      aSupportString.Append(" SW"_ns);
    }
    if (it == supportInfo.hwDecodeSupport) {
      aSupportString.Append(" HW"_ns);
    }
    aSupportString.Append("\n"_ns);
  }
  // Remove any trailing newline characters
  if (!aSupportString.IsEmpty()) {
    aSupportString.Truncate(aSupportString.Length() - 1);
  }
}

MCSInfo* MCSInfo::GetInstance() {
  StaticMutexAutoLock lock(sInitMutex);
  CODEC_SUPPORT_LOG("Getting MCSInfo instance.");
  if (!sInstance) {
    CODEC_SUPPORT_LOG("No MCSInfo instance found - creating new one.");
    sInstance.reset(new MCSInfo());
  }
  MOZ_ASSERT(sInstance);
  return sInstance.get();
}

MCSInfo::MCSInfo() {
  // Initialize hash tables
  mHashTableMCS.reset(new nsTHashMap<MediaCodecsSupport, CodecDefinition>());
  mHashTableCodec.reset(new nsTHashMap<MediaCodec, CodecDefinition>());

  for (const auto& it : GetAllCodecDefinitions()) {
    // Insert MediaCodecsSupport values as keys
    MOZ_ASSERT(mHashTableMCS);
    mHashTableMCS->InsertOrUpdate(it.swDecodeSupport, it);
    mHashTableMCS->InsertOrUpdate(it.hwDecodeSupport, it);
    // Insert codec enum values as keys
    MOZ_ASSERT(mHashTableCodec);
    mHashTableCodec->InsertOrUpdate(it.codec, it);
  }
}

CodecDefinition MCSInfo::GetCodecDefinition(const MediaCodec& aCodec) {
  CodecDefinition info;
  MCSInfo* instance = GetInstance();
  MOZ_ASSERT(instance);
  MOZ_ASSERT(instance->mHashTableCodec);
  if (instance->mHashTableCodec->Get(aCodec, &info)) {
    CODEC_SUPPORT_LOG("Could not find codec definition for codec enum: %d!",
                      static_cast<int>(aCodec));
  }
  return info;
}

std::array<CodecDefinition, 12> MCSInfo::GetAllCodecDefinitions() {
  static constexpr std::array<CodecDefinition, 12> codecDefinitions = {
      {{MediaCodec::H264, "H264", "video/avc",
        MediaCodecsSupport::H264SoftwareDecode,
        MediaCodecsSupport::H264HardwareDecode},
       {MediaCodec::VP9, "VP9", "video/vp9",
        MediaCodecsSupport::VP9SoftwareDecode,
        MediaCodecsSupport::VP9HardwareDecode},
       {MediaCodec::VP8, "VP8", "video/vp8",
        MediaCodecsSupport::VP8SoftwareDecode,
        MediaCodecsSupport::VP8HardwareDecode},
       {MediaCodec::AV1, "AV1", "video/av1",
        MediaCodecsSupport::AV1SoftwareDecode,
        MediaCodecsSupport::AV1HardwareDecode},
       {MediaCodec::Theora, "Theora", "video/theora",
        MediaCodecsSupport::TheoraSoftwareDecode,
        MediaCodecsSupport::TheoraHardwareDecode},
       {MediaCodec::AAC, "AAC", "audio/mp4a-latm",
        MediaCodecsSupport::AACSoftwareDecode,
        MediaCodecsSupport::AACHardwareDecode},
       {MediaCodec::MP3, "MP3", "audio/mpeg",
        MediaCodecsSupport::MP3SoftwareDecode,
        MediaCodecsSupport::MP3HardwareDecode},
       {MediaCodec::Opus, "Opus", "audio/opus",
        MediaCodecsSupport::OpusSoftwareDecode,
        MediaCodecsSupport::OpusHardwareDecode},
       {MediaCodec::Vorbis, "Vorbis", "audio/vorbis",
        MediaCodecsSupport::VorbisSoftwareDecode,
        MediaCodecsSupport::VorbisHardwareDecode},
       {MediaCodec::FLAC, "FLAC", "audio/flac",
        MediaCodecsSupport::FLACSoftwareDecode,
        MediaCodecsSupport::FLACHardwareDecode},
       {MediaCodec::Wave, "Wave", "audio/x-wav",
        MediaCodecsSupport::WaveSoftwareDecode,
        MediaCodecsSupport::WaveHardwareDecode},
       {MediaCodec::SENTINEL, "Undefined codec name",
        "Undefined MIME type string", MediaCodecsSupport::SENTINEL,
        MediaCodecsSupport::SENTINEL}}};
  return codecDefinitions;
}
}  // namespace mozilla::media

#undef CODEC_SUPPORT_LOG
