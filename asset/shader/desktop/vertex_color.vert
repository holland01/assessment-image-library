in vec3 position;
in vec4 color;

uniform mat4 modelToView;
uniform mat4 viewToClip;

smooth out vec4 frag_Color;

void main(void)
{
    gl_Position = viewToClip * modelToView * vec4( position, 1.0 );
    frag_Color = color;
}

