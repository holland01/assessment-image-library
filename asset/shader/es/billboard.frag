varying vec4 frag_Color;
varying vec2 frag_TexCoord;
uniform sampler2D image;

void main( void )
{
    vec4 tex = texture2D( image, frag_TexCoord );
    gl_FragColor = frag_Color * tex;
}
