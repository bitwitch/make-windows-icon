#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // just need BITMAPHEADERINFO but including only wingdi.h doesnt work... lame.
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef struct {
	u16 reserved_zero; // must always be zero
	u16 image_type;    // 1 for icon (.ICO) image, 2 for cursor (.CUR) image. Other values are invalid.
	u16 num_images;    // Specifies number of images in the file. 
} IcoHeader;

typedef struct {
	u8 image_width;
	u8 image_height;
	u8 num_colors_in_palette; // should be 0 if the image does not use a color palette
	u8 reserved_zero; // must be 0
	u16 color_planes; // should be 0 or 1
	u16 bits_per_pixel; 
	u32 image_size_in_bytes;
	u32 offset; // offset of the bmp or png data from the beginning of the ico file
} IcoEntry;

bool file_write(FILE *f, u8 *data, u64 size) {
	u64 bytes_written = fwrite(data, 1, size, f);
	return bytes_written == size;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Usage: %s input_image_filename [--png]\n", argv[0]);
		return 1;
	}

	char *filename = argv[1];
	bool mode_png = argc > 2 && 0 == strncmp(argv[2], "--png", 5);

	int og_width, og_height, og_channels;
	unsigned char* og_image_data = stbi_load(filename, &og_width, &og_height, &og_channels, STBI_rgb_alpha);

	u32 sizes[] = { 16, 32, 48, 256 }; 
	i32 num_sizes = ARRAY_COUNT(sizes);
	
	IcoHeader header = {0};
	header.image_type = 1;
	header.num_images = (u16)num_sizes;

	FILE *ico_file = fopen("icon.ico", "wb");
	if (!ico_file) {
		fprintf(stderr, "failed to open icon.ico for writing\n");
		return 1;
	}

	//-----------------------------------------------------------------------------
	// Write .ico header
	//-----------------------------------------------------------------------------
	if (!file_write(ico_file, (u8*)&header, sizeof(header))) {
		fprintf(stderr, "failed to write header to icon.ico\n");
		return 1;
	}

	//-----------------------------------------------------------------------------
	// Generate images of each size
	//-----------------------------------------------------------------------------
	u8 *images[ARRAY_COUNT(sizes)] = {0};
	int image_sizes[ARRAY_COUNT(sizes)] = {0};

	for (i32 i=0; i<num_sizes; ++i) {
		u32 size = sizes[i];
		u32 width = size;
		u32 height = size;

		if (mode_png) {
			u8 *image_data = stbir_resize_uint8_linear(
					og_image_data, og_width, og_height, 0,
					0, size, size, 0,
					STBIR_RGBA);
			if (!image_data) {
				fprintf(stderr, "failed to generate %dx%d image for icon\n", size, size);
				return 1;
			}
			images[i] = stbi_write_png_to_mem(image_data,
				width * STBI_rgb_alpha, 
				width, height, STBI_rgb_alpha, 
				&image_sizes[i]);
			free(image_data);
			if (images[i] == 0) {
				fprintf(stderr, "failed to write png to ico file\n");
				return 1;
			}

		} else {
			// NOTE(shaw): The height of the BMP image must be twice the
			// height declared in the image directory. The second half of the
			// bitmap should be an AND mask for the existing screen pixels, with
			// the output pixels given by the formula 
			//     Output = (Existing AND Mask) XOR Image
			// Set the mask to be zero everywhere for a clean overwrite. 
			height *= 2;
			u32 resized_num_bytes = width * height * STBI_rgb_alpha;
			image_sizes[i] = sizeof(BITMAPINFOHEADER) + resized_num_bytes;
			images[i] = calloc(1, image_sizes[i]);

			BITMAPINFOHEADER *bmi_header = (BITMAPINFOHEADER*)images[i];
			bmi_header->biSize = sizeof(BITMAPINFOHEADER);
			bmi_header->biWidth = width;
			bmi_header->biHeight = height;
			bmi_header->biPlanes = 1;
			bmi_header->biBitCount = 32;
			bmi_header->biCompression = BI_RGB;
			bmi_header->biSizeImage = resized_num_bytes;

			u8 *resized_image_data = images[i] + sizeof(BITMAPINFOHEADER);
			u8 *check = stbir_resize_uint8_linear(
					og_image_data, og_width, og_height, 0,
					resized_image_data, size, size, 0,
					STBIR_RGBA);
			if (!check) {
				fprintf(stderr, "failed to generate %dx%d image for icon\n", size, size);
				return 1;
			}
		}
	}

	//-----------------------------------------------------------------------------
	// Write icon directory entries
	//-----------------------------------------------------------------------------
	u32 data_offset = sizeof(IcoHeader) + (num_sizes * sizeof(IcoEntry));
	for (i32 i=0; i<num_sizes; ++i) {
		u32 size = sizes[i];
		u8 entry_size = size == 256 ? 0 : (u8)size;

		IcoEntry entry = {
			.image_width = entry_size,
			.image_height = entry_size,
			.num_colors_in_palette = 0,
			.color_planes = 1,
			.bits_per_pixel = 32,
			.image_size_in_bytes = image_sizes[i],
			.offset = data_offset,
		};
		if (!file_write(ico_file, (u8*)&entry, sizeof(entry))) {
			fprintf(stderr, "failed to write image data to file\n");
			return 1;
		}
		data_offset += image_sizes[i];
	}

	//-----------------------------------------------------------------------------
	// Write image data
	//-----------------------------------------------------------------------------
	for (i32 i=0; i<num_sizes; ++i) {
		if (!file_write(ico_file, images[i], image_sizes[i])) {
			fprintf(stderr, "failed to write image data to file\n");
			return 1;
		}
	}

	return 0;
}


