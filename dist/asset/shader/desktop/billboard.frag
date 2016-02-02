smooth in vec2 frag_TexCoord;

uniform sampler2D image;
uniform vec4 color;

out vec4 fragment;

const vec4 gamma = vec4( vec3( 1.0 / 3.0 ), 1.0 );

void main( void )
{
    vec4 tex = texture( image, frag_TexCoord );

    /*
    if ( tex.a == 0.0 )
    {
        tex.a = vec4(1.0);
        //discard;
    }
    */

    fragment = pow( tex * color, gamma );
}
