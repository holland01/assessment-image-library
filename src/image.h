#pragma once

#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <sstream>
#include <string>
#include <cstring>
#include <array>

// TODO:
// 1) make a few test images and print out the data
// 2) render them with OpenGL
// 3) test the pixel testing functions - maybe through a simple user interactive program

#define IMG_DEF template <typename Tchannel, color_format Eformat, typename Tint = uint32_t>
#define IMG_DATA_TMPL data<Tchannel, Eformat, Tint>
#define IMG_INT_TYPE Tint
#define IMG_PIXEL_TMPL pixel<Tchannel, Eformat, Tint>

namespace img {

enum class color_format
{
	rgb = 3,
	greyscale = 1
};

IMG_DEF struct pixel
{
   // always initialize to zero, by default
	std::array<Tchannel, (size_t)Eformat> mChannels = {{}};
};

IMG_DEF struct data
{
	using channel_type = Tchannel;
	using int_type = Tint;
	using pixel_type = pixel<Tchannel, Eformat, Tint>;
	using buffer_type = std::vector<pixel_type>;

	int_type mWidth;
	int_type mHeight;
	buffer_type mPixels;
};

IMG_DEF IMG_INT_TYPE calc_pixel_offset(const IMG_DATA_TMPL &image, IMG_INT_TYPE x, IMG_INT_TYPE y)
{
	return (y * image.mWidth + x);
}

IMG_DEF IMG_PIXEL_TMPL &get_pixel(IMG_DATA_TMPL &image, IMG_INT_TYPE x, IMG_INT_TYPE y)
{
	return image.mPixels[calc_pixel_offset(image, x, y)];
}

IMG_DEF const IMG_PIXEL_TMPL &get_pixel(const IMG_DATA_TMPL &image, IMG_INT_TYPE x, IMG_INT_TYPE y)
{
	return image.mPixels[calc_pixel_offset(image, x, y)];
}

IMG_DEF IMG_DATA_TMPL make_image(IMG_INT_TYPE width, IMG_INT_TYPE height, const IMG_PIXEL_TMPL &fillValue = IMG_PIXEL_TMPL())
{
	size_t size = width * height;
	IMG_DATA_TMPL img;
	img.mWidth = width;
	img.mHeight = height;
	img.mPixels.resize(size, fillValue);

	return std::move(img);
}

IMG_DEF IMG_DATA_TMPL copy(const IMG_DATA_TMPL &image)
{
	IMG_DATA_TMPL c = {
		image.mWidth,
		image.mHeight,
		image.mPixels
	};

	return std::move(c);
}

IMG_DEF std::string to_string(const IMG_DATA_TMPL &image)
{
	std::stringstream stream;
	for (IMG_INT_TYPE y = 0; y < image.mHeight; ++y) {
		for (IMG_INT_TYPE x = 0; x < image.mWidth; ++x) {
			stream << to_string(get_pixel(image, x, y));
			if ((x + 1) != image.mWidth)
				stream << ", ";
		}
		stream << '\n';
	}
	return stream.str();
}

IMG_DEF std::string to_string(const IMG_PIXEL_TMPL &p)
{
	std::stringstream stream;
	stream << "{ ";
	for (size_t i = 0; i < p.mChannels.size(); ++i) {
		stream << p.mChannels[i];
		if ((i + 1) != p.mChannels.size())
			stream << ", ";
	}
	stream << " }";
	return stream.str();
}

using rgb_f32 = data<float, color_format::rgb>;
using rgb_u8 = data<uint8_t, color_format::rgb>;

} // namespace img

#undef IMG_DEF
#undef IMG_DATA_TMPL
#undef IMG_INT_TYPE
#undef IMG_PIXEL_TMPL

