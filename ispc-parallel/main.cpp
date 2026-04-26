#include <assert.h>
#include <chrono>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "benchmark_options.h"
#include "shader.h"

#define nil (void*)0ULL

#define WIDTH 800
#define HEIGHT 600

using namespace ispc;


int
DumpPPM(const char *filename, unsigned int *pixels, int width, int height)
{
	unsigned int	pixel;
	FILE * out;
	int	i;

	out = fopen(filename, "wb");
	if (out == nil) {
		perror("Failed to open file");
		return 1;
	}

	fprintf(out, "P6 %d %d 255 ", width, height);
	for (i = 0; i < width * height; i++) {
		pixel = pixels[i];
		fputc((pixel >> 24) & 0xFF, out);
		fputc((pixel >> 16) & 0xFF, out);
		fputc((pixel >> 8) & 0xFF, out);
	}

	return 0;
}


int
main(int argc, char** argv)
{
	BenchmarkOptions options;
	const int parseResult = ParseBenchmarkOptions(argc, argv, &options);
	if (parseResult != 0) {
		PrintBenchmarkUsage(argv[0]);
		return parseResult;
	}
	if (options.help) {
		PrintBenchmarkUsage(argv[0]);
		return 0;
	}

	unsigned int	*pixels;
	float	fi;
	int	i;

	pixels = (unsigned int *)calloc(WIDTH * HEIGHT, sizeof(*pixels));
	assert(pixels != nil);

	fi = 0;
	for (i = 0; i < options.warmup; i++) {
		Shader(pixels, WIDTH, HEIGHT, fi);
		fi++;
	}

	const auto start = std::chrono::steady_clock::now();
	for (i = 0; i < options.frames; i++) {
		Shader(pixels, WIDTH, HEIGHT, fi);
		fi++;
	}
	const auto end = std::chrono::steady_clock::now();

	const double totalMs = std::chrono::duration<double, std::milli>(end - start).count();
	PrintBenchmarkResult("ispc_parallel", WIDTH, HEIGHT, options, totalMs);

	if (options.writeOutput) {
		Shader(pixels, WIDTH, HEIGHT, 0.0f);
		DumpPPM(options.output, pixels, WIDTH, HEIGHT);
	}
	free(pixels);
	return 0;
}
