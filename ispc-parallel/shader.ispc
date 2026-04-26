typedef float<2> vec2;
typedef float<4> vec4;

static inline float dot(vec2 a, vec2 b) { return a.x*b.x + a.y*b.y; }

static inline vec4 sin(vec4 v)  { vec4 r = {sin(v.x), sin(v.y), sin(v.z), sin(v.w)}; return r; }
static inline vec2 cos(vec2 v)  { vec2 r = {cos(v.x), cos(v.y)}; return r; }
static inline vec4 exp(vec4 v)  { vec4 r = {exp(v.x), exp(v.y), exp(v.z), exp(v.w)}; return r; }
static inline vec4 tanh(vec4 v) { return (exp(2.*v) - 1.) / (exp(2.*v) + 1.); }

task
void
ShaderLine(uniform uint32 pixels[], uniform int width, uniform int height, uniform float t)
{
	const uniform int span = height / taskCount;
	const uniform int begin = taskIndex * span;
	const uniform int end = min(begin + span, height);

	const uniform vec2 r = {width, height};
	for (uniform int y = begin; y < end; y++) {
		prefetchw_l1(&pixels[y * width]);
		prefetchw_l1(&pixels[y * width + programCount]);

		#pragma unroll 8
		foreach (x = 0 ... width) {
			vec4 o = 0;
			const vec2 FC = {x, height-y};

			uniform vec2 i = 0;
			const vec2 p = (FC * 2 - r) / r.y;
			const vec2 l = 4 - 4 * abs(0.7 - dot(p, p));
			vec2 v = p * l;

			#pragma unroll 8
			for (; i.y < 8; o += (sin(v.xyyx) + 1) * abs(v.x - v.y)) {
				i.y++;
				v += cos(v.yx * i.y + i + t) / i.y + 0.7;
			}
			const uniform vec4 tmp = {-1, 1, 2, 0};
			o = tanh(5 * exp(l.x - 4 - p.y * tmp) / o);

			#pragma ignore warning(perf)
			pixels[y * width + x] = (((uint32)(o.r*255)) << 24) | (((uint32)(o.g*255)) << 16) | (((uint32)(o.b*255)) << 8);
		}
	}
}


export
void
Shader(uniform uint32 pixels[], uniform int width, uniform int height, uniform float time)
{
	launch[num_cores()] ShaderLine(pixels, width, height, time);
}

