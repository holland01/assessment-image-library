### image-library assessment

This repo is primarily for the purposes of evaluation by a potential employer.

However, most of the code here has been written with other goals in mind:
the image-library incorporated as desired by the assessment is contained
within a code base I've maintained and worked on over the years.

The code base is used primarily for writing demos and experimentation.

The directory, src, contains all of the source code. The files which are
directly relevant to the assessment are:

* src/img.h - the actual image library file
* src/tests/image_test.h - an example demo which shows how to use the file
itself.

image_test.h is a demo which uses the img.h file to
render the following 8 different kinds of images:
    - RGB 8 bit
    - Greyscale 8 bit
    - RGB 8 bit embossed
    - Greyscale 8 bit embossed

    - RGB floating point
    - Greyscale floating point
    - RGB floating point embossed
    - Greyscale floating point embossed

All of the images are dependent on two files: lena_rgb.jpg, and lena.png,
both of which can be found in the asset/ folder. As their names suggest,
these images display the same content. The key difference is that
the png variant is black and white, while the jpg is obviously colored.

Demo interaction is simple: one can move around a basic scene in 3D
space using the mouse and W,S,A,D keys for standard movement
in addition to the space bar and left shift key for raising and lowering the
camera, respectively.

In this demo, there's only set of images in the scene; so,
moving forward and backward using W and S, respectively, should be sufficient
to view everything properly. The user may have to move backward a few
steps before the image is shown.

Pressing the "UP" key will toggle between displaying
the two available data formats - floating point and 8 bit.

The images are gamma corrected directly in the OpenGL shader which processes
their sampling and fragment output. Technically, gamma correction can be
accomplished using the SRGB8 texture format which is provided by the API,
but this is a much smoother process.

Unfortunately, the demo has only been tested on Ubuntu 15.04. There is a simple
guide provided on running it, for those who have access to an Ubuntu distro.

(scroll to the bottom for a simple installation guide)

### Answers to Assessment Questions

* **why you chose the implementation you did, and any upsides/downsides it
  afords in comparison to other options.**

The implementation chosen was strictly for the sake of demonstrating a
reasonable understanding of modern C++, in addition to showing
a few scenarios where more C-like techniques were useful (mostly
    involving usage memcpy).

As such, this implementation makes large usage of templates. The benefits
of using a templated architecture is that it allows for users to adjust
the usage of their data to different formats as necessary. Much of the same
code can be used for the direct processing of the image data itself, regardless
of its native type (e.g, 32 bit integer, float, a byte, etc.).

The downsides of this approach are the obvious code bloat and longer compile
times. The code bloat is very unideal for embedded systems, considering that
embedded systems will often have strict memory requirements which also
will include limitations on the size of the executable binary itself.

If I were restricted to something like C99, my implementation of choice
would be to represent an image internally as a pointer to an unsigned char,
and to perform any conversions between differing types as necessary.

What may be worth taking into account as well is cache efficiency: a more cache
efficient solution would allow for a image data store which was handled
by the library itself. Users would access the images themselves using
handles and API functions designed to query and/or set data corresponding
to the image. Internally, each differing type of image data would be granted
its own buffer. For example, all width values would be stored in their own
buffer, as would be the same for height values. Data buffers
would also be contained in something like an std::vector as well.

* **if you ran out of time, what you were hoping to do next**

I would have implemented the image scaling function, in addition
to providing a Windows build :).   

### Running the demo (Ubuntu only)

The demo can be found in the dist folder.

The binary file "op" is the executable.

The code bases links against both SDL 2 and Bullet Physics. Bullet is statically
linked, and so is SDL 2. However, SDL 2 itself requires shared
library dependencies. So, open up a terminal and copypasta
the following:

```
sudo apt-get install libgl1-mesa-dev \
libglu1-mesa-dev \
libglew-dev \
libasound2-dev \
libpulse-dev \
libxcursor-dev \
libxinerama-dev \
libxi-dev \
libxrandr-dev \
libxss-dev \
libwayland-egl1-mesa \
libwayland-cursor0 \
libxkbcommon-dev;
```

If you're running another distro, the above should at least tell you what
libraries you need. A Windows build has been omitted simply as a means
to save time, however if it's desired one can be produced.
