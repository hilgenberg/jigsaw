#ifdef ANDROID
#include "Utility/Preferences.h"
#include <oboe/Oboe.h>
#include <algorithm>
#include <vector>
#include "data.h"
#include <cstring>
#include <limits>

bool pending_vibration = false;

class WavFile
{
public:
	WavFile(const uint8_t *fd, size_t len) { load(fd, len); }

	std::vector<float> samples;
	int sampleRate  = 0;
	int numChannels = 0;

private:
	static inline int32_t readInt  (const uint8_t *d) { return ((int32_t)d[3] << 24) | ((int32_t)d[2] << 16) | ((int32_t)d[1] << 8) | (int32_t)d[0]; }
	static inline int16_t readShort(const uint8_t *d) { return ((int16_t)d[1] << 8) | (int16_t)d[0]; }
	static const uint8_t* getChunk(const char *header, const uint8_t *d, const uint8_t *end)
	{
		for (; d+4 <= end; d += 8 + (uint32_t)readInt(d+4))
			if (!memcmp(d, header, 4)) return d;
		return NULL;
	}
	enum WavAudioFormat {
		PCM = 0x0001,
		IEEEFloat = 0x0003,
		ALaw = 0x0006,
		MULaw = 0x0007,
		Extensible = 0xFFFE
	};
	bool load(const uint8_t *fd, size_t len)
	{
		#define DIE(msg) do { LOG_ERROR("Error reading wav data: %s", msg); return false; } while(0)
		const uint8_t * const end = fd + len;

		if (len < 12) DIE("File too short");
		// int32_t fileSizeInBytes = readInt(fd+4) + 8;
		if (memcmp(fd, "RIFF", 4) || memcmp(fd+8, "WAVE", 4)) DIE("Header not found");
		auto *data   = getChunk("data", fd+12, end); if (!data)   DIE("DATA header not found");
		auto *format = getChunk("fmt ", fd+12, end); if (!format) DIE("FMT header not found");
		if (format + 22 + 2 > end) DIE("File too short #2");
		uint16_t fmt = readShort(format + 8); if (fmt != PCM && fmt != IEEEFloat && fmt != Extensible) DIE("unsupported audio format");
		numChannels = readShort(format + 10); if (numChannels < 1 || numChannels > 128) DIE("invalid channel count");
		sampleRate = (int)readInt(format + 12);
		int bitDepth = (int)readShort(format + 22); // checked below while reading data
		int bps = bitDepth / 8; // bytes per sample
		if (readInt(format + 16) != (uint32_t)(numChannels * sampleRate * (bitDepth/8))) DIE("Bytes/sec inconsistent");
		if (readShort(format + 20) != numChannels * bps) DIE("Bytes/block inconsistent");
		if (data + 8 > end) DIE("File too short #3");
		int N = readInt(data + 4) / bps; if (N % numChannels) DIE("invalid sample count");
		samples.resize(N); float *sample = samples.data();
		auto *d = data + 8; if (d + N*bps > end) DIE("file too short #4");

		for (int i = 0; i < N; ++i, d += bps)
		{
			switch (bitDepth)
			{
				case 8:  *sample++ = (*d - 128) / 128.0f; break;
				case 16: *sample++ = readShort(d) / 32768.0f; break;
				case 24:
				{
					int32_t k = (d[2] << 16) | (d[1] << 8) | d[0];
					if (k & 0x800000) k |= ~0xFFFFFF; // extend sign
					*sample++ = k / 8388608.0f;
					break;
				}
				case 32:
				{
					int32_t k = readInt(d);
					if (fmt == IEEEFloat)
						*sample++ = reinterpret_cast<float &>(k);
					else // assume PCM
						*sample++ = (float)k / (float)(std::numeric_limits<std::int32_t>::max());
					break;
				}
				default: DIE("unsupported bit depth");
			}
		}

		return true;
	}
};

static WavFile &click()
{
	static WavFile f(click_data, click_data_len);
	return f;
}

class Audio : public oboe::AudioStreamCallback
{
public:
	Audio() {}

	void play_click()
	{
		pending_vibration = Preferences::vibrate();
		if (!Preferences::click()) return;
		if (!stream && !init()) return;
		pos = 0;
		stream->requestStart();
	}

	void pause()
	{
		if (!stream) return;
		stream->close();
		stream = nullptr;
	}

private:
	bool init()
	{
		oboe::AudioStreamBuilder builder;
		builder.setSharingMode(oboe::SharingMode::Shared);
		builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
		builder.setFormat(oboe::AudioFormat::Float);
		builder.setCallback(this);
		auto &data = click();
		builder.setChannelCount(data.numChannels);
		builder.setSampleRate(data.sampleRate);
		return builder.openStream(stream) == oboe::Result::OK;
	}

	oboe::DataCallbackResult onAudioReady(oboe::AudioStream *stream, void *buf_, int32_t N) override
	{
		const WavFile &data     = click();
		float         *buf      = (float*)buf_;
		const int      end      = (int)(data.samples.size() / data.numChannels);
		const int      n        = pos >= end ? 0 : std::min((int)N, end-pos);
		const int      channels = stream->getChannelCount();

		if (channels == data.numChannels)
		{
			memcpy(buf, &data.samples[pos*channels], n*channels*sizeof(float));
			pos += n;
			buf += n*channels;
		}
		else
		{
			const int NC = data.numChannels;
			for (int e = pos+n; pos < e; ++pos)
			for (int c = 0; c < channels; ++c)
				*buf++ = data.samples[pos*NC + c%NC];
		}

		if ((N -= n) > 0) memset(buf, 0, N*sizeof(float)*channels);
		return pos < end ? oboe::DataCallbackResult::Continue : oboe::DataCallbackResult::Stop;
	}

	std::shared_ptr<oboe::AudioStream> stream;
	int pos = 0; // in click data stream
};

static std::unique_ptr<Audio> audio;

void audio_pause()
{
	if (!audio) return;
	audio->pause();
}
void play_click()
{
	if (!audio) audio.reset(new Audio);
	audio->play_click();
}

#endif