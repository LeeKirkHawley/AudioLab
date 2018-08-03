#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>


class AudioOutDevice
{
private:
	HWAVEOUT hWaveOut; /* device handle */
	WAVEHDR* waveBlocks;
	volatile int _waveFreeBlockCount;
	int waveCurrentBlock;
	WAVEFORMATEX wfx; /* look this up in your documentation */

public:
	const int BLOCK_SIZE = 8192;
	const int _BLOCK_COUNT = 20;

	AudioOutDevice();
	~AudioOutDevice();

	int waveFreeBlockCount()
	{
		return _waveFreeBlockCount;
	}
	int BLOCK_COUNT()
	{
		return _BLOCK_COUNT;
	}

	void Open();
	void Close();
	static void CALLBACK waveOutProc(
		HWAVEOUT hWaveOut,
		UINT uMsg,
		DWORD dwInstance,
		DWORD dwParam1,
		DWORD dwParam2);
	WAVEHDR* allocateBlocks(int size, int count);
	void freeBlocks(WAVEHDR* blockArray);
	void writeAudio(LPSTR data, int size);
};



