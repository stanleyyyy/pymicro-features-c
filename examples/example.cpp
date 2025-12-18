// examples/example.cpp
// C++ example usage of the micro_features library

#include <iostream>
#include <vector>
#include <memory>
#include "micro_features.h"

// C++ wrapper class for convenience
class MicroFrontendWrapper {
public:
	MicroFrontendWrapper() : frontend_(micro_frontend_create()) {
		if (!frontend_) {
			throw std::runtime_error("Failed to create frontend");
		}
	}

	~MicroFrontendWrapper() {
		if (frontend_) {
			micro_frontend_destroy(frontend_);
		}
	}

	// Delete copy constructor and assignment
	MicroFrontendWrapper(const MicroFrontendWrapper &) = delete;
	MicroFrontendWrapper &operator=(const MicroFrontendWrapper &) = delete;

	// Move constructor
	MicroFrontendWrapper(MicroFrontendWrapper &&other) noexcept
		: frontend_(other.frontend_) {
		other.frontend_ = nullptr;
	}

	// Move assignment
	MicroFrontendWrapper &operator=(MicroFrontendWrapper &&other) noexcept {
		if (this != &other) {
			if (frontend_) {
				micro_frontend_destroy(frontend_);
			}
			frontend_ = other.frontend_;
			other.frontend_ = nullptr;
		}
		return *this;
	}

	std::vector<float> process_samples(const std::vector<int16_t> &audio) {
		MicroFrontendOutput output;
		int result = micro_frontend_process_samples(
			frontend_, audio.data(), audio.size(), &output);

		if (result != 0) {
			throw std::runtime_error("Failed to process samples");
		}

		std::vector<float> features(output.features,
					    output.features +
						output.features_size);
		free(output.features);  // Free the C-allocated memory

		return features;
	}

	void reset() { micro_frontend_reset(frontend_); }

private:
	MicroFrontend *frontend_;
};

int main() {
	try {
		// Create a frontend instance
		MicroFrontendWrapper frontend;

		// Example: Process some audio samples
		const size_t num_samples = 160;  // 10ms at 16kHz
		std::vector<int16_t> audio_samples(num_samples, 0);

		// Process the samples
		std::vector<float> features = frontend.process_samples(audio_samples);

		// Print results
		std::cout << "Generated " << features.size() << " features:\n";
		for (size_t i = 0; i < features.size(); ++i) {
			std::cout << "  Feature[" << i << "] = " << features[i]
				  << "\n";
		}

		// Reset the frontend
		frontend.reset();

	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}

