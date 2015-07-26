attribute vec3 position;
attribute vec2 texCoord;
attribute vec4 color;

uniform vec3 origin;
uniform mat3 viewOrient;
uniform mat4 modelToView;
uniform mat4 viewToClip;

varying vec4 frag_Color;
varying vec2 frag_TexCoord;

void main( void )
{
    mat3 orient = -viewOrient;

    gl_Position = viewToClip * modelToView * vec4( origin + orient * position, 1.0 );
    frag_Color = color;
    frag_TexCoord = texCoord;
}
