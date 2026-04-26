#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

struct BenchmarkOptions {
	int frames = 100;
	int warmup = 3;
	const char* output = "image.ppm";
	bool writeOutput = true;
	bool csv = false;
	bool help = false;
};

inline void PrintBenchmarkUsage(const char* executableName) {
	std::printf(
		"Usage: %s [options]\n"
		"\n"
		"Options:\n"
		"  --frames N       Number of measured frames. Default: 100\n"
		"  --warmup N       Number of warmup frames before timing. Default: 3\n"
		"  --output PATH    Write final frame to PATH. Default: image.ppm\n"
		"  --no-output      Do not write image output.\n"
		"  --csv            Print benchmark as CSV.\n"
		"  --help           Show this help.\n",
		executableName);
}

inline bool ParseBenchmarkInt(const char* value, int* out) {
	char* end = nullptr;
	long parsed = std::strtol(value, &end, 10);
	if (end == value || *end != '\0' || parsed < 0 || parsed > 100000000L) {
		return false;
	}
	*out = static_cast<int>(parsed);
	return true;
}

inline int ParseBenchmarkOptions(int argc, char** argv, BenchmarkOptions* options) {
	for (int i = 1; i < argc; ++i) {
		const char* arg = argv[i];
		if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
			options->help = true;
			return 0;
		}
		if (std::strcmp(arg, "--csv") == 0) {
			options->csv = true;
			continue;
		}
		if (std::strcmp(arg, "--no-output") == 0) {
			options->writeOutput = false;
			continue;
		}
		if (std::strcmp(arg, "--frames") == 0 || std::strcmp(arg, "--warmup") == 0 ||
			std::strcmp(arg, "--output") == 0) {
			if (i + 1 >= argc) {
				std::fprintf(stderr, "Missing value for %s\n", arg);
				return 1;
			}
			const char* value = argv[++i];
			if (std::strcmp(arg, "--frames") == 0 && !ParseBenchmarkInt(value, &options->frames)) {
				std::fprintf(stderr, "Invalid --frames value: %s\n", value);
				return 1;
			}
			if (std::strcmp(arg, "--warmup") == 0 && !ParseBenchmarkInt(value, &options->warmup)) {
				std::fprintf(stderr, "Invalid --warmup value: %s\n", value);
				return 1;
			}
			if (std::strcmp(arg, "--output") == 0) {
				options->output = value;
				options->writeOutput = true;
			}
			continue;
		}
		std::fprintf(stderr, "Unknown option: %s\n", arg);
		return 1;
	}

	if (options->frames <= 0) {
		std::fprintf(stderr, "--frames must be greater than zero.\n");
		return 1;
	}

	return 0;
}

inline void PrintBenchmarkResult(const char* name, int width, int height, const BenchmarkOptions& options, double totalMs) {
	const double avgMs = totalMs / static_cast<double>(options.frames);
	const double fps = avgMs > 0.0 ? 1000.0 / avgMs : 0.0;
	if (options.csv) {
		std::printf("name,width,height,frames,warmup,total_ms,avg_ms,fps\n");
		std::printf("%s,%d,%d,%d,%d,%.6f,%.6f,%.6f\n",
			name, width, height, options.frames, options.warmup, totalMs, avgMs, fps);
		return;
	}

	std::printf(
		"BENCHMARK name=%s width=%d height=%d frames=%d warmup=%d total_ms=%.6f avg_ms=%.6f fps=%.6f\n",
		name, width, height, options.frames, options.warmup, totalMs, avgMs, fps);
}
