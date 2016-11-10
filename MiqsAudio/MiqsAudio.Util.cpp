#include "pch.h"
#include "MiqsAudio.Base.h"


using namespace MiqsAudio;


const double i8scale = 127.5;
const double i16scale = 32767.5;
const double i24scale = 8388607.5;
const double i32scale = 2147483647.5;

struct FormatI8
{
	static const double scale;
	static void ToI8(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int8_t * in = (std::int8_t *)i;
		std::int8_t * out = (std::int8_t *)o;

		*out = *in;
	}

	static void ToI16(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int8_t * in = (std::int8_t *)i;
		std::int16_t * out = (std::int16_t *)o;

		*out = *in;
		*out <<= 8;
	}

	static void ToI32(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int8_t * in = (std::int8_t *)i;
		std::int32_t * out = (std::int32_t *)o;

		*out = *in;
	}

	static void ToF32(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int8_t * in = (std::int8_t *)i;
		float * out = (float *)o;

		*out = static_cast<float>(*in);
		*out += 0.5; *out /= static_cast<float>(scale);
	}

	static void ToF64(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int8_t * in = (std::int8_t *)i;
		double * out = (double *)o;

		*out = static_cast<double>(*in);
		*out += 0.5; *out /= scale;
	}

};

struct FormatI16
{
	static const double scale;
	static void ToI8(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int16_t * in = (std::int16_t *)i;
		std::int8_t * out = (std::int8_t *)o;

		*out = (std::int8_t)((*in >> 8) & 0x00ff);
	}

	static void ToI16(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int16_t * in = (std::int16_t *)i;
		std::int16_t * out = (std::int16_t *)o;

		*out = *in;
	}

	static void ToI32(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int16_t * in = (std::int16_t *)i;
		std::int32_t * out = (std::int32_t *)o;

		*out = *in;
		*out <<= 16;
	}

	static void ToF32(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int16_t * in = (std::int16_t *)i;
		float * out = (float *)o;

		*out = static_cast<float>(*in);
		*out += 0.5; *out /= static_cast<float>(scale);
	}

	static void ToF64(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int16_t * in = (std::int16_t *)i;
		double * out = (double *)o;

		*out = static_cast<double>(*in);
		*out += 0.5; *out /= scale;
	}

};


struct FormatI32
{
	static const double scale;
	static void ToI8(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int32_t * in = (std::int32_t *)i;
		std::int8_t * out = (std::int8_t *)o;

		*out = (std::int8_t)((*in >> 24) & 0x000000ff);
	}

	static void ToI16(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int32_t * in = (std::int32_t *)i;
		std::int16_t * out = (std::int16_t *)o;

		*out = (std::int16_t)((*in >> 16) & 0x0000ffff);
	}

	static void ToI32(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int32_t * in = (std::int32_t *)i;
		std::int32_t * out = (std::int32_t *)o;

		*out = *in;
	}

	static void ToF32(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int32_t * in = (std::int32_t *)i;
		float * out = (float *)o;

		*out = static_cast<float>(*in);
		*out += 0.5; *out /= static_cast<float>(scale);
	}

	static void ToF64(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		std::int32_t * in = (std::int32_t *)i;
		double * out = (double *)o;

		*out = static_cast<double>(*in);
		*out += 0.5; *out /= scale;
	}

};

struct FormatF32
{
	static void ToI8(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		float * in = (float *)i;
		std::int8_t * out = (std::int8_t *)o;

		*out = (std::int8_t)(*in * i8scale - 0.5);
	}

	static void ToI16(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		float * in = (float *)i;
		std::int16_t * out = (std::int16_t *)o;

		*out = (std::int16_t)(*in * i16scale - 0.5);
	}

	static void ToI32(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		float * in = (float *)i;
		std::int32_t * out = (std::int32_t *)o;

		*out = (std::int32_t)(*in * i32scale - 0.5);
	}

	static void ToF32(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		float * in = (float *)i;
		float * out = (float *)o;

		*out = static_cast<float>(*in);
	}

	static void ToF64(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		float * in = (float *)i;
		double * out = (double *)o;

		*out = static_cast<double>(*in);
	}

};

struct FormatF64
{
	static void ToI8(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		double * in = (double *)i;
		std::int8_t * out = (std::int8_t *)o;

		*out = (std::int8_t)(*in * i8scale - 0.5);
	}

	static void ToI16(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		double * in = (double *)i;
		std::int16_t * out = (std::int16_t *)o;

		*out = (std::int16_t)(*in * i16scale - 0.5);
	}

	static void ToI32(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		double * in = (double *)i;
		std::int32_t * out = (std::int32_t *)o;

		*out = (std::int32_t)(*in * i32scale - 0.5);
	}

	static void ToF32(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		double * in = (double *)i;
		float * out = (float *)o;

		*out = static_cast<float>(*in);
	}

	static void ToF64(MiqsAudio::byte_t * i, MiqsAudio::byte_t * o)
	{
		double * in = (double *)i;
		double * out = (double *)o;

		*out = static_cast<double>(*in);
	}

};


const double FormatI8::scale = i8scale;
const double FormatI16::scale = i16scale;
const double FormatI32::scale = i32scale;


void MiqsAudio::ConvertIOFormat(byte_t * dstBuffer, byte_t * srcBuffer, ConvertInfo & info, size_t bufferSize)
{
	auto iBytes = GetBytes(info.srcFormat);
	auto oBytes = GetBytes(info.dstFormat);
	size_t j{};
	byte_t * in = srcBuffer;
	byte_t * out = dstBuffer;

	switch (info.srcFormat)
	{
	case AudioFormat::Int8:
	{
		switch (info.dstFormat)
		{
		case AudioFormat::Int8:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI8::ToI8(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Int16:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI8::ToI16(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Int32:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI8::ToI32(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Float32:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI8::ToF32(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Float64:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI8::ToF64(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		}

		break;
	}
	case AudioFormat::Int16:
	{
		switch (info.dstFormat)
		{
		case AudioFormat::Int8:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI16::ToI8(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Int16:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI16::ToI16(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Int32:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI16::ToI32(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Float32:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI16::ToF32(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Float64:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI16::ToF64(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		}
		break;
	}
	case AudioFormat::Int32:
	{
		switch (info.dstFormat)
		{
		case AudioFormat::Int8:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI32::ToI8(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Int16:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI32::ToI16(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Int32:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI32::ToI32(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Float32:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI32::ToF32(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Float64:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatI32::ToF64(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		}
		break;
	}
	case AudioFormat::Float32:
	{
		switch (info.dstFormat)
		{
		case AudioFormat::Int8:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatF32::ToI8(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Int16:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatF32::ToI16(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Int32:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatF32::ToI32(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Float32:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatF32::ToF32(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Float64:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatF32::ToF64(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		}
		break;
	}
	case AudioFormat::Float64:
	{
		switch (info.dstFormat)
		{
		case AudioFormat::Int8:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatF64::ToI8(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Int16:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatF64::ToI16(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Int32:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatF64::ToI32(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Float32:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatF64::ToF32(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		case AudioFormat::Float64:
		{
			for (size_t i{}; i < bufferSize; i++)
			{
				for (j = 0; j < info.nChannels; j++)
				{
					FormatF64::ToF64(&(in[info.srcChannelOffset[j] * iBytes]), &(out[info.dstChannelOffset[j] * oBytes]));
				}
				in += info.srcSampleStep * iBytes;
				out += info.dstSampleStep * oBytes;
			}
			break;
		}
		}
		break;
	}
	}
}
