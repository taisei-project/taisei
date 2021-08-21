#version 330 core

#include "../lib/render_context.glslh"
#include "../interface/laser_pass1.glslh"

// Uneven capsule SDF derivation by Inigo Quilez
// https://iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm

// An uneven capsule is used instead of a simple segment/capsule SDF
// because we have to interpolate between different laser widths.

float cro(in vec2 a, in vec2 b) {
	return a.x * b.y - a.y * b.x;
}

float sdUnevenCapsule(in vec2 p, in vec2 pa, in vec2 pb, in float ra, in float rb) {
	p  -= pa;
	pb -= pa;
	float h = dot(pb, pb);
	vec2  q = vec2(dot(p, vec2(pb.y, -pb.x)), dot(p, pb)) / h;

	q.x = abs(q.x);

	float b = ra-rb;
	vec2  c = vec2(sqrt(h - b * b), b);

	float k = cro(c, q);
	float m = dot(c, q);
	float n = dot(q, q);

	     if( k < 0.0 ) return sqrt(h * (n                  )) - ra;
	else if( k > c.x ) return sqrt(h * (n + 1.0 - 2.0 * q.y)) - rb;
	                   return m                               - ra;
}

float sdCircle(in vec2 p, in vec2 c, in float r) {
	return length(c - p) - r;
}

void main(void) {
	vec2 a = segment.xy;
	vec2 b = segment.zw;

	// NOTE: rb >= ra
	float ra = segment_width.x;
	float rb = segment_width.y;

	float d;

	if(a == b) {
		d = sdCircle(coord, b, rb);
	} else {
		d = sdUnevenCapsule(coord, a, b, ra, rb);
	}

	fragOutput = vec4(d, segment_time, 0, 1);
}
