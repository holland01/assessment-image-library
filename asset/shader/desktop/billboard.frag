smooth in vec4 frag_Color;
smooth in vec2 frag_TexCoord;

uniform sampler2D image;

out vec4 fragment;
void main( void )
{
    vec4 tex = texture( image, frag_TexCoord );

    if ( tex.a == 0.0 )
    {
        discard;
    }

    fragment = tex;
}
