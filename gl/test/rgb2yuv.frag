
uniform sampler2D t;
uniform int w;
uniform int h;

//    float y =  0.299*tex.r + 0.587*tex.g + 0.114*tex.b;
//    float u = -0.168*tex.r - 0.330*tex.g + 0.498*tex.b + 0.5;
//    float v =  0.498*tex.r - 0.417*tex.g - 0.081*tex.b + 0.5;

vec4 reduce(vec3 m, int x, int y, int step, float inc)
{
	int i;
	vec4 r;
	for (i = 0; i < 4; i++) {
		r[i] = dot(m, texture2D(t, 
			vec2(float(x+i*step)/float(w), float(y)/float(h))).rgb
		) + inc;
	}
	return r;
}

void main() 
{ 
	vec4 c = gl_FragCoord;
	vec3 my = vec3(0.299, 0.587, 0.114); 
	vec3 mu = vec3(-0.168, -0.330, 0.498);
	vec3 mv = vec3(0.498, -0.417, -0.081);
	int p = int(c.x) + int(c.y)*w;
	int s = w*h;
	int sy = s/4;
	int su = sy/4;
	int sv = su;

	if (p < sy) {
		p = p * 4;
		int y = p/w;
		int x = p - y*w;
		gl_FragData[1] = reduce(my, x, y, 1, 0.0);
	} else if (p < sy+su) {
		p = (p - sy)*4;
		int y = p/(w/2);
		int x = p - y*(w/2);
		gl_FragData[1] = reduce(mu, x*2, y*2, 2, 0.5);
	} else if (p < sy+su+sv) {
		p = (p - sy - su)*4;
		int y = p/(w/2);
		int x = p - y*(w/2);
		gl_FragData[1] = reduce(mv, x*2, y*2, 2, 0.5);
	}
}

