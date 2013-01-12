
uniform sampler2D t;

//    float y =  0.299*tex.r + 0.587*tex.g + 0.114*tex.b;
//    float u = -0.168*tex.r - 0.330*tex.g + 0.498*tex.b + 0.5;
//    float v =  0.498*tex.r - 0.417*tex.g - 0.081*tex.b + 0.5;

vec4 reduce(vec3 m, float x, float y, float w, float h, float inc)
{
	int i;
	vec4 r;
	for (i = 0; i < 4; i++) {
		r[i] = dot(m, texture2D(t, vec2((x+float(i))/w, y/h)).rgb) + inc;
	}
	return r;
}

void main() 
{ 
	vec4 c = gl_FragCoord;
	vec3 my = vec3(0.299, 0.587, 0.114); 
	vec3 mu = vec3(-0.168, -0.330, 0.498);
	vec3 mv = vec3(0.498, -0.417, -0.081);
	float w = 32.0;
	float h = 32.0;
	float pos = c.x + c.y*h;
	float size = w*h;
	if (pos < size*0.25) {
		// Y data
		float p = pos*4.0;
		float y = floor(p/h);
		float x = p - y*h;
		gl_FragData[1] = vec4(x/32.0,0.0,0.0,1.0);
		gl_FragData[1] = texture2D(t, vec2(x/w,y/h));
		gl_FragData[1] = reduce(my, x, y, w, h, 0.0);
	} else if (pos < size*0.375) {
		//gl_FragData[1] = vec4(1.0,0.0,0.0,1.0);
		float p = (pos - size*0.25)*16.0;
		float y = floor(p/h);
		float x = p - y*h;
		gl_FragData[1] = reduce(mu, x, y, w, h, 0.5);
	} else if (pos < size*0.5) {
		//gl_FragData[1] = vec4(0.0,1.0,0.0,1.0);
		float p = (pos - size*0.375)*16.0;
		float y = floor(p/h);
		float x = p - y*h;
		gl_FragData[1] = reduce(mv, x, y, w, h, 0.5);
	}
}

