
uniform sampler2D t;

//    float y =  0.299*tex.r + 0.587*tex.g + 0.114*tex.b;
//    float u = -0.168*tex.r - 0.330*tex.g + 0.498*tex.b + 0.5;
//    float v =  0.498*tex.r - 0.417*tex.g - 0.081*tex.b + 0.5;

void main() 
{ 
	vec4 c = gl_FragCoord;
	float y = dot(vec3(0.299, 0.587, 0.114), texture2D(t, gl_TexCoord[0].xy).rgb);
	float u = dot(vec3(-0.168, -0.330, 0.498), texture2D(t, gl_TexCoord[0].xy).rgb) + 0.5;
	float v = dot(vec3(0.498, -0.417, -0.081), texture2D(t, gl_TexCoord[0].xy).rgb) + 0.5;
	gl_FragData[1] = vec4(y, y, y, 1.0);
	gl_FragData[2] = vec4(u, u, u, 1.0);
	gl_FragData[3] = vec4(v, v, v, 1.0);
}

