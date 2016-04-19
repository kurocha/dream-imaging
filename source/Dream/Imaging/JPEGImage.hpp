//
//  JPEGImage.hpp
//  File file is part of the "vulkanraum" project and released under the Orion Transfer Commercial License.
//
//  Created by Samuel Williams on 18/4/2016.
//  Copyright, 2016, by Samuel Williams. All rights reserved.
//

#pragma once

#include "Image.hpp"

extern "C" {
#include <jpeglib.h>
}

namespace Dream
{
	namespace Imaging
	{
		class JPEGImage
		{
		public:
			JPEGImage();
			virtual ~JPEGImage();
			
		private:
			
		};
	}
}