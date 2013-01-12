
uniform sampler2D t;
uniform float s;

void main() 
{
	vec4 c = gl_FragCoord;
	gl_FragData[1] = vec4(vec3(1.0,1.0,1.0)*s,1.0);
}

