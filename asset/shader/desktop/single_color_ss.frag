uniform vec4 color;

out vec4 fragment;

const vec4 gamma = vec4( vec3( 1.0 / 2.2 ), 1.0 );

void main(void)
{
    fragment = pow( color, gamma );
}

