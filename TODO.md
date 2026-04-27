# TODO: blazingfastcpp optimization plan

Current baseline from `benchmark-results.csv`:

- `ispc_parallel`: 1.0939 ms/frame average, 915.63 FPS.
- `blazingfastcpp`: 0.9683 ms/frame average, 1033.37 FPS.
- Current C++ speedup vs ISPC: 1.130x.
- Benchmark setup: 800x600, 300 measured frames, 20 warmup frames, 7 runs, Release, AVX2, OpenMP.

## Progress

- [x] 2026-04-26: Established baseline benchmark and saved raw results to `benchmark-results.csv`.
- [x] 2026-04-26: Added benchmark options and comparison script for repeatable regression checks.
- [x] Capture current image checksum before optimization: `blazing-baseline.ppm` SHA256 `6D9653960822BA1899039595D40FFEAF1E1D9BD291D155279A9E9BFD245C0825`.
- [x] Apply safe hot-loop cleanup: remove unused `oW`.
- [x] Regression check after `oW` removal: output SHA256 unchanged.
- [x] Apply positive-input `tanh` specialization for final color map.
- [x] Regression check after positive `tanh`: output SHA256 unchanged.
- [x] Specialize fixed-size 800x600 hot path: row-based AVX2 loop with no tile bounds or scalar tail.
- [x] Regression check after fixed-size row path: output SHA256 unchanged.
- [x] Run full benchmark after accepted optimizations and save raw results.
- [x] Update this TODO with final accepted/rejected optimization notes.
- [x] Current optimized C++ benchmark: 0.9166 ms/frame average, 1091.55 FPS, 1.187x vs ISPC in the same run.
- [x] Improvement vs prior C++ baseline: 0.9683 -> 0.9166 ms/frame average, about 5.34% faster.
- [x] Simplified final RGB base math by replacing `base - py * constants` with direct add/sub; output SHA256 unchanged.
- [x] Rewrote `v_sin`/`v_cos` quadrant sign handling with sign-bit XOR instead of extra blends/multiplies; output SHA256 unchanged.
- [x] Removed dead final `vx/vy` update in the 8th unrolled step; output SHA256 unchanged.
- [x] Rejected single-buffer benchmark loop: output SHA256 unchanged, but benchmark regressed from 0.9037 to 0.9220 ms/frame average, so double-buffer loop was restored.
- [x] Final accepted benchmark run saved to `benchmark-final-2.csv`: C++ 0.9653 ms/frame average, ISPC 1.2105 ms/frame average, C++ speedup 1.254x vs ISPC.
- [x] Best accepted run during this pass saved to `benchmark-after-dead-last-update.csv`: C++ 0.9037 ms/frame average, ISPC 1.1427 ms/frame average, C++ speedup 1.265x vs ISPC.
- [x] Final render regression check: SHA256 remains `6D9653960822BA1899039595D40FFEAF1E1D9BD291D155279A9E9BFD245C0825`.
- [x] Replaced PowerShell benchmark script with cross-platform `scripts/benchmark.py`.
- [x] Verified Windows benchmark script and CMake `benchmark` target with Python.
- [x] Verified WSL/Linux configure, build, executable runs, Python benchmark smoke-test, and CMake `benchmark` target using a local Linux `ispc` extracted from the Ubuntu package.
- [x] Tested MSVC `/GL`/LTCG, `/O2` without `/Ot`, `/openmp:llvm`, and `schedule(static, 1)`; all were rejected due to slower measured C++ runtime.
- [x] Tested final reciprocal approximation with Newton-Raphson; output SHA256 stayed unchanged, but measured C++ runtime did not improve, so it was rejected.
- [x] Swept `OMP_NUM_THREADS`; best tested Windows result was `OMP_NUM_THREADS=24` with C++ 0.7729 ms/frame over 3 runs.
- [x] Added `--omp-num-threads` to `scripts/benchmark.py` for reproducible thread-count tuning.
- [x] 2026-04-26: Added latest README benchmark: Windows Release, 300 frames, 20 warmup, 7 runs, `OMP_NUM_THREADS=24`, C++ 0.7548 ms/frame, ISPC 1.2515 ms/frame, speedup 1.658x.
- [x] 2026-04-26: Added `ispc_parallel_fasta` as a separate ISPC optimization target cloned from `ispc_parallel`.
- [x] 2026-04-26: Extended `scripts/benchmark.py` and CMake `benchmark` target to compare `ispc_parallel`, `ispc_parallel_fasta`, and `blazingfastcpp`.
- [x] 2026-04-26: Verified initial `ispc_parallel_fasta` image SHA256 matches `ispc_parallel`: `7DEE2F57EFAB20DFF495DB8AC99B9FA65522D74CA01AF51BC677A2B165C097F1`.
- [x] 2026-04-26: Initial three-way benchmark, Windows Release, 300 frames, 20 warmup, 7 runs, `OMP_NUM_THREADS=24`: `ispc_parallel` 1.2835 ms/frame, `ispc_parallel_fasta` 1.3004 ms/frame, `blazingfastcpp` 0.7563 ms/frame.
- [x] 2026-04-26: `ispc_parallel_fasta` baseline before optimization: 1.2753 ms/frame over 5 runs with matching SHA256.
- [x] 2026-04-26: Accepted first ISPC task split optimization: `launch[24]` with original-output `renderedHeight=576`; SHA256 unchanged, `fasta` improved to 0.9946 ms/frame over 5 runs.
- [x] 2026-04-26: ISPC macro substitution does not work directly inside `launch[...]`; switched `ispc_parallel_fasta/shader.ispc` to a CMake-configured template using `@ISPC_FASTA_TASK_COUNT@`.
- [x] 2026-04-26: Swept `FASTA_TASK_COUNT` over 12, 16, 18, 24, 32, 36, 48, 64 with `FASTA_RENDERED_HEIGHT=576`; best measured values were 24 tasks at 0.9959 ms/frame and 48 tasks at 0.9978 ms/frame over 3 runs. Kept 24 as default.
- [x] 2026-04-26: Removed `prefetchw_l1` from `ispc_parallel_fasta`; SHA256 unchanged and performance was neutral at 0.9957 ms/frame over 5 runs, so kept as cleanup.
- [x] 2026-04-26: Specialized `ispc_parallel_fasta` kernel for fixed 800x600 while keeping the host `Shader` ABI; SHA256 unchanged and `fasta` improved to 0.9468 ms/frame over 5 runs.
- [x] 2026-04-26: Replaced `vec4 o/tanh/exp` final RGB path in `ispc_parallel_fasta` with three scalar varying RGB accumulators; SHA256 unchanged and `fasta` improved to 0.8925 ms/frame over 5 runs.
- [x] 2026-04-26: Rejected ISPC `--opt=fast-math` for exact-output mode: `fasta` SHA256 changed from `7DEE2F57EFAB20DFF495DB8AC99B9FA65522D74CA01AF51BC677A2B165C097F1` to `E66AA8D628834398A286A1E5B45B1B4DA7E97011554BE15FE474F8EF995DCA5E`.
- [x] 2026-04-26: Rejected ISPC `avx2-i32x16` for `fasta`: SHA256 unchanged, but benchmark regressed to 1.0459 ms/frame over 5 runs versus 0.8925 ms/frame for `avx2-i32x8`.
- [x] 2026-04-26: Re-swept task counts after RGB scalarization. Valid exact-output counts that divide `renderedHeight=576`: 16 -> 1.2734, 24 -> 0.8975, 32 -> 1.1743, 36 -> 1.1218, 48 -> 0.9013 ms/frame over 3 runs. Kept 24 as default.
- [x] 2026-04-26: Rejected scalarized `pX/pY/l` setup for `fasta`: exact SHA256 was preserved after matching source operation order, but benchmark regressed to 0.9567 ms/frame over 5 runs. Restored original `vec2 p/l` setup.
- [x] 2026-04-26: Rejected 64-byte aligned host allocation for `ispc_parallel_fasta`: SHA256 unchanged, but benchmark was neutral/slower at 0.8971 ms/frame over 5 runs versus 0.8925 ms/frame.
- [x] 2026-04-26: Rejected specialized exported `Shader800x600` host call: SHA256 unchanged, but benchmark was neutral/slower at 0.8956 ms/frame over 5 runs and added an ISPC export-call warning.
- [x] 2026-04-26: Accepted 64-byte aligned `fasta` output allocation together with ISPC `--opt=force-aligned-memory`: SHA256 unchanged and benchmark was 0.8920 ms/frame over 5 runs. Set the option as the `ispc_parallel_fasta` default.
- [x] 2026-04-26: Removed dead `vec4` helper code from `ispc_parallel_fasta` after RGB scalarization.
- [x] 2026-04-26: Final accepted `ispc_parallel_fasta` benchmark saved to `benchmark-ispc-fasta-final.csv`: `ispc_parallel` 1.2906 ms/frame, `ispc_parallel_fasta` 0.8943 ms/frame, `blazingfastcpp` 0.7683 ms/frame over 7 runs. `fasta` speedup vs original ISPC: 1.443x. Output SHA256 remained `7DEE2F57EFAB20DFF495DB8AC99B9FA65522D74CA01AF51BC677A2B165C097F1`.
- [x] 2026-04-26: Verified Linux/WSL build and smoke benchmark with locally unpacked Ubuntu `ispc` 1.22.0; `ispc_parallel_fasta` SHA256 matched `ispc_parallel`. `make` reported minor `/mnt/e` clock-skew warnings, but binaries built and ran successfully.
- [x] 2026-04-26: Added `render_images` CMake target and `scripts/benchmark.py --images-only` mode to write `ispc_parallel.ppm`, `ispc_parallel_fasta.ppm`, and `blazingfastcpp.ppm` into the project root for visual comparison.

## ispc_parallel_fasta Optimization Plan

Baseline target:

- Keep `ispc_parallel` as the immutable reference implementation.
- Use `ispc_parallel_fasta` for experimental ISPC changes.
- Compare all three implementations with `python scripts/benchmark.py --frames 300 --warmup 20 --runs 7 --omp-num-threads 24 --out-csv ...`.
- For every accepted `ispc_parallel_fasta` change, compare output against `ispc_parallel` first, then benchmark.

Important finding:

- Current ISPC row splitting uses `span = height / taskCount`, `begin = taskIndex * span`, `end = min(begin + span, height)`.
- If `height` is not divisible by `taskCount`, tail rows are not rendered. With 600 rows and common `num_cores()` values such as 32, rows after `span * taskCount` can remain zero.
- Fixing this changes output and may increase measured work. Treat this as a correctness task separate from speed tasks; benchmark both "exact clone" and "correct coverage" modes if needed.

Priority 1: Establish ISPC-specific validation

- Add a small image/checksum helper or script that renders `ispc_parallel` and `ispc_parallel_fasta` with identical options and reports SHA256.
- Add a visual/correctness note for the row-tail issue before optimizing. Decide whether `fasta` should preserve original output for apples-to-apples speed comparison or fix the missing rows for correctness.
- Add benchmark CSV names that include implementation and date, but keep generated CSV ignored by git.

Priority 2: Task decomposition and OpenMP overhead

- Test replacing `launch[num_cores()]` with a fixed or tuned task count: 8, 12, 16, 24, 32, 48, 64.
- Test row chunks instead of core-count chunks: one task per row block, for example 16 or 32 rows per task.
- Fix row partitioning with balanced integer division: `begin = taskIndex * height / taskCount`, `end = (taskIndex + 1) * height / taskCount`. Measure separately because it renders all rows.
- Compare ISPC task runtime modes in `tasksys.cpp`: current `ISPC_USE_OMP` vs default platform runtime and any lightweight custom/static row-loop approach.
- Consider a host-side loop over rows/tasks if ISPC `launch/sync` overhead dominates at 800x600.

Priority 3: Specialize shader constants for 800x600

- Create a fixed-size `Shader800x600` export in `ispc_parallel_fasta` with no uniform `width`/`height` parameters.
- Replace repeated `r = {width, height}` and divisions by `r.y` with constants for 800 and 600.
- Remove generic code paths that cannot trigger for width 800 and AVX2 program count.
- Keep the original exported `Shader` only if needed for compatibility; otherwise call the specialized export from `main.cpp`.

Priority 4: Remove dead or redundant math

- Mirror the accepted C++ finding: the final update of `v` in the 8th iteration is not used after accumulation. Check whether ISPC's `for` update expression still computes it on the last iteration; if yes, rewrite the loop to avoid final dead `cos` work.
- The `tmp` vector includes a 4th lane for alpha-like data that is not written to output. Investigate whether computing lane 4 of `o/tanh` can be avoided or if ISPC vector packing makes this impractical.
- Replace `tanh(5 * exp(...) / o)` with a positive-input specialized form only if output stays identical or the approximation is explicitly accepted.
- Profile or inspect generated assembly for repeated scalar transcendental calls caused by helper functions `sin(vec4)`, `cos(vec2)`, `exp(vec4)`, and `tanh(vec4)`.

Priority 5: ISPC compiler options

- Test `--opt=fast-math` if supported by the installed ISPC version, and compare output hash before accepting.
- Test targets: `avx2-i32x8`, `avx2-i32x16` if supported, and host CPU native variants where available.
- Test `--math-lib=fast` or equivalent ISPC math options if available in the local ISPC build.
- Record ISPC version in benchmark notes because Windows currently uses a newer trunk build while WSL used Ubuntu 1.22.0 during validation.

Priority 6: Host-side and memory behavior

- Replace `calloc` with aligned allocation for `pixels`, matching the C++ path, and measure whether ISPC stores benefit.
- Avoid writing output during benchmark; already handled by `--no-output`, but keep output path outside measured runtime.
- Check whether `prefetchw_l1` helps or hurts. The shader is math-heavy; prefetch may be noise or harmful on modern cores.
- Test single-buffer behavior is already inherent in ISPC path; no double-buffer comparison needed.

Priority 7: Acceptance criteria

- Build all targets on Windows: `cmake -S . -B build && cmake --build build --config Release`.
- Smoke benchmark all three targets: `python scripts/benchmark.py --frames 3 --warmup 1 --runs 2 --skip-build --build-dir build --omp-num-threads 24`.
- Full benchmark accepted changes: `python scripts/benchmark.py --frames 300 --warmup 20 --runs 7 --skip-build --build-dir build --omp-num-threads 24 --out-csv benchmark-ispc-fasta-after.csv`.
- Output regression: `ispc_parallel_fasta` should match `ispc_parallel` unless the task explicitly accepts a correctness fix or approximation; if it differs, document why.
- Update this TODO with accepted/rejected notes after each `fasta` experiment.

## Ground Rules

- Keep every optimization measurable. Use `python scripts/benchmark.py --frames 300 --warmup 20 --runs 7 --out-csv ...` before and after each change.
- Preserve visual output unless a task explicitly accepts approximate rendering. Add an image-diff or checksum mode before changing math approximations aggressively.
- Optimize the timed render path first: `ShaderTiledAVX` in `BlazingFastCPP_for_anton2920_stream/BlazingFastCPP_for_anton2920_stream.cpp`.
- Do not judge performance from a single run. Use average, median, min, max, and stddev; watch for thermal throttling and background scheduler noise.
- Treat `DumpPPM` as non-critical for benchmark mode because `--no-output` excludes it from measured runtime.

## Profiling First

- Add an optional profiler-friendly benchmark mode with longer runtime, for example `--frames 5000 --warmup 100 --no-output`, so VTune, AMD uProf, WPA, or Visual Studio Profiler get stable samples.
- Capture CPU counters for the hot loop: retired instructions, cycles, IPC, vector FP operations, FP divides, branch misses, L1/L2 misses, frontend stalls, backend stalls, and OpenMP runtime overhead.
- Build an assembly inspection target or documented command for MSVC and clang-cl so changes can be checked for spills, redundant broadcasts, missed FMA, and unexpected scalar fallback.
- Add a correctness harness that renders one frame from ISPC and C++ and reports max per-channel error, mean error, and optionally a PPM diff image.

## Priority 1: Reduce Hot Math Cost

- Replace the three final `v_tanh(5 * exp(x) / den)` calls with a fused approximation specialized for positive inputs. The current code does 3 `v_exp`, 3 `v_div`, then each `v_tanh` performs another `v_exp` and another `v_div`, so the final color map is likely one of the biggest costs.
- Derive or approximate `tanh(5 * exp(base) / den)` directly. Candidate approaches: rational approximation on the observed input range, piecewise polynomial with saturation, or lookup-assisted approximation if image quality tolerates it.
- Profile the input ranges for `baseR/baseG/baseB`, `oX/oY/oZ`, and final tanh arguments over multiple frames before choosing approximation intervals.
- Consider replacing `_mm256_div_ps` with reciprocal approximation plus one Newton-Raphson step for denominators that tolerate small error. The final path currently has several full vector divides.
- Split math quality levels behind a compile-time option: `BLAZINGFASTCPP_MATH_PRECISE`, `BLAZINGFASTCPP_MATH_FAST`, and possibly `BLAZINGFASTCPP_MATH_UNSAFE`.

## Priority 2: Optimize sin/cos Vector Math

- Audit `v_sin` and `v_cos`: each call performs argument reduction and computes both sine and cosine polynomials. The inner loop calls `v_sin(vx)`, `v_sin(vy)`, `v_cos(argX)`, and `v_cos(argY)` eight times per vector.
- Add a combined `sincos_reduced` helper that returns both sine and cosine when both are needed, and avoid duplicate range reduction if a future formulation can reuse reduced arguments.
- Investigate whether the shader can update `sin(vx)`, `sin(vy)`, `cos(argX)`, and `cos(argY)` with lower-degree approximations without visible regressions. Current polynomials may be more accurate than needed for a generative image.
- Replace blend-heavy quadrant selection with sign-bit XOR and permutation-friendly masks if assembly shows high shuffle/blend pressure.
- Pre-broadcast constants as `inline const __m256` where MSVC actually keeps them efficient, but verify assembly. The helper `set1()` inside math functions may generate repeated broadcasts or loads.

## Priority 3: Specialize for 800x600 and AVX2

- Create a specialized render path for fixed `WIDTH=800`, `HEIGHT=600`. Remove generic `width`, `height`, `tilesX`, `tilesY`, `xmax`, `ymax`, and scalar tail checks from the hot path.
- Since 800 is divisible by 8 and tiles start at multiples of 64, the scalar tail path is not used for the current image size. Move generic tail handling into a fallback path.
- Precompute per-row `py`, row-dependent constants, and possibly `base` components that do not depend on `x`. This is small but cheap to test.
- Precompute per-vector `px` lanes for all 100 vector groups per row if memory/cache impact is acceptable. This may reduce per-frame arithmetic but needs measurement because the shader is math-heavy.
- Mark hot functions force-inline for MSVC and clang/GCC through a small macro. Confirm that helpers are actually inlined and no call overhead remains in the inner loop.

## Priority 4: Register Pressure and Inner-Loop Structure

- Reduce live variable count in the unrolled 8-step block. The current block keeps `vx`, `vy`, `oX`, `oY`, `oZ`, `oW`, `diff`, `sX`, `sY`, `argX`, and `argY` live repeatedly, which can cause spills.
- Remove `oW` if it is not used for final output. It mirrors `oX` accumulation but is never consumed after the iterative block.
- Replace the manually duplicated 8-step block with a macro or templated helper only if the generated assembly remains fully unrolled. The current duplication is hard to maintain and easy to mis-optimize.
- Test two independent pixels-vector groups per loop iteration to expose more instruction-level parallelism around high-latency approximations and divides. Watch register pressure; this may help on wide out-of-order cores or hurt via spills.
- Reorder final RGB computations to overlap independent `v_exp`/division/tanh work with packing and stores.

## Priority 5: Parallelism and Scheduling

- Compare current tile scheduling (`64x64`, 130 OpenMP tasks per frame) against row-block scheduling. For a compute-bound shader, simpler static row chunks may reduce OpenMP overhead and improve cache predictability.
- Test tile sizes: `64x32`, `64x64`, `128x16`, `128x32`, and pure row ranges. Keep the best per CPU family if results differ.
- Add benchmark controls for `OMP_NUM_THREADS`, thread pinning, and schedule type. Record CPU model and thread count in benchmark output.
- Consider a persistent worker pool or one outer OpenMP parallel region across all frames in benchmark mode. Current code enters an OpenMP parallel region once per frame, which adds overhead at sub-millisecond frame times.
- Verify NUMA and affinity behavior on high-core-count systems. For 800x600, too many threads may add overhead rather than speed.

## Priority 6: Compiler and Build Flags

- Compare MSVC, clang-cl, and GCC/Clang on the same machine. AVX2 intrinsic code can differ substantially in scheduling and register allocation.
- Test MSVC `/O2` versus `/Ot`, `/GL`, `/Gw`, `/Gy`, and linker `/LTCG`. Keep only flags with measured benefit.
- For clang-cl or Clang/GCC, test `-O3`, `-ffast-math`, `-fno-math-errno`, `-march=x86-64-v3`, and explicit `-mavx2 -mfma`.
- Add CMake presets for reproducible benchmark builds: MSVC Release, clang-cl Release, and optionally native Release.
- Add a CPU feature guard at startup, or document that the binary requires AVX2+FMA.

## Priority 7: Memory, Stores, and Output

- The render path is mostly compute-bound, but keep stores aligned. Current frame buffers are 64-byte aligned and 800-pixel rows keep 32-byte vector stores aligned.
- Consider non-temporal stores only after profiling. At 800x600x4 bytes, frame output is small enough that normal cached stores are probably better.
- `DumpPPM` currently converts packed `uint32` pixels to RGB bytes through temporary stack arrays per 8 pixels. This is outside benchmark timing with `--no-output`, but can be optimized later with AVX2 byte shuffle/store if image output speed matters.
- Check whether double buffering is needed for benchmark mode. Rendering directly into one buffer may slightly reduce pointer swaps and cache footprint, but likely has tiny impact.

## Priority 8: Algorithmic Changes

- Compare the C++ formula against the ISPC shader line-by-line. Some current C++ choices may compute values that are not used, such as `oW`, or may differ from ISPC semantics in ways that accidentally add work.
- Explore moving invariant parts of the iterative formula out of the pixel loop. The time `t`, `k`, `1/k`, and lane constants are obvious; row and vector group constants may also be reusable.
- Evaluate lower iteration counts or adaptive iteration only if visual requirements allow it. The inner 8-step loop dominates work, so reducing it is high impact but changes the image.
- Consider AVX-512 as a separate future path, not as a replacement for the AVX2 path. Keep AVX2 as the baseline target.

## Validation Checklist Per Optimization

- Build: `cmake -S . -B build && cmake --build build --config Release`.
- Benchmark: `python scripts/benchmark.py --frames 300 --warmup 20 --runs 7 --out-csv benchmark-after.csv`.
- Compare to baseline: average ms/frame, median ms/frame, stddev, and speedup vs ISPC.
- Render check: run both binaries without `--no-output` and compare resulting images or checksums.
- Assembly check for hot changes: no unexpected scalar math calls, no excessive stack spills, no loss of FMA in hot path.
