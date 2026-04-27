typedef float<2> vec2;

static inline float dot(vec2 a, vec2 b) { return a.x*b.x + a.y*b.y; }

static inline vec2 cos(vec2 v)  { vec2 r = {cos(v.x), cos(v.y)}; return r; }
static inline float tanh(float v) { return (exp(2.*v) - 1.) / (exp(2.*v) + 1.); }

task
void
ShaderLine(uniform uint32 pixels[], uniform float t)
{
	const uniform int width = 800;
	const uniform int height = 600;
	const uniform int renderedHeight = @ISPC_FASTA_RENDERED_HEIGHT@;
	const uniform int span = renderedHeight / taskCount;
	const uniform int begin = taskIndex * span;
	const uniform int end = min(begin + span, renderedHeight);

	const uniform vec2 r = {width, height};
	for (uniform int y = begin; y < end; y++) {
		#pragma unroll 8
		foreach (x = 0 ... width) {
			float oR = 0;
			float oG = 0;
			float oB = 0;
			const vec2 FC = {x, height-y};

			uniform vec2 i = 0;
			const vec2 p = (FC * 2 - r) / r.y;
			const vec2 l = 4 - 4 * abs(0.7 - dot(p, p));
			vec2 v = p * l;

			#pragma unroll 8
			for (; i.y < 8;) {
				i.y++;
				v += cos(v.yx * i.y + i + t) / i.y + 0.7;
				const float diff = abs(v.x - v.y);
				oR += (sin(v.x) + 1) * diff;
				const float yTerm = (sin(v.y) + 1) * diff;
				oG += yTerm;
				oB += yTerm;
			}
			const float base = l.x - 4;
			const float rgbR = tanh(5 * exp(base + p.y) / oR);
			const float rgbG = tanh(5 * exp(base - p.y) / oG);
			const float rgbB = tanh(5 * exp(base - 2 * p.y) / oB);

			#pragma ignore warning(perf)
			pixels[y * width + x] = (((uint32)(rgbR*255)) << 24) | (((uint32)(rgbG*255)) << 16) | (((uint32)(rgbB*255)) << 8);
		}
	}
}


export
void
Shader(uniform uint32 pixels[], uniform int width, uniform int height, uniform float time)
{
	launch[@ISPC_FASTA_TASK_COUNT@] ShaderLine(pixels, time);
}
