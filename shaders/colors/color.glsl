uniform sampler2D fbo_texture;
uniform vec4 color;
varying vec2 f_texcoord;

void main(void) {
  gl_FragColor = texture2D(fbo_texture, f_texcoord) * 0.5 + color * 0.5;
}