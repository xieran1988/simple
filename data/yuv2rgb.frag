
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
void main() {
	vec2 c0 = gl_TexCoord[0].st;
	vec2 c = vec2(c0.x, 1.0 - c0.y);
	float y = texture2D(tex1, c).r;
	float u = texture2D(tex2, c).r - 0.5;
	float v = texture2D(tex3, c).r - 0.5;
	float r = y + 1.402 * v;  
	float g = y - 0.344 * u - 0.714 * v;  
	float b = y + 1.772 * u;       
	gl_FragColor = vec4(r,g,b,1.0);
}

