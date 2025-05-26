uniform sampler2D fbo_texture;
varying vec2 f_texcoord;

void main(void) {
  gl_FragColor = texture2D(fbo_texture, f_texcoord) * 0.5 + vec4(0.5, 0.0, 0.5, 0.5);
}