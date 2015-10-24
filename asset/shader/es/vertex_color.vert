attribute vec3 position;
attribute vec3 color;

uniform mat4 modelToView;
uniform mat4 viewToClip;

varying vec4 frag_Color;

void main( void )
{
    gl_Position = viewToClip * modelToView * vec4( position, 1.0 );
    frag_Color = color;
}
