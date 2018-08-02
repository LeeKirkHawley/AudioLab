#pragma once

#include <string>
#include <iostream>
#include <fstream>

using namespace std;

//Chunks
struct chunk_t
{
	char ID[4]; //"data" = 0x61746164
	unsigned long size;  //Chunk data bytes
};

class WaveFile
{
	struct wav_header_t
	{
		char _chunkID[4]; //"RIFF" = 0x46464952
		unsigned long _chunkSize; //28 [+ sizeof(wExtraFormatBytes) + wExtraFormatBytes] + sum(sizeof(chunk.id) + sizeof(chunk.size) + chunk.size)
		char _format[4]; //"WAVE" = 0x45564157
		char _subchunk1ID[4]; //"fmt " = 0x20746D66
		unsigned long _subchunk1Size; //16 [+ sizeof(wExtraFormatBytes) + wExtraFormatBytes]
		unsigned short _audioFormat;
		unsigned short _numChannels;
		unsigned long _sampleRate;
		unsigned long _byteRate;
		unsigned short _blockAlign;
		unsigned short _bitsPerSample;
		//[WORD wExtraFormatBytes;]
		//[Extra format bytes]
	};

	wav_header_t _header;
	//FILE *fin = nullptr;
	std::ifstream infile;



public:
	WaveFile();
	~WaveFile();

	int numChannels()
	{
		return _header._numChannels;
	}

	int sampleRate()
	{
		return _header._sampleRate;
	}

	int bytesPerSample()
	{
		return _header._bitsPerSample / 8;
	}

	bool Open(std::wstring FilePath)
	{
		//int err = _wfopen_s(&fin, FilePath.c_str(), L"rb");
		infile.open(FilePath.c_str(), ios::binary | ios::in);

		if (!infile.is_open())
			return false;

		//Read WAV header
		//fread(&_header, sizeof(_header), 1, fin);
		infile.read((char*)&_header, sizeof(_header));

		//Print WAV header
		//printf("WAV File Header read:\n");
		//printf("File Type: %s\n", header.chunkID);
		//printf("File Size: %ld\n", header.chunkSize);
		//printf("WAV Marker: %s\n", header.format);
		//printf("Format Name: %s\n", header.subchunk1ID);
		//printf("Format Length: %ld\n", header.subchunk1Size);
		//printf("Format Type: %hd\n", header.audioFormat);
		//printf("Number of Channels: %hd\n", header.numChannels);
		//printf("Sample Rate: %ld\n", header.sampleRate);
		//printf("Sample Rate * Bits/Sample * Channels / 8: %ld\n", header.byteRate);
		//printf("Bits per Sample * Channels / 8.1: %hd\n", header.blockAlign);
		//printf("Bits per Sample: %hd\n", header.bitsPerSample);

		//skip wExtraFormatBytes & extra format bytes
		//fseek(f, header.chunkSize - 16, SEEK_CUR);

		//Reading file
		chunk_t chunk;
		//printf("id\t" "size\n");
		//go to data chunk
		while (true)
		{
			//fread(&chunk, sizeof(chunk), 1, fin);
			int s = sizeof(chunk);
			infile.read((char*)&chunk, sizeof(chunk));
			//printf("%c%c%c%c\t" "%li\n", chunk.ID[0], chunk.ID[1], chunk.ID[2], chunk.ID[3], chunk.size);
			if (*(unsigned int *)&chunk.ID == 0x61746164)
				break;
			//skip chunk data bytes
			//fseek(fin, chunk.size, SEEK_CUR);
			infile.seekg(chunk.size, ios_base::cur);
		}

		//Number of samples
		int sample_size = _header._bitsPerSample / 8;
		int samples_count = chunk.size * 8 / _header._bitsPerSample;
		//printf("Samples count = %i\n", samples_count);

		short int *value = new short int[samples_count];
		memset(value, 0, sizeof(short int) * samples_count);

		return true;
	}

	bool Read(BYTE* Bytes, int Samples)
	{
		return Read(Bytes, Samples, bytesPerSample(), numChannels());
	}

	bool Read(BYTE* Bytes, int Samples, int SampleSizeBytes, int Channels)
	{
		if (!infile.is_open())
			return false;

		int BytesToRead = Samples * SampleSizeBytes * Channels;

		//int BytesRead = fread(Bytes, 1, BytesToRead, fin);
		try
		{
			if (!infile.read((char *)Bytes, BytesToRead))
				return false;
		}
		catch (std::ifstream::failure e)
		{

		}

		return true;
	}

	void Close()
	{
		if (infile.is_open())
			infile.close();
	}
};

