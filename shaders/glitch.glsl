uniform sampler2D fbo_texture;
varying vec2 f_texcoord;

void main(void) {
  vec4 left = texture2D(fbo_texture, f_texcoord - vec2(0.02, 0));
  vec4 right = texture2D(fbo_texture, f_texcoord + vec2(0.02, 0));
  gl_FragColor = vec4(left.r, right.g, right.b, left.a * 0.5 + right.a * 0.5);
}