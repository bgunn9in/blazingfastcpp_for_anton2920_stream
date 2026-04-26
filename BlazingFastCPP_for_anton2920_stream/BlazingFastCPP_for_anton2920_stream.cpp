#include <immintrin.h>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "benchmark_options.h"

#define WIDTH 800
#define HEIGHT 600
using uint32 = unsigned int;

static inline void* malloc_aligned(size_t size, size_t alignment) {
#ifdef _MSC_VER
	return _aligned_malloc(size, alignment);
#else
	if (size % alignment != 0) {
		size += alignment - (size % alignment);
	}
	return std::aligned_alloc(alignment, size);
#endif
}

static inline void free_aligned(void* p) {
#ifdef _MSC_VER
	_aligned_free(p);
#else
	std::free(p);
#endif
}

static inline FILE* fopen_cross(const char* filename, const char* mode) {
#ifdef _MSC_VER
	FILE* f = nullptr;
	if (fopen_s(&f, filename, mode) != 0) return nullptr;
	return f;
#else
	return std::fopen(filename, mode);
#endif
}

namespace vecmath {

	static inline __m256 set1(float x) { return _mm256_set1_ps(x); }

	static const float INV_PI_2 = 0.636619772367581343075535f;
	static const float PIO2_1 = 1.5707962512969970703125f;
	static const float PIO2_2 = 7.549789415861596353352e-8f;

	static const float S1 = -1.6666667163e-1f;
	static const float S2 = 8.3333337680e-3f;
	static const float S3 = -1.9841270114e-4f;
	static const float S4 = 2.7557314297e-6f;

	static const float C2 = -5.0000000000e-1f;
	static const float C4 = 4.1666665673e-2f;
	static const float C6 = -1.3888889225e-3f;
	static const float C8 = 2.4801587642e-5f;

	static inline void reduce_approx(__m256 x, __m256& r, __m256i& q) {
		__m256 y = _mm256_mul_ps(x, set1(INV_PI_2));
		__m256 y_round = _mm256_round_ps(y, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
		__m256i n = _mm256_cvtps_epi32(y_round);
		__m256 nf = _mm256_cvtepi32_ps(n);

		r = _mm256_sub_ps(x, _mm256_fmadd_ps(nf, set1(PIO2_1), _mm256_mul_ps(nf, set1(PIO2_2))));
		q = _mm256_and_si256(n, _mm256_set1_epi32(3));
	}

	static inline __m256 sin_poly(__m256 r) {
		__m256 r2 = _mm256_mul_ps(r, r);
		__m256 r3 = _mm256_mul_ps(r2, r);
		__m256 r5 = _mm256_mul_ps(r3, r2);
		__m256 r7 = _mm256_mul_ps(r5, r2);
		__m256 r9 = _mm256_mul_ps(r7, r2);
		__m256 t1 = _mm256_fmadd_ps(set1(S1), r3, r);
		__m256 t2 = _mm256_fmadd_ps(set1(S2), r5, t1);
		__m256 t3 = _mm256_fmadd_ps(set1(S3), r7, t2);
		__m256 t4 = _mm256_fmadd_ps(set1(S4), r9, t3);
		return t4;
	}

	static inline __m256 cos_poly(__m256 r) {
		__m256 r2 = _mm256_mul_ps(r, r);
		__m256 r4 = _mm256_mul_ps(r2, r2);
		__m256 r6 = _mm256_mul_ps(r4, r2);
		__m256 r8 = _mm256_mul_ps(r4, r4);
		__m256 c = set1(1.0f);
		c = _mm256_fmadd_ps(set1(C2), r2, c);
		c = _mm256_fmadd_ps(set1(C4), r4, c);
		c = _mm256_fmadd_ps(set1(C6), r6, c);
		c = _mm256_fmadd_ps(set1(C8), r8, c);
		return c;
	}

	static inline __m256 v_sin(__m256 x) {
		__m256 r; __m256i q; reduce_approx(x, r, q);
		__m256 s = sin_poly(r);
		__m256 c = cos_poly(r);
		__m256 is_q1 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(q, _mm256_set1_epi32(1)));
		__m256 is_q2 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(q, _mm256_set1_epi32(2)));
		__m256 is_q3 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(q, _mm256_set1_epi32(3)));
		__m256 sin_val = _mm256_blendv_ps(s, c, is_q1);
		__m256 sign = _mm256_and_ps(_mm256_or_ps(is_q2, is_q3), _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000)));
		return _mm256_xor_ps(sin_val, sign);
	}

	static inline __m256 v_cos(__m256 x) {
		__m256 r; __m256i q; reduce_approx(x, r, q);
		__m256 s = sin_poly(r);
		__m256 c = cos_poly(r);
		__m256 is_q1 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(q, _mm256_set1_epi32(1)));
		__m256 is_q2 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(q, _mm256_set1_epi32(2)));
		__m256 is_q3 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(q, _mm256_set1_epi32(3)));
		__m256 use_s = _mm256_or_ps(is_q1, is_q3);
		__m256 base = _mm256_blendv_ps(c, s, use_s);
		__m256 sign = _mm256_and_ps(_mm256_or_ps(is_q1, is_q2), _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000)));
		return _mm256_xor_ps(base, sign);
	}

	static const float INV_LN2 = 1.44269504088896341f;
	static const float LN2_HI = 0.693147182464599609375f;
	static const float LN2_LO = 1.428606765330187045e-6f;
	static const float EXP_LOWER_CLAMP = -87.0f;
	static const float EXP_UPPER_CLAMP = 88.0f;

	static inline __m256 exp_poly(__m256 r) {
		__m256 r2 = _mm256_mul_ps(r, r);
		__m256 r3 = _mm256_mul_ps(r2, r);
		__m256 r4 = _mm256_mul_ps(r2, r2);
		__m256 r5 = _mm256_mul_ps(r4, r);
		__m256 t = _mm256_add_ps(set1(1.0f), r);
		t = _mm256_fmadd_ps(set1(0.5f), r2, t);
		t = _mm256_fmadd_ps(set1(1.0f / 6.0f), r3, t);
		t = _mm256_fmadd_ps(set1(1.0f / 24.0f), r4, t);
		t = _mm256_fmadd_ps(set1(1.0f / 120.0f), r5, t);
		return t;
	}

	static inline __m256 v_exp(__m256 x) {
		x = _mm256_max_ps(x, set1(EXP_LOWER_CLAMP));
		x = _mm256_min_ps(x, set1(EXP_UPPER_CLAMP));
		__m256 y = _mm256_mul_ps(x, set1(INV_LN2));
		__m256 y_round = _mm256_round_ps(y, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
		__m256i ki = _mm256_cvtps_epi32(y_round);
		__m256 kf = _mm256_cvtepi32_ps(ki);
		__m256 r = _mm256_sub_ps(x, _mm256_fmadd_ps(kf, set1(LN2_HI), _mm256_mul_ps(kf, set1(LN2_LO))));
		__m256 er = exp_poly(r);
		__m256i e = _mm256_add_epi32(ki, _mm256_set1_epi32(127));
		e = _mm256_max_epi32(e, _mm256_set1_epi32(1));
		e = _mm256_min_epi32(e, _mm256_set1_epi32(254));
		__m256i ebits = _mm256_slli_epi32(e, 23);
		__m256 pow2k = _mm256_castsi256_ps(ebits);
		return _mm256_mul_ps(er, pow2k);
	}

	static inline __m256 v_tanh(__m256 x) {
		__m256 ax = _mm256_and_ps(x, _mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff)));
		__m256 is_big = _mm256_cmp_ps(ax, set1(9.0f), _CMP_GT_OQ);
		__m256 e2x = v_exp(_mm256_add_ps(ax, ax));
		__m256 num = _mm256_sub_ps(e2x, set1(1.0f));
		__m256 den = _mm256_add_ps(e2x, set1(1.0f));
		__m256 base = _mm256_div_ps(num, den);
		__m256 saturated = set1(1.0f);
		__m256 t = _mm256_blendv_ps(base, saturated, is_big);
		__m256 signmask = _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000));
		__m256 one = set1(1.0f);
		__m256 neg_one = _mm256_xor_ps(one, _mm256_and_ps(x, signmask));
		return _mm256_mul_ps(t, neg_one);
	}

	static inline __m256 v_tanh_positive(__m256 x) {
		__m256 is_big = _mm256_cmp_ps(x, set1(9.0f), _CMP_GT_OQ);
		__m256 e2x = v_exp(_mm256_add_ps(x, x));
		__m256 num = _mm256_sub_ps(e2x, set1(1.0f));
		__m256 den = _mm256_add_ps(e2x, set1(1.0f));
		__m256 base = _mm256_div_ps(num, den);
		return _mm256_blendv_ps(base, set1(1.0f), is_big);
	}

}

inline __m256 vabs(__m256 x) {
	const __m256 mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff));
	return _mm256_and_ps(x, mask);
}

inline void store_rgb8_u32_aligned(uint32* dst, int index, __m256 r, __m256 g, __m256 b) {
	const __m256 scale = _mm256_set1_ps(255.0f);
	r = _mm256_mul_ps(r, scale);
	g = _mm256_mul_ps(g, scale);
	b = _mm256_mul_ps(b, scale);

	const __m256i Ri = _mm256_cvtps_epi32(r);
	const __m256i Gi = _mm256_cvtps_epi32(g);
	const __m256i Bi = _mm256_cvtps_epi32(b);

	const __m256i Rsh = _mm256_slli_epi32(Ri, 24);
	const __m256i Gsh = _mm256_slli_epi32(Gi, 16);
	const __m256i Bsh = _mm256_slli_epi32(Bi, 8);
	const __m256i packed = _mm256_or_si256(_mm256_or_si256(Rsh, Gsh), Bsh);

	_mm256_store_si256((__m256i*)(dst + index), packed);
}

static inline void ShaderTiledAVX(uint32* pixels, float t) {
	constexpr int width = WIDTH;
	constexpr int height = HEIGHT;
	static_assert((WIDTH % 8) == 0, "Fixed AVX2 renderer requires width divisible by 8.");

	constexpr float rw = (float)WIDTH;
	constexpr float rh = (float)HEIGHT;

	const __m256 two = _mm256_set1_ps(2.0f);
	const __m256 four = _mm256_set1_ps(4.0f);
	const __m256 one = _mm256_set1_ps(1.0f);
	const __m256 c0p7 = _mm256_set1_ps(0.7f);
	const __m256 eps = _mm256_set1_ps(1e-12f);
	const __m256 five = _mm256_set1_ps(5.0f);
	const __m256 tV = _mm256_set1_ps(t);
	const __m256 rwV = _mm256_set1_ps(rw);
	const __m256 rhV = _mm256_set1_ps(rh);
	const __m256 inv_rh = _mm256_set1_ps(1.0f / rh);

	const __m256 lane = _mm256_set_ps(7.f, 6.f, 5.f, 4.f, 3.f, 2.f, 1.f, 0.f);
	const __m256 step8 = _mm256_set1_ps(8.0f);

	__m256 IK[8], INVIK[8];
	for (int k = 0; k < 8; ++k) {
		float kk = (float)(k + 1);
		IK[k] = _mm256_set1_ps(kk);
		INVIK[k] = _mm256_set1_ps(1.0f / kk);
	}

#pragma omp parallel for schedule(static)
	for (int y = 0; y < height; ++y) {
			const float fy = (float)(height - y);
			const __m256 fyV = _mm256_set1_ps(fy);

			const __m256 py = _mm256_mul_ps(_mm256_sub_ps(_mm256_mul_ps(fyV, two), rhV), inv_rh);

			int x = 0;
			__m256 baseX = _mm256_setzero_ps();
			__m256 fx = _mm256_add_ps(baseX, lane);

			for (; x < width; x += 8) {
				__m256 px = _mm256_mul_ps(_mm256_sub_ps(_mm256_mul_ps(fx, two), rwV), inv_rh);

				__m256 dotp = _mm256_fmadd_ps(px, px, _mm256_mul_ps(py, py));

				__m256 l = _mm256_sub_ps(four, _mm256_mul_ps(four, vabs(_mm256_sub_ps(c0p7, dotp))));

				__m256 vx = _mm256_mul_ps(px, l);
				__m256 vy = _mm256_mul_ps(py, l);

				__m256 oX = _mm256_setzero_ps();
				__m256 oY = _mm256_setzero_ps();
				__m256 oZ = _mm256_setzero_ps();

				{
					__m256 diff = vabs(_mm256_sub_ps(vx, vy));
					__m256 sX = _mm256_add_ps(vecmath::v_sin(vx), one);
					__m256 sY = _mm256_add_ps(vecmath::v_sin(vy), one);
					oX = _mm256_fmadd_ps(sX, diff, oX);
					oY = _mm256_fmadd_ps(sY, diff, oY);
					oZ = _mm256_fmadd_ps(sY, diff, oZ);
					__m256 argX = _mm256_add_ps(_mm256_mul_ps(vy, IK[0]), tV);
					__m256 argY = _mm256_add_ps(_mm256_mul_ps(vx, IK[0]), _mm256_add_ps(IK[0], tV));
					vx = _mm256_fmadd_ps(vecmath::v_cos(argX), INVIK[0], _mm256_add_ps(vx, c0p7));
					vy = _mm256_fmadd_ps(vecmath::v_cos(argY), INVIK[0], _mm256_add_ps(vy, c0p7));

					diff = vabs(_mm256_sub_ps(vx, vy));
					sX = _mm256_add_ps(vecmath::v_sin(vx), one);
					sY = _mm256_add_ps(vecmath::v_sin(vy), one);
					oX = _mm256_fmadd_ps(sX, diff, oX);
					oY = _mm256_fmadd_ps(sY, diff, oY);
					oZ = _mm256_fmadd_ps(sY, diff, oZ);
					argX = _mm256_add_ps(_mm256_mul_ps(vy, IK[1]), tV);
					argY = _mm256_add_ps(_mm256_mul_ps(vx, IK[1]), _mm256_add_ps(IK[1], tV));
					vx = _mm256_fmadd_ps(vecmath::v_cos(argX), INVIK[1], _mm256_add_ps(vx, c0p7));
					vy = _mm256_fmadd_ps(vecmath::v_cos(argY), INVIK[1], _mm256_add_ps(vy, c0p7));

					diff = vabs(_mm256_sub_ps(vx, vy));
					sX = _mm256_add_ps(vecmath::v_sin(vx), one);
					sY = _mm256_add_ps(vecmath::v_sin(vy), one);
					oX = _mm256_fmadd_ps(sX, diff, oX);
					oY = _mm256_fmadd_ps(sY, diff, oY);
					oZ = _mm256_fmadd_ps(sY, diff, oZ);
					argX = _mm256_add_ps(_mm256_mul_ps(vy, IK[2]), tV);
					argY = _mm256_add_ps(_mm256_mul_ps(vx, IK[2]), _mm256_add_ps(IK[2], tV));
					vx = _mm256_fmadd_ps(vecmath::v_cos(argX), INVIK[2], _mm256_add_ps(vx, c0p7));
					vy = _mm256_fmadd_ps(vecmath::v_cos(argY), INVIK[2], _mm256_add_ps(vy, c0p7));

					diff = vabs(_mm256_sub_ps(vx, vy));
					sX = _mm256_add_ps(vecmath::v_sin(vx), one);
					sY = _mm256_add_ps(vecmath::v_sin(vy), one);
					oX = _mm256_fmadd_ps(sX, diff, oX);
					oY = _mm256_fmadd_ps(sY, diff, oY);
					oZ = _mm256_fmadd_ps(sY, diff, oZ);
					argX = _mm256_add_ps(_mm256_mul_ps(vy, IK[3]), tV);
					argY = _mm256_add_ps(_mm256_mul_ps(vx, IK[3]), _mm256_add_ps(IK[3], tV));
					vx = _mm256_fmadd_ps(vecmath::v_cos(argX), INVIK[3], _mm256_add_ps(vx, c0p7));
					vy = _mm256_fmadd_ps(vecmath::v_cos(argY), INVIK[3], _mm256_add_ps(vy, c0p7));

					diff = vabs(_mm256_sub_ps(vx, vy));
					sX = _mm256_add_ps(vecmath::v_sin(vx), one);
					sY = _mm256_add_ps(vecmath::v_sin(vy), one);
					oX = _mm256_fmadd_ps(sX, diff, oX);
					oY = _mm256_fmadd_ps(sY, diff, oY);
					oZ = _mm256_fmadd_ps(sY, diff, oZ);
					argX = _mm256_add_ps(_mm256_mul_ps(vy, IK[4]), tV);
					argY = _mm256_add_ps(_mm256_mul_ps(vx, IK[4]), _mm256_add_ps(IK[4], tV));
					vx = _mm256_fmadd_ps(vecmath::v_cos(argX), INVIK[4], _mm256_add_ps(vx, c0p7));
					vy = _mm256_fmadd_ps(vecmath::v_cos(argY), INVIK[4], _mm256_add_ps(vy, c0p7));

					diff = vabs(_mm256_sub_ps(vx, vy));
					sX = _mm256_add_ps(vecmath::v_sin(vx), one);
					sY = _mm256_add_ps(vecmath::v_sin(vy), one);
					oX = _mm256_fmadd_ps(sX, diff, oX);
					oY = _mm256_fmadd_ps(sY, diff, oY);
					oZ = _mm256_fmadd_ps(sY, diff, oZ);
					argX = _mm256_add_ps(_mm256_mul_ps(vy, IK[5]), tV);
					argY = _mm256_add_ps(_mm256_mul_ps(vx, IK[5]), _mm256_add_ps(IK[5], tV));
					vx = _mm256_fmadd_ps(vecmath::v_cos(argX), INVIK[5], _mm256_add_ps(vx, c0p7));
					vy = _mm256_fmadd_ps(vecmath::v_cos(argY), INVIK[5], _mm256_add_ps(vy, c0p7));

					diff = vabs(_mm256_sub_ps(vx, vy));
					sX = _mm256_add_ps(vecmath::v_sin(vx), one);
					sY = _mm256_add_ps(vecmath::v_sin(vy), one);
					oX = _mm256_fmadd_ps(sX, diff, oX);
					oY = _mm256_fmadd_ps(sY, diff, oY);
					oZ = _mm256_fmadd_ps(sY, diff, oZ);
					argX = _mm256_add_ps(_mm256_mul_ps(vy, IK[6]), tV);
					argY = _mm256_add_ps(_mm256_mul_ps(vx, IK[6]), _mm256_add_ps(IK[6], tV));
					vx = _mm256_fmadd_ps(vecmath::v_cos(argX), INVIK[6], _mm256_add_ps(vx, c0p7));
					vy = _mm256_fmadd_ps(vecmath::v_cos(argY), INVIK[6], _mm256_add_ps(vy, c0p7));

					diff = vabs(_mm256_sub_ps(vx, vy));
					sX = _mm256_add_ps(vecmath::v_sin(vx), one);
					sY = _mm256_add_ps(vecmath::v_sin(vy), one);
					oX = _mm256_fmadd_ps(sX, diff, oX);
					oY = _mm256_fmadd_ps(sY, diff, oY);
					oZ = _mm256_fmadd_ps(sY, diff, oZ);
				}

				__m256 base = _mm256_sub_ps(l, four);
				__m256 baseR = _mm256_add_ps(base, py);
				__m256 baseG = _mm256_sub_ps(base, py);
				__m256 baseB = _mm256_sub_ps(base, _mm256_add_ps(py, py));

				__m256 eR = vecmath::v_exp(baseR);
				__m256 eG = vecmath::v_exp(baseG);
				__m256 eB = vecmath::v_exp(baseB);

				__m256 dR = _mm256_max_ps(oX, eps);
				__m256 dG = _mm256_max_ps(oY, eps);
				__m256 dB = _mm256_max_ps(oZ, eps);

				__m256 r = vecmath::v_tanh_positive(_mm256_div_ps(_mm256_mul_ps(eR, five), dR));
				__m256 g = vecmath::v_tanh_positive(_mm256_div_ps(_mm256_mul_ps(eG, five), dG));
				__m256 b = vecmath::v_tanh_positive(_mm256_div_ps(_mm256_mul_ps(eB, five), dB));

				store_rgb8_u32_aligned(pixels, y * width + x, r, g, b);

				baseX = _mm256_add_ps(baseX, step8);
				fx = _mm256_add_ps(baseX, lane);
			}
		}
}

int DumpPPM(const char* filename, const uint32* pixels, int width, int height) {
	FILE* out = fopen_cross(filename, "wb");
	if (!out) { std::perror("Failed to open file"); return 1; }

	std::fprintf(out, "P6\n%d %d\n255\n", width, height);

	const int chunkRows = 32;
	const size_t rowBytes = (size_t)width * 3;
	const size_t chunkBytes = rowBytes * chunkRows;

	unsigned char* chunk = (unsigned char*)malloc_aligned(chunkBytes, 64);
	if (!chunk) { std::fclose(out); return 1; }

	int y = 0;
	while (y < height) {
		int rows = std::min(chunkRows, height - y);
		for (int r = 0; r < rows; ++r) {
			const uint32* p = &pixels[(size_t)(y + r) * width];
			unsigned char* dst = chunk + (size_t)r * rowBytes;
			int x = 0;
			for (; x + 8 <= width; x += 8) {
				__m256i px = _mm256_load_si256((const __m256i*)(p + x));
				__m256i R = _mm256_srli_epi32(px, 24);
				__m256i G = _mm256_and_si256(_mm256_srli_epi32(px, 16), _mm256_set1_epi32(0xFF));
				__m256i B = _mm256_and_si256(_mm256_srli_epi32(px, 8), _mm256_set1_epi32(0xFF));

				alignas(32) uint32 r8[8], g8[8], b8[8];
				_mm256_store_si256((__m256i*)r8, R);
				_mm256_store_si256((__m256i*)g8, G);
				_mm256_store_si256((__m256i*)b8, B);

				for (int i = 0; i < 8; ++i) {
					dst[(x + i) * 3 + 0] = (unsigned char)r8[i];
					dst[(x + i) * 3 + 1] = (unsigned char)g8[i];
					dst[(x + i) * 3 + 2] = (unsigned char)b8[i];
				}
			}
			for (; x < width; ++x) {
				uint32 pxu = p[x];
				dst[x * 3 + 0] = (unsigned char)((pxu >> 24) & 0xFF);
				dst[x * 3 + 1] = (unsigned char)((pxu >> 16) & 0xFF);
				dst[x * 3 + 2] = (unsigned char)((pxu >> 8) & 0xFF);
			}
		}

		std::fwrite(chunk, 1, (size_t)rows * rowBytes, out);
		y += rows;
	}

	free_aligned(chunk);
	std::fclose(out);
	return 0;
}

struct FrameBuffers {
	uint32* A = nullptr;
	uint32* B = nullptr;
	FrameBuffers(size_t count) {
		A = (uint32*)malloc_aligned(count * sizeof(uint32), 64);
		B = (uint32*)malloc_aligned(count * sizeof(uint32), 64);
		std::memset(A, 0, count * sizeof(uint32));
		std::memset(B, 0, count * sizeof(uint32));
	}
	~FrameBuffers() {
		free_aligned(A);
		free_aligned(B);
	}
	uint32* front() { return A; }
	uint32* back() { return B; }
	void swap() { std::swap(A, B); }
};

int main(int argc, char** argv) {
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

	FrameBuffers fb{ (size_t)WIDTH * HEIGHT };

	float fi = 0.f;
	for (int i = 0; i < options.warmup; ++i) {
		ShaderTiledAVX(fb.back(), fi);
		fb.swap();
		fi += 1.f;
	}

	const auto start = std::chrono::steady_clock::now();
	for (int i = 0; i < options.frames; ++i) {
		ShaderTiledAVX(fb.back(), fi);
		fb.swap();
		fi += 1.f;
	}
	const auto end = std::chrono::steady_clock::now();

	const double totalMs = std::chrono::duration<double, std::milli>(end - start).count();
	PrintBenchmarkResult("blazingfastcpp", WIDTH, HEIGHT, options, totalMs);

	if (options.writeOutput) {
		ShaderTiledAVX(fb.front(), 0.0f);
		DumpPPM(options.output, fb.front(), WIDTH, HEIGHT);
	}
	return 0;
}
