in vec2 frag_TexCoord;

uniform sampler2D image;

out vec4 fragment;

void main(void)
{
    vec4 f = texture(image, frag_TexCoord);

    if (length(f.rgb) == 0)
        discard;

    // Red stands out a lot nicer as a font
    fragment = vec4( f.rgb * vec3( 1.0, 0.0, 0.0 ), 1.0 );
}

