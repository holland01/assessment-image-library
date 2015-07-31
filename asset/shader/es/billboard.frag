varying vec4 frag_Color;
varying vec2 frag_TexCoord;
uniform sampler2D image;

void main( void )
{
    vec4 tex = texture2D( image, frag_TexCoord );
    if ( tex.a == 0.0 )
    {
        discard;
    }

    gl_FragColor = tex;
}