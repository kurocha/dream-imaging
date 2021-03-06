# Images

Provides image loading, saving and manipulation.

[![Actions Status](https://github.com/kurocha/images/workflows/Tests/badge.svg)](https://github.com/kurocha/images/actions?workflow=Tests)

## Setup

The build tool `teapot` needs to be installed (which requires [Ruby][2]):

	$ gem install teapot

[2]: http://www.ruby-lang.org/en/downloads/

### Install Dependencies

To fetch project dependencies, run the following:

	$ cd images
	$ teapot fetch

### Validate Behaviour

The project can be checked by running unit tests:

	$ cd images
	$ teapot Test/Images

## Usage

`Dream::Imaging` provides a few useful classes for dealing with pixel data: `Dream::Imaging::PixelBuffer` which has an associated `Dream::Imaging::PixelBufferLayout`, and `Dream::Imaging::Image` represents a typical 2D image.

### Loading

You can load an image using data:

	auto data = owner<Resources::FileData>("Images/fixtures/Dream.png");
	auto image = owner<PNGImage>(data);
	log("Image size:", image->size());

An image cannot be used directly but must be converted to a pixel buffer with a specific layout:

	PixelLayout2D layout(image.size());
	PixelBuffer2D pixel_buffer(layout);
	image->convert(layout, pixel_buffer.data());

This will read the image data and convert it into the pixel buffer. This is done because the layout may have alignment requirements, e.g. row stride. This guarantees also the output format, even if the input format was, for example, only RGB. This is handled internally as efficiently as possible.

### Saving

You can save a pixel buffer using specific save methods exposed by the various file format classes.

	auto output_data = PNGImage::save(pixel_buffer.layout, pixel_buffer.data());
	output_data->buffer->write_to_path("output.png");

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

## License

Released under the MIT license.

Copyright, 2017, by [Samuel G. D. Williams](http://www.codeotaku.com/samuel-williams).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.