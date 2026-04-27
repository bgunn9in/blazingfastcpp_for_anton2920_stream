# blazingfastcpp_for_anton2920_stream

Tested on AMD Ryzen 9 5950X

## Build

ISPC must be available in `PATH`.

```sh
cmake -S . -B build
cmake --build build --config Release
```

On Linux/WSL, use a Linux `ispc` binary. If it is not in `PATH`, pass it explicitly:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DISPC_EXECUTABLE=/path/to/ispc
cmake --build build --config Release
```

The build produces three executables:

- `ispc_parallel` - Intel ISPC version.
- `ispc_parallel_fasta` - Intel ISPC version fork for optimization work.
- `blazingfastcpp` - C++ AVX2/OpenMP version.

## Benchmark

All executables support benchmark options:

```sh
ispc_parallel --frames 300 --warmup 20 --no-output
ispc_parallel_fasta --frames 300 --warmup 20 --no-output
blazingfastcpp --frames 300 --warmup 20 --no-output
```

Run the comparison script:

```sh
python scripts/benchmark.py --frames 300 --warmup 20 --runs 7
```

On this workload, thread count can materially affect results. Pin a specific OpenMP thread count with:

```sh
python scripts/benchmark.py --frames 300 --warmup 20 --runs 7 --omp-num-threads 24
```

Or from CMake after configuring:

```sh
cmake --build build --config Release --target benchmark
```

Use `--out-csv benchmark-results.csv` to save per-run raw results.

Generate visual comparison images in the project root:

```sh
cmake --build build --config Release --target render_images
```

This writes:

- `ispc_parallel.ppm`
- `ispc_parallel_fasta.ppm`
- `blazingfastcpp.ppm`

Latest benchmark result on AMD Ryzen 9 5950X, Windows Release build, `--frames 300 --warmup 20 --runs 7 --omp-num-threads 24`:

| implementation | avg ms/frame | min ms | max ms | stddev ms | FPS | speedup vs ISPC |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `ispc_parallel` | 1.2906 | 1.2333 | 1.3327 | 0.0306 | 775.27 | 1.000x |
| `ispc_parallel_fasta` | 0.8943 | 0.8895 | 0.8982 | 0.0031 | 1118.20 | 1.443x |
| `blazingfastcpp` | 0.7683 | 0.7570 | 0.8058 | 0.0155 | 1302.09 | 1.680x |

Used C++ opts: `/openmp /Ot /fp:fast /std:c++20 /arch:AVX2`
Used `ispc_parallel_fasta` ISPC opts: `-O3 --target=avx2-i32x8 --opt=force-aligned-memory`
