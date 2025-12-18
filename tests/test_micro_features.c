// tests/test_micro_features.c
// C test program based on Python test_micro_features.py

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "micro_features.h"
#include "wav_reader.h"

#define BYTES_PER_CHUNK (160 * 2)  // 10ms @ 16kHz (16-bit mono)
#define SAMPLES_PER_CHUNK 160

// Compare two float arrays with tolerance
static int compare_features(const float *a, const float *b, size_t size,
			     float tolerance) {
	for (size_t i = 0; i < size; ++i) {
		if (fabsf(a[i] - b[i]) > tolerance) {
			return 0;
		}
	}
	return 1;
}

// Test with zeros
static int test_zeros(void) {
	printf("Running test_zeros...\n");
	MicroFrontend *frontend = micro_frontend_create();
	if (!frontend) {
		fprintf(stderr, "Failed to create frontend\n");
		return 1;
	}

	int16_t audio[160] = {0};  // 10ms of zeros
	MicroFrontendOutput output;

	// Need at least 30ms of audio before we get features
	int result = micro_frontend_process_samples(frontend, audio, 160,
						    &output);
	if (result != 0) {
		fprintf(stderr, "First process_samples failed\n");
		micro_frontend_destroy(frontend);
		return 1;
	}
	if (output.samples_read != 160 || output.features_size != 0) {
		fprintf(stderr, "Expected 160 samples read, 0 features\n");
		free(output.features);
		micro_frontend_destroy(frontend);
		return 1;
	}
	free(output.features);

	result = micro_frontend_process_samples(frontend, audio, 160, &output);
	if (result != 0) {
		fprintf(stderr, "Second process_samples failed\n");
		micro_frontend_destroy(frontend);
		return 1;
	}
	if (output.samples_read != 160 || output.features_size != 0) {
		fprintf(stderr, "Expected 160 samples read, 0 features\n");
		free(output.features);
		micro_frontend_destroy(frontend);
		return 1;
	}
	free(output.features);

	result = micro_frontend_process_samples(frontend, audio, 160, &output);
	if (result != 0) {
		fprintf(stderr, "Third process_samples failed\n");
		micro_frontend_destroy(frontend);
		return 1;
	}
	if (output.samples_read != 160 || output.features_size != 40) {
		fprintf(stderr, "Expected 160 samples read, 40 features, got %zu\n",
			output.features_size);
		free(output.features);
		micro_frontend_destroy(frontend);
		return 1;
	}
	free(output.features);

	// Now we get features for each 10ms chunk
	result = micro_frontend_process_samples(frontend, audio, 160, &output);
	if (result != 0) {
		fprintf(stderr, "Fourth process_samples failed\n");
		micro_frontend_destroy(frontend);
		return 1;
	}
	if (output.samples_read != 160 || output.features_size != 40) {
		fprintf(stderr, "Expected 160 samples read, 40 features\n");
		free(output.features);
		micro_frontend_destroy(frontend);
		return 1;
	}
	free(output.features);

	micro_frontend_destroy(frontend);
	printf("  test_zeros: PASSED\n");
	return 0;
}

// Process WAV file and collect features
static int process_wav_file(const char *filename, float **features_out,
			     size_t *features_count_out) {
	WavFile wav;
	if (wav_file_read(filename, &wav) != 0) {
		fprintf(stderr, "Failed to read WAV file: %s\n", filename);
		return 1;
	}

	MicroFrontend *frontend = micro_frontend_create();
	if (!frontend) {
		fprintf(stderr, "Failed to create frontend\n");
		wav_file_free(&wav);
		return 1;
	}

	// Estimate max features (one per 10ms chunk)
	size_t max_features = (wav.data_size / BYTES_PER_CHUNK) * 40;
	float *all_features = (float *)malloc(max_features * sizeof(float));
	if (!all_features) {
		micro_frontend_destroy(frontend);
		wav_file_free(&wav);
		return 1;
	}

	size_t total_features = 0;
	size_t i = 0;
	while ((i + SAMPLES_PER_CHUNK) * 2 <= wav.data_size) {
		MicroFrontendOutput output;
		int result = micro_frontend_process_samples(
			frontend, &wav.data[i], SAMPLES_PER_CHUNK, &output);
		if (result != 0) {
			// Some chunks might not produce features (first few)
			free(output.features);
			i += SAMPLES_PER_CHUNK;
			continue;
		}

		if (output.features_size > 0) {
			if (total_features + output.features_size > max_features) {
				// Reallocate if needed
				max_features = (total_features + output.features_size) * 2;
				float *new_features =
					(float *)realloc(all_features,
							max_features * sizeof(float));
				if (!new_features) {
					free(all_features);
					free(output.features);
					micro_frontend_destroy(frontend);
					wav_file_free(&wav);
					return 1;
				}
				all_features = new_features;
			}
			memcpy(&all_features[total_features], output.features,
			       output.features_size * sizeof(float));
			total_features += output.features_size;
		}
		free(output.features);
		i += SAMPLES_PER_CHUNK;
	}

	micro_frontend_destroy(frontend);
	wav_file_free(&wav);

	*features_out = all_features;
	*features_count_out = total_features;
	return 0;
}

// Test silence WAV file
static int test_silence(void) {
	printf("Running test_silence...\n");
	float *features = NULL;
	size_t features_count = 0;

	if (process_wav_file("tests/silence.wav", &features, &features_count) !=
	    0) {
		return 1;
	}

	if (features_count == 0) {
		fprintf(stderr, "No features generated from silence.wav\n");
		free(features);
		return 1;
	}

	printf("  Generated %zu features from silence.wav\n", features_count);
	printf("  First 5 features: ");
	for (size_t i = 0; i < 5 && i < features_count; ++i) {
		printf("%.6f ", features[i]);
	}
	printf("\n");

	free(features);
	printf("  test_silence: PASSED\n");
	return 0;
}

// Test speech WAV file
static int test_speech(void) {
	printf("Running test_speech...\n");
	float *features = NULL;
	size_t features_count = 0;

	if (process_wav_file("tests/speech.wav", &features, &features_count) !=
	    0) {
		return 1;
	}

	if (features_count == 0) {
		fprintf(stderr, "No features generated from speech.wav\n");
		free(features);
		return 1;
	}

	printf("  Generated %zu features from speech.wav\n", features_count);
	printf("  First 5 features: ");
	for (size_t i = 0; i < 5 && i < features_count; ++i) {
		printf("%.6f ", features[i]);
	}
	printf("\n");

	free(features);
	printf("  test_speech: PASSED\n");
	return 0;
}

// Test reset functionality
static int test_reset(void) {
	printf("Running test_reset...\n");
	WavFile wav;
	if (wav_file_read("tests/speech.wav", &wav) != 0) {
		fprintf(stderr, "Failed to read speech.wav\n");
		return 1;
	}

	MicroFrontend *frontend = micro_frontend_create();
	if (!frontend) {
		fprintf(stderr, "Failed to create frontend\n");
		wav_file_free(&wav);
		return 1;
	}

	// Process first time
	float *features1 = NULL;
	size_t features1_count = 0;
	size_t max_features = (wav.data_size / BYTES_PER_CHUNK) * 40;
	features1 = (float *)malloc(max_features * sizeof(float));
	if (!features1) {
		micro_frontend_destroy(frontend);
		wav_file_free(&wav);
		return 1;
	}

	size_t i = 0;
	while ((i + SAMPLES_PER_CHUNK) * 2 <= wav.data_size) {
		MicroFrontendOutput output;
		int result = micro_frontend_process_samples(
			frontend, &wav.data[i], SAMPLES_PER_CHUNK, &output);
		if (result == 0 && output.features_size > 0) {
			memcpy(&features1[features1_count], output.features,
			       output.features_size * sizeof(float));
			features1_count += output.features_size;
		}
		free(output.features);
		i += SAMPLES_PER_CHUNK;
	}

	// Process second time without reset (should be different)
	float *features2 = NULL;
	size_t features2_count = 0;
	features2 = (float *)malloc(max_features * sizeof(float));
	if (!features2) {
		free(features1);
		micro_frontend_destroy(frontend);
		wav_file_free(&wav);
		return 1;
	}

	i = 0;
	while ((i + SAMPLES_PER_CHUNK) * 2 <= wav.data_size) {
		MicroFrontendOutput output;
		int result = micro_frontend_process_samples(
			frontend, &wav.data[i], SAMPLES_PER_CHUNK, &output);
		if (result == 0 && output.features_size > 0) {
			memcpy(&features2[features2_count], output.features,
			       output.features_size * sizeof(float));
			features2_count += output.features_size;
		}
		free(output.features);
		i += SAMPLES_PER_CHUNK;
	}

	// Features should be different
	if (features1_count == features2_count &&
	    compare_features(features1, features2, features1_count, 0.0001f)) {
		fprintf(stderr, "Features should be different without reset\n");
		free(features1);
		free(features2);
		micro_frontend_destroy(frontend);
		wav_file_free(&wav);
		return 1;
	}

	// Reset and process third time (should match first)
	micro_frontend_reset(frontend);
	float *features3 = NULL;
	size_t features3_count = 0;
	features3 = (float *)malloc(max_features * sizeof(float));
	if (!features3) {
		free(features1);
		free(features2);
		micro_frontend_destroy(frontend);
		wav_file_free(&wav);
		return 1;
	}

	i = 0;
	while ((i + SAMPLES_PER_CHUNK) * 2 <= wav.data_size) {
		MicroFrontendOutput output;
		int result = micro_frontend_process_samples(
			frontend, &wav.data[i], SAMPLES_PER_CHUNK, &output);
		if (result == 0 && output.features_size > 0) {
			memcpy(&features3[features3_count], output.features,
			       output.features_size * sizeof(float));
			features3_count += output.features_size;
		}
		free(output.features);
		i += SAMPLES_PER_CHUNK;
	}

	// Features should match first time
	if (features1_count != features3_count ||
	    !compare_features(features1, features3, features1_count, 0.0001f)) {
		fprintf(stderr, "Features should match after reset\n");
		free(features1);
		free(features2);
		free(features3);
		micro_frontend_destroy(frontend);
		wav_file_free(&wav);
		return 1;
	}

	free(features1);
	free(features2);
	free(features3);
	micro_frontend_destroy(frontend);
	wav_file_free(&wav);

	printf("  test_reset: PASSED\n");
	return 0;
}

int main(void) {
	printf("Running micro_features C library tests...\n\n");

	int failed = 0;

	if (test_zeros() != 0) {
		failed = 1;
	}
	if (test_silence() != 0) {
		failed = 1;
	}
	if (test_speech() != 0) {
		failed = 1;
	}
	if (test_reset() != 0) {
		failed = 1;
	}

	printf("\n");
	if (failed) {
		printf("SOME TESTS FAILED\n");
		return 1;
	} else {
		printf("ALL TESTS PASSED\n");
		return 0;
	}
}

