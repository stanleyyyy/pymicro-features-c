// examples/example.c
// Example usage of the micro_features library

#include <stdio.h>
#include <stdlib.h>
#include "micro_features.h"

int main(void) {
	// Create a frontend instance
	MicroFrontend *frontend = micro_frontend_create();
	if (!frontend) {
		fprintf(stderr, "Failed to create frontend\n");
		return 1;
	}

	// Example: Process some audio samples
	// In a real application, you would read audio from a file or microphone
	// For this example, we'll use dummy data
	const size_t num_samples = 160;  // 10ms at 16kHz
	int16_t *audio_samples = (int16_t *)malloc(num_samples * sizeof(int16_t));
	if (!audio_samples) {
		micro_frontend_destroy(frontend);
		return 1;
	}

	// Fill with dummy data (silence in this case)
	for (size_t i = 0; i < num_samples; ++i) {
		audio_samples[i] = 0;
	}

	// Process the samples
	MicroFrontendOutput output;
	int result = micro_frontend_process_samples(frontend, audio_samples,
						    num_samples, &output);

	if (result != 0) {
		fprintf(stderr, "Failed to process samples (error code: %d)\n",
			result);
		free(audio_samples);
		micro_frontend_destroy(frontend);
		return 1;
	}

	// Print results
	printf("Processed %zu samples\n", output.samples_read);
	printf("Generated %zu features:\n", output.features_size);
	for (size_t i = 0; i < output.features_size; ++i) {
		printf("  Feature[%zu] = %.6f\n", i, output.features[i]);
	}

	// Free the features array
	free(output.features);

	// Reset the frontend
	micro_frontend_reset(frontend);

	// Clean up
	free(audio_samples);
	micro_frontend_destroy(frontend);

	return 0;
}

