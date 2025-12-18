// tests/wav_reader.c
// Simple WAV file reader for 16kHz, 16-bit, mono PCM files

#include "wav_reader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// WAV file header structure
#pragma pack(push, 1)
typedef struct {
	char riff[4];           // "RIFF"
	uint32_t file_size;     // File size - 8
	char wave[4];            // "WAVE"
	char fmt[4];             // "fmt "
	uint32_t fmt_size;       // Format chunk size (usually 16)
	uint16_t audio_format;   // 1 = PCM
	uint16_t num_channels;   // Number of channels
	uint32_t sample_rate;    // Sample rate
	uint32_t byte_rate;      // Byte rate
	uint16_t block_align;    // Block align
	uint16_t bits_per_sample; // Bits per sample
	char data[4];            // "data"
	uint32_t data_size;      // Data chunk size
} WavHeader;
#pragma pack(pop)

int wav_file_read(const char *filename, WavFile *wav) {
	FILE *file = fopen(filename, "rb");
	if (!file) {
		return -1;
	}

	WavHeader header;
	if (fread(&header, sizeof(WavHeader), 1, file) != 1) {
		fclose(file);
		return -2;
	}

	// Verify RIFF header
	if (memcmp(header.riff, "RIFF", 4) != 0 ||
	    memcmp(header.wave, "WAVE", 4) != 0) {
		fclose(file);
		return -3;
	}

	// Verify format
	if (memcmp(header.fmt, "fmt ", 4) != 0 || header.audio_format != 1) {
		fclose(file);
		return -4;
	}

	// Check for data chunk (might be after fmt chunk)
	if (memcmp(header.data, "data", 4) != 0) {
		// Skip to data chunk
		fseek(file, sizeof(WavHeader) - 4, SEEK_SET);
		char chunk_id[4];
		uint32_t chunk_size;
		while (fread(chunk_id, 4, 1, file) == 1) {
			if (memcmp(chunk_id, "data", 4) == 0) {
				if (fread(&chunk_size, 4, 1, file) != 1) {
					fclose(file);
					return -5;
				}
				header.data_size = chunk_size;
				break;
			}
			if (fread(&chunk_size, 4, 1, file) != 1) {
				fclose(file);
				return -6;
			}
			fseek(file, chunk_size, SEEK_CUR);
		}
		if (memcmp(chunk_id, "data", 4) != 0) {
			fclose(file);
			return -7;
		}
	}

	// Verify expected format: 16kHz, 16-bit, mono
	if (header.sample_rate != 16000 || header.bits_per_sample != 16 ||
	    header.num_channels != 1) {
		fclose(file);
		return -8;
	}

	// Allocate memory for audio data
	size_t num_samples = header.data_size / 2; // 16-bit = 2 bytes per sample
	int16_t *data = (int16_t *)malloc(header.data_size);
	if (!data) {
		fclose(file);
		return -9;
	}

	// Read audio data
	if (fread(data, 2, num_samples, file) != num_samples) {
		free(data);
		fclose(file);
		return -10;
	}

	fclose(file);

	// Fill WavFile structure
	wav->sample_rate = header.sample_rate;
	wav->bits_per_sample = header.bits_per_sample;
	wav->num_channels = header.num_channels;
	wav->data_size = header.data_size;
	wav->data = data;

	return 0;
}

void wav_file_free(WavFile *wav) {
	if (wav && wav->data) {
		free(wav->data);
		wav->data = NULL;
		wav->data_size = 0;
	}
}

