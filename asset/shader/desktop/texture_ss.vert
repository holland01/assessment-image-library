in vec3 position;
in vec2 texCoord;

out vec2 frag_TexCoord;

void main(void)
{
    gl_Position = vec4( position.xy, 0.0, 1.0 );
    frag_TexCoord = texCoord;
}

