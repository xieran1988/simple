
uniform sampler2D t;
uniform vec2 toff[25];

void main() {
//	float a = texture2D(t, gl_FragCoord.xy);
//	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
	gl_FragColor = texture2D(t, gl_FragCoord.xy);
return ;
	vec4 c = vec4(0.0);
	for (int i = 0; i < 25; i++) 
		c += texture2D(t, gl_FragCoord.xy + toff[i]);
	gl_FragColor = texture2D(t, gl_FragCoord.xy);
//	gl_FragColor = c/25.0;
}

