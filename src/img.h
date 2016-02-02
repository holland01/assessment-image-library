#pragma once

#include "def.h"

#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <sstream>
#include <string>
#include <cstring>
#include <array>
#include <lib/stb_image.h>
#include <glm/glm.hpp>
#include <lib/glm/mat3x3.hpp>
#include <lib/glm/gtc/matrix_access.hpp>

// These are nice in situations when you find yourself making lots of changes as you go along :)
// Given time, I would replace the macros with their substitutions, but for now they work.
#define IMG_DEF template <typename Tchannel, color_format Eformat, typename Tint = int32_t>
#define IMG_DATA_TMPL data<Tchannel, Eformat, Tint>
#define IMG_INT_TYPE Tint
#define IMG_PIXEL_TMPL pixel<Tchannel, Eformat, Tint>

namespace img {

// Using an approach similar to computing a vector's magnitude has proven
// adequate as a normalization method. I think that
// dividing by the determinant could produce even better results, though
static inline glm::mat3	normalize_kernel(const glm::mat3& base)
{
	glm::mat3 mm(base);

	float sum = 0.0f;
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			sum += mm[i][j] * mm[i][j];

	if (sum != 0.0f)
		mm /= glm::sqrt(sum);

	return mm;
}

// A simple helper function we can use to make a kernel with.
glm::mat3 make_kernel(bool normalize,
							  float a, float b, float c,
							  float d, float e, float f,
							  float g, float h, float i)
{
	glm::mat3 base(	glm::vec3(a, b, c),
					glm::vec3(d, e, f),
					glm::vec3(g, h, i) );

	if (normalize)
		return normalize_kernel(base);

	return base;
}

#define EMBOSS_SAMPLE_FACTOR -2.0f
#define EMBOSS_CENTER_FACTOR 6.0f

static inline glm::tmat3x3<float> kernel_emboss(bool normalize)
{
	return make_kernel(normalize,
					   EMBOSS_SAMPLE_FACTOR, EMBOSS_SAMPLE_FACTOR, 0.0f,
					   EMBOSS_SAMPLE_FACTOR, EMBOSS_CENTER_FACTOR, 0.0f,
					   0.0f, 0.0f, 0.0f);
}

#undef EMBOSS_SAMPLE_FACTOR
#undef EMBOSS_CENTER_FACTOR

enum class color_format
{
	rgb = 3,
	greyscale = 1
};

// Simple pixel type. Image data stores a buffer of this.
IMG_DEF struct pixel
{
	using channel_t = Tchannel;
	using vec_t = std::array<Tchannel, (size_t)Eformat>;

	vec_t mChannels;

	pixel(channel_t v = channel_t(0))
	{
		mChannels.fill(v);
	}
};

// "data" is basically a catch-all term for an image.
IMG_DEF struct data
{
	using channel_t = Tchannel;
	using int_t = Tint;
    using pixel_t = pixel<Tchannel, Eformat, Tint>;
    using buffer_t = std::vector<pixel_t>;

    static const size_t PIXEL_STRIDE = (size_t)Eformat;
	static const size_t PIXEL_STRIDE_BYTES = PIXEL_STRIDE * sizeof(Tchannel);

	// the integers are abstracted as template parameters out in the event that the user knows the hard limits
	// of their dimensions. For example, maybe they're not using images larger than 128x128.
	// We use a sane default of int32_t, if unspecified.
    int_t mWidth;
    int_t mHeight;
    buffer_t mPixels;
};

// It's useful to have the ability to convert the image data to pure bytes in some situations (e.g., if we're using
// floats as a format)
using raw_buffer = std::vector<uint8_t>;

// Trivial simple helper methods
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

// Converts each "pixel" type in the image's
// data buffer to a corresponding byte representation
// and returns it as a buffer.
IMG_DEF raw_buffer get_raw_pixels(const IMG_DATA_TMPL &image)
{
    constexpr size_t stride = (size_t)Eformat;

	// It seems that C++ compilers won't optimize out static
	// conditionals which evauluate to false... there's use
	// of an emulated static_if further down, whose implementation
	// can be found in "base.inl" (it was stolen, link is provided)
	if (std::is_same<Tchannel, float>::value) {
		uint8_t* pBytes = (uint8_t*)(&image.mPixels[0].mChannels[0]);
		constexpr size_t byteStride = stride * sizeof(Tchannel);
		size_t length = byteStride * image.mPixels.size();
		raw_buffer pixels(length);
		memcpy(&pixels[0], pBytes, length);
		return std::move(pixels);
    } else {
		raw_buffer pixels(stride * sizeof(Tchannel) * image.mPixels.size(), 0);
		memcpy(&pixels[0], &image.mPixels[0].mChannels[0], pixels.size());
        return std::move(pixels);
    }
}

// Useful for constructing textures out of user-specified data, rather than image files stored on disk.
// Obviously not much going on here.
IMG_DEF IMG_DATA_TMPL make_image(IMG_INT_TYPE width, IMG_INT_TYPE height, const IMG_PIXEL_TMPL &fillValue)
{
	size_t size = width * height;
	IMG_DATA_TMPL img;
	img.mWidth = width;
	img.mHeight = height;
	img.mPixels.resize(size, fillValue);

	return std::move(img);
}

enum class from_file_error
{
	none,
	invalid_path,
	incompatible_format // we return this if stbi_load* returns a channel count for the image which doesn't work with the desired format
};

// The main image creation function. from_file_error is a pointer specifically because
// the user just may not care; having the option of a nullptr can be nice in some cases.
template <typename image_t>
image_t from_file(const std::string &path, from_file_error *error, bool invertImage = true)
{
	image_t img;
	from_file_error e = from_file_error::none;

	using channel_t = typename image_t::channel_t;
	using int_t = typename image_t::int_t;
	using pixel_t = typename image_t::pixel_t;

	int32_t numChannels = 0;
	channel_t *buffer = nullptr;

	// We use the emulated static_if (*) here to avoid
	// the compiler complaining about conflicting types for the stbi_load* function
	// in the opposing conditional

	// (*) see base.inl for the implementation and source reference
	static_if<std::is_same<channel_t, float>::value>([&](auto f) {
		f(buffer) = (channel_t*) stbi_loadf(path.c_str(), (int32_t*) &img.mWidth, (int32_t*) &img.mHeight, &numChannels, 0);
	}).else_([&](auto f) {
		f(buffer) = (channel_t*) stbi_load(path.c_str(), (int32_t*) &img.mWidth, (int32_t*) &img.mHeight, &numChannels, 0);
	});

	if (!buffer) {
		e = from_file_error::invalid_path;
		goto finish; // Haters gonna hate: I believe goto is still effective in some situations :)
	}

	if ((size_t)numChannels != image_t::PIXEL_STRIDE) {
		e = from_file_error::incompatible_format;
		goto finish;
	}

	// Since we have a goto, we wrap the following in a block to avoid
	// compiler errors about jumping across stack boundries
	{
		size_t length = img.mWidth * img.mHeight;
		img.mPixels.resize(length, typename image_t::pixel_t(255));

		// STBI loads the image with the top left-most pixel being
		// the beginning; OpenGL's texture coordinate system has an inverse
		// relationship with the y-axis, where the bottom left is the origin of
		// the image.
		if (invertImage) {
			for (int_t y = 0; y < img.mHeight; ++y) {
				for (int_t x = 0; x < img.mWidth; ++x) {
					pixel_t& pix = img.mPixels[calc_pixel_offset(img, x, y)];
					channel_t* bpix = &buffer[numChannels * ((img.mHeight - 1 - y) * img.mWidth + x)];
					memcpy(&pix.mChannels[0], bpix, sizeof(bpix[0]) * numChannels);
				}
			}
		} else {
			memcpy(&img.mPixels[0].mChannels[0], buffer, image_t::PIXEL_STRIDE_BYTES * length);
		}
	}

finish:
	if (buffer)
		stbi_image_free(buffer);

	if (error)
		*error = e;

	return std::move(img);
}

// Applies an arbitrary kernel matrix to an image; the result
// is a copy of the source image with the matrix applied to it.
// Regardless of the image data's format, we use floating point computations.
template <typename image_t>
image_t apply_kernel(const image_t& src, const glm::mat3& kernel)
{
	image_t copy(src);

	using int_t = typename image_t::int_t;
	using pixel_t = typename image_t::pixel_t;
	using channel_t = typename image_t::channel_t;

	static const bool IS_FLOAT = std::is_same< float, channel_t >::value;

	// Trivial image-kernel convolving function. Information
	// on kernels was taken from https://en.wikipedia.org/wiki/Kernel_(image_processing)
	for (int_t y = 0; y < copy.mHeight; ++y) {
		for (int_t x = 0; x < copy.mWidth; ++x) {
			std::array< float, image_t::PIXEL_STRIDE > accum;
			accum.fill( 0.0f );

			for (int32_t i = -1; i <= 1; ++i) {
				for (int32_t j = -1; j <= 1; ++j) {
					int_t px = (x + j) % copy.mWidth;

					// There are other methods for dealing with out of bounds pixels; this method
					// was the simplest given the amount of time available
					if (px < 0)
						px = copy.mWidth + px;

					int_t py = (y + i) % copy.mHeight;
					if (py < 0)
						py = copy.mHeight + py;

					// We use 1 - j or i because the example given in the Wiki article
					// follows the same inverse relationship; I'm not sure if other methods
					// are used.
					int_t kx = 1 - j;
					int_t ky = 1 - i;

					const pixel_t& pix = copy.mPixels[calc_pixel_offset(copy, px, py)];

					// If we're not using floats for our image data,
					// we need to do a bit of conversion.
					if (IS_FLOAT) {
						for (uint32_t i = 0; i < accum.size(); ++i)
							accum[i] += pix.mChannels[i] * kernel[ky][kx];
					} else {
						float kernelf = kernel[ky][kx] == 2? -2.0f: kernel[ky][kx];
						for (uint32_t i = 0; i < accum.size(); ++i) {
							float x = float(pix.mChannels[i]) / 255.0f;
							accum[i] += x * kernelf;
						}
					}
				}
			}

			// Make sure nothing is screwed up.
			for (uint32_t i = 0; i < accum.size(); ++i)
				accum[i] = glm::clamp(accum[i], 0.0f, 1.0f);

			uint32_t offset = calc_pixel_offset(copy, x, y);

			static_if<IS_FLOAT>([&](auto f) {
				f(copy.mPixels[offset].mChannels) = accum;
			}).else_([&](auto f){
				for (uint32_t i = 0; i < accum.size(); ++i)
					f(copy.mPixels[offset].mChannels[i]) = uint8_t(accum[i] * 255.0f);
			});
		}
	}

	return std::move(copy);
}

// Debugging...
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

// Debugging...
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

// Some predefined types
using rgb_f32_t = data<float, color_format::rgb>;
using rgb_u8_t = data<uint8_t, color_format::rgb>;
using greyscale_u8_t = data<uint8_t, color_format::greyscale>;
using greyscale_f32_t = data<float, color_format::greyscale>;

} // namespace img

#undef IMG_DEF
#undef IMG_DATA_TMPL
#undef IMG_INT_TYPE
#undef IMG_PIXEL_TMPL
#undef IMG_MAKE_KERNEL
