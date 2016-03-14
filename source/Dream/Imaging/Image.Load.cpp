//
//  Imaging/ImageLoader.cpp
//  This file is part of the "Dream" project, and is released under the MIT license.
//
//  Created by Samuel Williams on 1/05/07.
//  Copyright (c) 2007 Samuel Williams. All rights reserved.
//
//

#include "Image.hpp"

#include <Dream/Core/Data.hpp>
#include <Dream/Core/Timer.hpp>
#include <Dream/Core/Logger.hpp>

extern "C" {
#include <png.h>
#include <jpeglib.h>
}

#include <exception>
#include <stdexcept>
#include <sstream>
#include <cstring>

namespace Dream {
	namespace Imaging {
		using namespace Core::Logging;

		struct DataFile {
			Shared<Buffer> buffer;
			unsigned offset;

			DataFile (const Ref<IData> data) {
				buffer = data->buffer();
				offset = 0;
			}

			const char * read_bytes (unsigned length) {
				unsigned prev_offset = offset;
				offset += length;
				return (const char*)(buffer->begin() + prev_offset);
			}

			static void png_read_data (png_structp png_reader, png_bytep data, png_size_t length) {
				DataFile *data_file = (DataFile *) png_get_io_ptr (png_reader);
				std::memcpy(data, data_file->read_bytes(length), length);
			}
		};

// MARK: -
// MARK: JPEG Loader Code

		static void jpeg_dummy (j_decompress_ptr cinfo) {
		}

		static boolean jpeg_fill_input_buffer (j_decompress_ptr cinfo) {
			// Force the jpeg library to stop reading if we run past the end of data
			static JOCTET eoi[2] = {(JOCTET)0xff, (JOCTET)JPEG_EOI};
			cinfo->src->next_input_byte = eoi;
			cinfo->src->bytes_in_buffer = 2;

			return (boolean)0;
		}

		/*
		 * Skip data --- used to skip over a potentially large amount of
		 * uninteresting data (such as an APPn marker).
		 */

		static void jpeg_skip_input_data (j_decompress_ptr cinfo, long num_bytes) {
			//DREAM_ASSERT(num_bytes < cinfo->src.bytes_in_buffer && "jpeglib tried to skip further than the end of file!!");

			if (num_bytes > 0 && num_bytes < (long)cinfo->src->bytes_in_buffer) {
				cinfo->src->next_input_byte += (std::size_t) num_bytes;
				cinfo->src->bytes_in_buffer -= (std::size_t) num_bytes;
			} else {
				jpeg_fill_input_buffer(cinfo);
			}
		}

		static void jpeg_memory_src (j_decompress_ptr cinfo, const JOCTET * buffer, std::size_t bufsize) {
			if (cinfo->src == NULL) {
				cinfo->src = (jpeg_source_mgr *) (*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(jpeg_source_mgr));
			}

			cinfo->src->init_source = jpeg_dummy;

			cinfo->src->fill_input_buffer = jpeg_fill_input_buffer;
			cinfo->src->skip_input_data = jpeg_skip_input_data;

			cinfo->src->resync_to_restart = jpeg_resync_to_restart; // default
			cinfo->src->term_source = jpeg_dummy;

			cinfo->src->next_input_byte = buffer;
			cinfo->src->bytes_in_buffer = bufsize;
		}

		static Ref<Image> load_jpeg_image (const Ptr<IData> data) {
			jpeg_decompress_struct cinfo;
			jpeg_error_mgr jerr;

			Ref<Image> result_image;
			DataType data_type = DataType::BYTE;
			
			Shared<Buffer> buffer = data->buffer();

			unsigned width, height;

			try {
				//memset (&cinfo, 0, sizeof (cinfo));
				memset (&jerr, 0, sizeof (jerr));

				cinfo.err = jpeg_std_error(&jerr);

				jpeg_create_decompress(&cinfo);
				jpeg_memory_src(&cinfo, buffer->begin(), buffer->size());

				jpeg_read_header(&cinfo, TRUE);

				width = cinfo.image_width;
				height = cinfo.image_height;

				unsigned row_width = 0;
				PixelFormat pixel_format;
				
				if (cinfo.jpeg_color_space == JCS_GRAYSCALE) {
					row_width = width;
					pixel_format = PixelFormat::L;
				} else {
					row_width = 3 * width;
					pixel_format = PixelFormat::RGB;
				}

				result_image = new Image(Vec2u(width, height), pixel_format, data_type);

				ByteT *line = result_image->data();
				jpeg_start_decompress(&cinfo);

				// read jpeg image
				while (cinfo.output_scanline < cinfo.output_height) {
					jpeg_read_scanlines(&cinfo, &line, 1);
					line += row_width;
				}

				jpeg_finish_decompress(&cinfo);
				jpeg_destroy_decompress(&cinfo);
			} catch (std::exception &e) {
			}

			return result_image;
		};

// MARK: -
// MARK: PNG Loader Code
		static void png_error (png_structp png_reader, png_const_charp msg) {
			throw std::runtime_error(msg);
		}

		static Ref<Image> load_png_image (const Ptr<IData> data) {
			// Image formatting details
			PixelFormat format = PixelFormat(0);
			DataType data_type = DataType(0);

			DataFile df(data);
			Ref<Image> result_image;
			Shared<Buffer> buffer = data->buffer();

			// internally used by libpng
			png_structp png_reader = NULL;
			// user requested transforms
			png_infop png_info = NULL;

			png_byte **ppb_row_pointers = NULL;

			if (!png_check_sig((png_byte*)buffer->begin(), 8)) {
				log_error("Could not verify PNG image!");
				return Ref<Image>();
			}

			try {
				png_reader = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, png_error, NULL);
				DREAM_ASSERT(png_reader != NULL && "png_create_read_struct returned NULL!");

				png_info = png_create_info_struct(png_reader);
				DREAM_ASSERT(png_info != NULL && "png_create_info_struct returned NULL!");

				// We will use this function to read data from Data class
				png_set_read_fn (png_reader, (void *)&df, DataFile::png_read_data);

				// Read PNG header
				png_read_info (png_reader, png_info);

				// Interpret IMAGE header
				int bit_depth, color_type;
				png_uint_32 width, height;

				png_get_IHDR (png_reader, png_info, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

				if (bit_depth < 8) {
					png_set_packing (png_reader);
					//png_set_expand(png_reader);
				}

				if (color_type == PNG_COLOR_TYPE_PALETTE) {
					png_set_expand(png_reader);
				}

				// after the transformations have been registered update png_info data
				png_read_update_info(png_reader, png_info);
				png_get_IHDR(png_reader, png_info, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

				// Figure out image format
				if (color_type == PNG_COLOR_TYPE_RGB) {
					format = PixelFormat::RGB;
				} else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
					format = PixelFormat::RGBA;
				} else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
					format = PixelFormat::LA;
				} else if (color_type == PNG_COLOR_TYPE_GRAY) {
					format = PixelFormat::L;
				}

				// Figure out bit depth
				if (bit_depth == 16) {
					// It is possible to convert to 8bpp using: png_set_strip_16 (png_reader);
					data_type = DataType::SHORT;
				} else if (bit_depth == 8) {
					data_type = DataType::BYTE;
				} else {
					std::stringstream s; s << "PNG: Bit depth of " << bit_depth << " not supported!" << std::endl;
					throw std::runtime_error(s.str());
				}

				// row_bytes is the width x number of channels
				unsigned row_bytes = png_get_rowbytes(png_reader, png_info);

				// Allocate the image_data buffer.
				result_image = new Image(Vec2u(width, height), format, data_type);
				ByteT *image_bytes = result_image->data();
				DREAM_ASSERT(image_bytes != NULL);

				//ByteT * image_data = (unsigned char *) malloc(rowbytes * height);
				ppb_row_pointers = (png_bytepp)malloc(height * sizeof(png_bytep));
				for (unsigned i = 0; i < height; i++)
					ppb_row_pointers[i] = image_bytes + i * row_bytes;

				png_read_image(png_reader, ppb_row_pointers);
				png_read_end(png_reader, NULL);

				free(ppb_row_pointers);

				if (png_reader) png_destroy_read_struct(&png_reader, &png_info, NULL);
			} catch (std::exception &e) {
				log_error("PNG read error: ", e.what());

				if (png_reader) png_destroy_read_struct(&png_reader, &png_info, NULL);

				png_reader = NULL;

				if (ppb_row_pointers) free(ppb_row_pointers);

				throw;
			}

			return result_image;
		}

// MARK: -
// MARK: Loader Multiplexer

		Ref<Image> Image::load_from_data (const Ptr<IData> data) {
			Ref<Image> loaded_image;

			auto buffer = data->buffer();
			auto image_type = read_image_type(*buffer);
			
			switch (image_type) {
			case ImageType::JPEG:
				return load_jpeg_image(data);
			case ImageType::PNG:
				return load_png_image(data);
			default:
				log_error("Could not load image: Unsupported image format.");
				return nullptr;
			}
		}
	}
}
