#ifndef MICRO_FEATURES_H_
#define MICRO_FEATURES_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle for the frontend instance
typedef struct MicroFrontend MicroFrontend;

// Output structure for processed samples
typedef struct {
	float *features;        // Array of feature values (caller must free)
	size_t features_size;   // Number of features
	size_t samples_read;    // Number of audio samples consumed
} MicroFrontendOutput;

// Create a new frontend instance
// Returns NULL on error
MicroFrontend *micro_frontend_create(void);

// Process 16kHz 16-bit audio samples
// audio_data: pointer to int16_t audio samples
// audio_size: number of samples (not bytes)
// Returns 0 on success, non-zero on error
// The output structure's features array must be freed by the caller
int micro_frontend_process_samples(MicroFrontend *frontend,
				   const int16_t *audio_data,
				   size_t audio_size,
				   MicroFrontendOutput *output);

// Reset the frontend state
void micro_frontend_reset(MicroFrontend *frontend);

// Destroy the frontend instance and free all resources
void micro_frontend_destroy(MicroFrontend *frontend);

#ifdef __cplusplus
}
#endif

#endif  // MICRO_FEATURES_H_

