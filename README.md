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

The build produces two executables:

- `ispc_parallel` - Intel ISPC version.
- `blazingfastcpp` - C++ AVX2/OpenMP version.

## Benchmark

Both executables support benchmark options:

```sh
ispc_parallel --frames 300 --warmup 20 --no-output
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

Latest benchmark result on AMD Ryzen 9 5950X, Windows Release build, `--frames 300 --warmup 20 --runs 7 --omp-num-threads 24`:

| implementation | avg ms/frame | min ms | max ms | stddev ms | FPS | speedup vs ISPC |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `ispc_parallel` | 1.2515 | 1.1439 | 1.3449 | 0.0748 | 801.97 | 1.000x |
| `blazingfastcpp` | 0.7548 | 0.7479 | 0.7739 | 0.0080 | 1324.94 | 1.658x |

Used opts: `/openmp /Ot /fp:fast /std:c++20 /arch:AVX2`
