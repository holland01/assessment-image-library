smooth in vec2 frag_TexCoord;

uniform sampler2D image;
uniform vec4 color;

out vec4 fragment;
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


    fragment = tex * color;
}
