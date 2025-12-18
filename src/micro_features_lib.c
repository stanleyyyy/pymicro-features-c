// src/micro_features_lib.c
#include "micro_features.h"

#include <stdlib.h>
#include <string.h>

#include "tensorflow/lite/experimental/microfrontend/lib/frontend.h"
#include "tensorflow/lite/experimental/microfrontend/lib/frontend_util.h"

// Constants
#define FEATURES_STEP_SIZE 10
#define PREPROCESSOR_FEATURE_SIZE 40
#define FEATURE_DURATION_MS 30
#define AUDIO_SAMPLE_FREQUENCY 16000
#define SAMPLES_PER_CHUNK (FEATURES_STEP_SIZE * (AUDIO_SAMPLE_FREQUENCY / 1000))
#define BYTES_PER_CHUNK (SAMPLES_PER_CHUNK * 2)
#define FLOAT32_SCALE 0.0390625f

// Frontend handle structure
struct MicroFrontend {
	struct FrontendConfig cfg;
	struct FrontendState st;
};

// Initialize configuration with defaults
static void init_cfg(struct FrontendConfig *cfg) {
	cfg->window.size_ms = FEATURE_DURATION_MS;
	cfg->window.step_size_ms = FEATURES_STEP_SIZE;

	cfg->filterbank.num_channels = PREPROCESSOR_FEATURE_SIZE;
	cfg->filterbank.lower_band_limit = 125.0f;
	cfg->filterbank.upper_band_limit = 7500.0f;

	cfg->noise_reduction.smoothing_bits = 10;
	cfg->noise_reduction.even_smoothing = 0.025f;
	cfg->noise_reduction.odd_smoothing = 0.06f;
	cfg->noise_reduction.min_signal_remaining = 0.05f;

	cfg->pcan_gain_control.enable_pcan = 1;
	cfg->pcan_gain_control.strength = 0.95f;
	cfg->pcan_gain_control.offset = 80.0f;
	cfg->pcan_gain_control.gain_bits = 21;

	cfg->log_scale.enable_log = 1;
	cfg->log_scale.scale_shift = 6;
}

MicroFrontend *micro_frontend_create(void) {
	MicroFrontend *frontend = (MicroFrontend *)malloc(sizeof(MicroFrontend));
	if (!frontend) {
		return NULL;
	}

	init_cfg(&frontend->cfg);
	if (!FrontendPopulateState(&frontend->cfg, &frontend->st,
				    AUDIO_SAMPLE_FREQUENCY)) {
		free(frontend);
		return NULL;
	}

	return frontend;
}

int micro_frontend_process_samples(MicroFrontend *frontend,
				    const int16_t *audio_data,
				    size_t audio_size,
				    MicroFrontendOutput *output) {
	if (!frontend || !audio_data || !output) {
		return -1;
	}

	// Initialize output
	output->features = NULL;
	output->features_size = 0;
	output->samples_read = 0;

	if (audio_size < SAMPLES_PER_CHUNK) {
		return -2;  // Not enough samples
	}

	size_t samples_read = 0;
	struct FrontendOutput fo = FrontendProcessSamples(&frontend->st, audio_data,
							   SAMPLES_PER_CHUNK,
							   &samples_read);

	// If no features generated (not enough samples yet), return success with 0 features
	if (fo.size == 0 || fo.values == NULL) {
		output->features = NULL;
		output->features_size = 0;
		output->samples_read = samples_read;
		return 0;
	}

	// Allocate and convert features to float
	float *features = (float *)malloc(fo.size * sizeof(float));
	if (!features) {
		return -4;  // Memory allocation failed
	}

	for (size_t i = 0; i < fo.size; ++i) {
		features[i] = (float)(fo.values[i] * FLOAT32_SCALE);
	}

	output->features = features;
	output->features_size = fo.size;
	output->samples_read = samples_read;

	return 0;
}

void micro_frontend_reset(MicroFrontend *frontend) {
	if (!frontend) {
		return;
	}

	FrontendFreeStateContents(&frontend->st);
	if (!FrontendPopulateState(&frontend->cfg, &frontend->st,
				    AUDIO_SAMPLE_FREQUENCY)) {
		// Reset failed, but we continue anyway
	}
}

void micro_frontend_destroy(MicroFrontend *frontend) {
	if (!frontend) {
		return;
	}

	FrontendFreeStateContents(&frontend->st);
	free(frontend);
}

