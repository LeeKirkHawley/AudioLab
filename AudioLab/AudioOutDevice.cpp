#include "stdafx.h"
#include "AudioOutDevice.h"

CRITICAL_SECTION waveCriticalSection;


AudioOutDevice::AudioOutDevice()
{
	hWaveOut = NULL;
	InitializeCriticalSection(&waveCriticalSection);
}


AudioOutDevice::~AudioOutDevice()
{
	DeleteCriticalSection(&waveCriticalSection);
}

void AudioOutDevice::Open()
{
	waveBlocks = allocateBlocks(BLOCK_SIZE, _BLOCK_COUNT);
	_waveFreeBlockCount = _BLOCK_COUNT;
	waveCurrentBlock = 0;
	//InitializeCriticalSection(&waveCriticalSection);

	//MMRESULT result;/* for waveOut return values */
					/*
					* first we need to set up the WAVEFORMATEX structure.
					* the structure describes the format of the audio.
					*/
	wfx.nSamplesPerSec = 44100; /* sample rate */
	wfx.wBitsPerSample = 16; /* sample size */
	wfx.nChannels = 2; /* channels*/
					   /*
					   * WAVEFORMATEX also has other fields which need filling.
					   * as long as the three fields above are filled this should
					   * work for any PCM (pulse code modulation) format.
					   */
	wfx.cbSize = 0; /* size of _extra_ info */
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	/*
	* try to open the default wave device. WAVE_MAPPER is
	* a constant defined in mmsystem.h, it always points to the
	* default wave device on the system (some people have 2 or
	* more sound cards).
	*/
	if (waveOutOpen(&hWaveOut,
					WAVE_MAPPER,
					&wfx,
					(DWORD_PTR)waveOutProc,
					(DWORD_PTR)&_waveFreeBlockCount,
					CALLBACK_FUNCTION) != MMSYSERR_NOERROR) 
	{
		//fprintf(stderr, "%s: unable to open wave mapper device\n", argv[0]);
		ExitProcess(1);
	}	/*
	* device is now open so print the success message
	* and then close the device again.
	*/
	//printf("The Wave Mapper device was opened successfully!\n");
	//waveOutClose(hWaveOut);
}

void AudioOutDevice::Close()
{
	if (hWaveOut != NULL)
	{
		for (int i = 0; i < _waveFreeBlockCount; i++)
		{
			if (waveBlocks[i].dwFlags & WHDR_PREPARED)
				waveOutUnprepareHeader(hWaveOut, &waveBlocks[i], sizeof(WAVEHDR));
		}

		//DeleteCriticalSection(&waveCriticalSection);
		freeBlocks(waveBlocks);

		waveOutClose(hWaveOut);
	}
}


void CALLBACK AudioOutDevice::waveOutProc(
	HWAVEOUT hWaveOut,
	UINT uMsg,
	DWORD dwInstance,
	DWORD dwParam1,
	DWORD dwParam2
)
{
	/*
	* pointer to free block counter
	*/
	int* freeBlockCounter = (int*)dwInstance;
	/*
	* ignore calls that occur due to openining and closing the
	* device.
	*/
	if (uMsg != WOM_DONE)
		return;
	EnterCriticalSection(&waveCriticalSection);
	(*freeBlockCounter)++;
	LeaveCriticalSection(&waveCriticalSection);
}

WAVEHDR* AudioOutDevice::allocateBlocks(int size, int count)
{
	unsigned char* buffer;
	int i;
	WAVEHDR* blocks;
	DWORD totalBufferSize = (size + sizeof(WAVEHDR)) * count;
	/*
	* allocate memory for the entire set in one go
	*/
	if ((buffer = (unsigned char *)HeapAlloc(
									GetProcessHeap(),
									HEAP_ZERO_MEMORY,
									totalBufferSize
		)) == NULL) 
	{
		fprintf(stderr, "Memory allocation error\n");
		ExitProcess(1);
	}
	/*
	* and set up the pointers to each bit
	*/
	blocks = (WAVEHDR*)buffer;
	buffer += sizeof(WAVEHDR) * count;
	for (i = 0; i < count; i++) {
		blocks[i].dwBufferLength = size;
		blocks[i].lpData = (LPSTR)buffer;
		buffer += size;
	}
	return blocks;
}

void AudioOutDevice::freeBlocks(WAVEHDR* blockArray)
{
	/*
	* and this is why allocateBlocks works the way it does
	*/
	HeapFree(GetProcessHeap(), 0, blockArray);
}

void AudioOutDevice::writeAudio(LPSTR data, int size)
{
	WAVEHDR* current;
	int remain;
	current = &waveBlocks[waveCurrentBlock];
	while (size > 0) {
		/*
		* first make sure the header we're going to use is unprepared
		*/
		if (current->dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(hWaveOut, current, sizeof(WAVEHDR));

		if (size < (int)(BLOCK_SIZE - current->dwUser)) 
		{
			memcpy(current->lpData + current->dwUser, data, size);
			current->dwUser += size;
			break;
		}

		remain = BLOCK_SIZE - current->dwUser;
		memcpy(current->lpData + current->dwUser, data, remain);
		size -= remain;
		data += remain;
		current->dwBufferLength = BLOCK_SIZE;
		waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR));
		waveOutWrite(hWaveOut, current, sizeof(WAVEHDR));
		EnterCriticalSection(&waveCriticalSection);
		_waveFreeBlockCount--;
		LeaveCriticalSection(&waveCriticalSection);
		/*
		* wait for a block to become free
		*/
		while (!_waveFreeBlockCount)
			Sleep(10);
		/*
		* point to the next block
		*/
		waveCurrentBlock++;
		waveCurrentBlock %= _BLOCK_COUNT;
		current = &waveBlocks[waveCurrentBlock];
		current->dwUser = 0;
	}
}


