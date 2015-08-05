attribute vec3 position;
attribute vec2 texCoord;

uniform vec3 origin;
uniform mat3 viewOrient;
uniform mat4 modelToView;
uniform mat4 viewToClip;

varying vec2 frag_TexCoord;

void main( void )
{
    // Note, this will turn the image upside down. STB_Image reads image data in a manner which differs from how OpenGL
    // expects its data, so this likely will result in a right-side-up image after the transformation is performed.
    mat3 orient = -viewOrient;

    gl_Position = viewToClip * modelToView * vec4( origin + orient * position, 1.0 );
    frag_TexCoord = texCoord;
}
