//
//  JPEGImage.cpp
//  This file is part of the "Images" project and released under the MIT License.
//
//  Created by Samuel Williams on 2/5/2016.
//  Copyright, 2016, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>

#include <Images/JPEGImage.hpp>
#include <Resources/FileData.hpp>

namespace Images
{
	UnitTest::Suite JPEGImageTestSuite {
		"Images::JPEGImage",
		
		{"can load file and size is correct",
			[](UnitTest::Examiner & examiner) {
				auto data = owned<Resources::FileData>("Images/fixtures/9452-v2.jpg");
				auto image = owned<JPEGImage>(data);
				
				examiner << "Width and size should be loaded correctly" << std::endl;
				examiner.expect(image->size()) == Image::Size{64, 64, 1};
			}
		},
		
		{"can load file and convert to pixel buffer",
			[](UnitTest::Examiner & examiner) {
				auto data = owned<Resources::FileData>("Images/fixtures/9452-v2.jpg");
				auto image = owned<JPEGImage>(data);
				
				PixelLayout2D pixel_layout{image->size()};
				auto pixel_buffer = owned<PixelBuffer2D>(pixel_layout);
				image->load(pixel_layout, pixel_buffer->begin());
				
				examiner << "Correct pixel colour was loaded from file" << std::endl;
				examiner.expect((*pixel_buffer)[{0, 0}]) == PixelFormat::RGBA8{125, 103, 79, 255};
			}
		},
		
		{"can save to RGB data",
			[](UnitTest::Examiner & examiner) {
				auto data = owned<Resources::FileData>("Images/fixtures/9452-v2.jpg");
				auto image = owned<JPEGImage>(data);
				
				PixelLayout2D pixel_layout{image->size()};
				auto pixel_buffer = owned<PixelBuffer2D>(pixel_layout);
				image->load(pixel_layout, pixel_buffer->begin());
				
				auto output_buffer = JPEGImage::save(pixel_layout, pixel_buffer->begin());
				auto output_data = owned<BufferedData>(output_buffer);
				
				examiner << "Output data was generated." << std::endl;
				examiner.expect(output_data->size()) > 0;
				
				auto output_image = owned<JPEGImage>(output_data);
				examiner << "Output image has same size as input" << std::endl;
				examiner.expect(output_image->size()) == image->size();
			}
		},
	};
}
