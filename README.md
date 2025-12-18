# Micro Features C/C++ Library

This is a pure C/C++ library that provides speech feature extraction using the TensorFlow Lite Micro audio frontend. It is a standalone version of the Python library functionality.

## API Overview

The library provides a simple C API for processing 16kHz 16-bit audio samples and extracting features.

### Header File

Include `micro_features.h` in your project:

```c
#include "micro_features.h"
```

### Functions

#### `MicroFrontend *micro_frontend_create(void)`

Creates a new frontend instance. Returns `NULL` on error.

#### `int micro_frontend_process_samples(MicroFrontend *frontend, const int16_t *audio_data, size_t audio_size, MicroFrontendOutput *output)`

Processes audio samples and extracts features.

**Parameters:**
- `frontend`: Frontend instance created with `micro_frontend_create()`
- `audio_data`: Pointer to int16_t audio samples (16kHz, 16-bit)
- `audio_size`: Number of samples (not bytes)
- `output`: Output structure that will be filled with results

**Returns:**
- `0` on success
- `-1` if any parameter is NULL
- `-2` if not enough samples provided (minimum 160 samples required)
- `-3` if no features were generated
- `-4` if memory allocation failed

**Note:** The `output->features` array must be freed by the caller using `free()`.

#### `void micro_frontend_reset(MicroFrontend *frontend)`

Resets the frontend state to initial conditions.

#### `void micro_frontend_destroy(MicroFrontend *frontend)`

Destroys the frontend instance and frees all resources.

### Data Structures

```c
typedef struct {
	float *features;        // Array of feature values (caller must free)
	size_t features_size;   // Number of features
	size_t samples_read;    // Number of audio samples consumed
} MicroFrontendOutput;
```

## Building

### Using the Makefile

```bash
make -f Makefile.lib
```

This will build:
- `libmicro_features.a` - Static library
- `examples/example_c` - C example
- `examples/example_cpp` - C++ example

### Manual Build

1. Compile all source files with `-DFIXED_POINT=16` flag
2. Link against the TensorFlow Lite Micro frontend sources
3. Include the `include/` and `kissfft/` directories

## Usage Example (C)

```c
#include "micro_features.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
	// Create frontend
	MicroFrontend *frontend = micro_frontend_create();
	if (!frontend) {
		fprintf(stderr, "Failed to create frontend\n");
		return 1;
	}

	// Prepare audio data (16kHz, 16-bit samples)
	int16_t audio_samples[160];  // 10ms of audio
	// ... fill audio_samples with your audio data ...

	// Process samples
	MicroFrontendOutput output;
	int result = micro_frontend_process_samples(
		frontend, audio_samples, 160, &output);

	if (result == 0) {
		printf("Generated %zu features\n", output.features_size);
		// Use output.features...
		free(output.features);  // Don't forget to free!
	}

	// Clean up
	micro_frontend_destroy(frontend);
	return 0;
}
```

## Usage Example (C++)

See `examples/example.cpp` for a C++ wrapper class that provides RAII semantics.

## Requirements

- C99 or C++11 compiler
- TensorFlow Lite Micro frontend library (included in `tensorflow/` directory)
- KISS FFT library (included in `kissfft/` directory)

## Configuration

The frontend is configured with the following defaults (matching the Python library):
- Sample rate: 16kHz
- Feature duration: 30ms
- Feature step size: 10ms
- Number of filterbank channels: 40
- Frequency range: 125Hz - 7500Hz

These settings are hardcoded in `src/micro_features_lib.c` in the `init_cfg()` function.

