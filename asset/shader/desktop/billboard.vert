in vec3 position;
in vec2 texCoord;

uniform vec3 origin;
uniform mat3 viewOrient;
uniform mat4 modelToView;
uniform mat4 viewToClip;

smooth out vec2 frag_TexCoord;

void main( void )
{
    mat3 orient = -viewOrient;
    gl_Position = viewToClip * modelToView * vec4( origin + orient * position, 1.0 );
    frag_TexCoord = texCoord;
}

