//==============================================================================
//
//  OvenMediaEngine
//
//  Created by Hyunjun Jang
//  Copyright (c) 2018 AirenSoft. All rights reserved.
//
//==============================================================================
#include "segment_stream.h"

#include <base/publisher/publisher.h>
#include <config/items/items.h>

#include "packetizer/packetizer.h"
#include "segment_stream_private.h"

SegmentStream::SegmentStream(
	const std::shared_ptr<pub::Application> application,
	const info::Stream &info,
	PacketizerFactory packetizer_factory)
	: Stream(application, info),

	  _packetizer_factory(packetizer_factory)
{
}

bool SegmentStream::Start()
{
	std::shared_ptr<MediaTrack> video_track = nullptr;
	std::shared_ptr<MediaTrack> audio_track = nullptr;

	for (auto &track_item : _tracks)
	{
		auto &track = track_item.second;

		if (CheckCodec(track->GetMediaType(), track->GetCodecId()))
		{
			switch (track->GetMediaType())
			{
				case cmn::MediaType::Video:
					video_track = track;
					break;

				case cmn::MediaType::Audio:
					audio_track = track;
					break;

				default:
					// Not implemented for another media types
					break;
			}
		}
	}

	bool video_enabled = (video_track != nullptr);
	bool audio_enabled = (audio_track != nullptr);

	if ((video_enabled == false) && (audio_enabled == false))
	{
		auto application = GetApplication();

		logtw("Stream [%s/%s] was not created because there were no supported codecs",
			  application->GetName().CStr(), GetName().CStr());

		return false;
	}

	if (video_enabled)
	{
		_media_tracks[video_track->GetId()] = video_track;
		_video_track = video_track;
	}

	if (audio_enabled)
	{
		_media_tracks[audio_track->GetId()] = audio_track;
		_audio_track = audio_track;
	}

	_packetizer = _packetizer_factory(GetApplicationName(), GetName().CStr(), _video_track, _audio_track);

	return Stream::Start();
}

bool SegmentStream::Stop()
{
	return Stream::Stop();
}

bool SegmentStream::OnStreamUpdated(const std::shared_ptr<info::Stream> &info)
{
	if (Stream::OnStreamUpdated(info) == false)
	{
		return false;
	}

	_last_msid = info->GetMsid();

	if (_packetizer != nullptr)
	{
		_packetizer->ResetPacketizer();
	}

	return true;
}

void SegmentStream::SendVideoFrame(const std::shared_ptr<MediaPacket> &media_packet)
{
	if (_packetizer != nullptr && _media_tracks.find(media_packet->GetTrackId()) != _media_tracks.end())
	{
		_packetizer->AppendVideoFrame(media_packet);
	}
}

void SegmentStream::SendAudioFrame(const std::shared_ptr<MediaPacket> &media_packet)
{
	if (_packetizer != nullptr && _media_tracks.find(media_packet->GetTrackId()) != _media_tracks.end())
	{
		_packetizer->AppendAudioFrame(media_packet);
	}
}

bool SegmentStream::GetPlayList(ov::String &play_list)
{
	if (_packetizer != nullptr)
	{
		return _packetizer->GetPlayList(play_list);
	}

	return false;
}

std::shared_ptr<const SegmentItem> SegmentStream::GetSegmentData(const ov::String &file_name) const
{
	if (_packetizer == nullptr)
	{
		return nullptr;
	}

	return _packetizer->GetSegmentData(file_name);
}

bool SegmentStream::CheckCodec(cmn::MediaType type, cmn::MediaCodecId codec_id)
{
	switch (type)
	{
		case cmn::MediaType::Video:
			switch (codec_id)
			{
				case cmn::MediaCodecId::H264:
				case cmn::MediaCodecId::H265:
					return true;

				default:
					// Not supported codec
					return false;
			}
			break;

		case cmn::MediaType::Audio:
			switch (codec_id)
			{
				case cmn::MediaCodecId::Aac:
					return true;

				default:
					// Not supported codec
					break;
			}
			break;

		default:
			break;
	}

	return false;
}
