// using C++ 17 standard
#include "io_bmp.h"
#include "memory_management.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>

// read bmp file then seperate the color channel and flip
void read_bmp_to_image(io_image& image, const char* fname) {
	bmp_in in; 
	if (bmp_in__open(&in, fname) != 0) {
		throw std::runtime_error("Failed to open bmp file.");
	}

	image.num_components = in.num_components;
	image.comps = new io_comp[in.num_components];

	for (int i = 0; i < in.num_components; i++) {
		// assign config data for each component
		io_comp* comp = &image.comps[i];
		comp->width = in.cols;
		comp->height = in.rows;
		comp->allocate_with_border(); // for later use (e.g. filters)
	}

	// separete image components
	io_byte temp_line[1024 * 3]; // maxium 1024 pixel for each row
	for (int r = 0; r < in.rows; r++) {
		// read each line
		if (bmp_in__get_line(&in, temp_line) != 0) {
			throw std::runtime_error("Failed to get line.");
		}

		// flip the row    
		int new_row = in.rows - r - 1;
		
		for (int col = 0; col < in.cols; col++) {
			if (image.num_components == 1) {
				int gray = temp_line[col];  // gray image only has one component
				image.comps[0].buf[new_row * image.comps[0].stride + col] = gray;
			} else if (image.num_components == 3) {
				int blue = temp_line[col * 3 + 0]; // Blue value
				int green = temp_line[col * 3 + 1]; // green value
				int red = temp_line[col * 3 + 2]; // red value

				// comp[1], comps[2], comps[3] control r, g, b values respectively
				image.comps[0].buf[new_row * image.comps[0].stride + col] = red;
				image.comps[1].buf[new_row * image.comps[1].stride + col] = green;
				image.comps[2].buf[new_row * image.comps[2].stride + col] = blue;
			}
		}
	}
	bmp_in__close(&in);
}

// output bmp 
void write_image_to_bmp(const io_image& image, const char* fname) {
	bmp_out out;
	if (bmp_out__open(&out, fname,
		image.comps[0].width,
		image.comps[0].height,
		image.num_components) != 0) {
		throw std::runtime_error("Failed to open output bmp file.");
	}

	io_byte temp_line[1024 * 3];

	for (int r = image.comps[0].height - 1; r >= 0; r--) {
		if (image.num_components == 1) {
			const int* gray_line = image.comps[0].buf + r * image.comps[0].stride;
			for (int col = 0; col < image.comps[0].width; col++) {
				temp_line[col] = static_cast<io_byte>(std::clamp(gray_line[col], 0, 255));
			}
		} else if (image.num_components == 3) {
			const int* r_line = image.comps[0].buf + r * image.comps[0].stride;
			const int* g_line = image.comps[1].buf + r * image.comps[1].stride;
			const int* b_line = image.comps[2].buf + r * image.comps[2].stride;
			for (int col = 0; col < image.comps[0].width; col++) {
				temp_line[col * 3 + 0] = static_cast<io_byte>(std::clamp(b_line[col], 0, 255));
				temp_line[col * 3 + 1] = static_cast<io_byte>(std::clamp(g_line[col], 0, 255));
				temp_line[col * 3 + 2] = static_cast<io_byte>(std::clamp(r_line[col], 0, 255));
			}
		}

		if (bmp_out__put_line(&out, temp_line) != 0) {
			throw std::runtime_error("Failed to write bmp line.");
		}
	}

	bmp_out__close(&out);
}

// brighten the first plane
void brighten(io_image& image, int delta) {
	io_comp& comp = image.comps[0];
	for (int r = 0; r < comp.height; ++r) {
		int* row = comp.buf + r * comp.stride;
		for (int c = 0; c < comp.width; ++c) {
			row[c] += delta;
		}
	}
}

int main(int argc, char *argv[]) { // agrc: argument count, 1 at minimum, .exe itself is an argument
								   // char *argv[]: argument vectors
	if (argc != 4) {
		fprintf(stderr, "Usage: %s <in bmp file template> <out bmp file template> <frame number>\n", argv[0]);
		return -1;
	}

	int num_frames = 1;
	if (sscanf(argv[3], "%d", &num_frames) == 0) {
		fprintf(stderr, "Invalid frame count argument.\n");
		return -1;
	}

	std::cout << "Processing " << num_frames << " frame(s)...\n" << std::endl;

	// processing every frame and output the bmp files based on the intput\output template
	for (int f = 0; f < num_frames; f++) {
		char* input_name = new char[strlen(argv[1]) + 20];
		char* output_name = new char[strlen(argv[2]) + 20];

		strcpy(input_name, argv[1]);
		strcpy(output_name, argv[2]);

		char* in_sep = strrchr(input_name, '.'); // find separator
		char* in_suffix = argv[1] + (in_sep - input_name);
		char* out_sep = strrchr(output_name, '.'); // find separator
		char* out_suffix = argv[2] + (out_sep - output_name);

		sprintf(in_sep, "%d%s", f + 1, in_suffix);
		sprintf(out_sep, "%d%s", f + 1, out_suffix);
		
		try {
			io_image image;
			read_bmp_to_image(image, input_name);
			brighten(image, 60);
			write_image_to_bmp(image, output_name);
			std::cout << "Processed " << input_name << "->" << output_name << std::endl;

		} catch (const std::exception& e) {
			std::cerr << "Error: " << e.what() << std::endl;
		} catch (int exc) {
			if (exc == IO_ERR_NO_FILE)
				fprintf(stderr, "Cannot open supplied input or output file.\n");
			else if (exc == IO_ERR_FILE_HEADER)
				fprintf(stderr, "Error encountered while parsing BMP file header.\n");
			else if (exc == IO_ERR_UNSUPPORTED)
				fprintf(stderr, "Input uses an unsupported BMP file format.\n  Current "
					"simple example supports only 8-bit and 24-bit data.\n");
			else if (exc == IO_ERR_FILE_TRUNC)
				fprintf(stderr, "Input or output file truncated unexpectedly.\n");
			else if (exc == IO_ERR_FILE_NOT_OPEN)
				fprintf(stderr, "Trying to access a file which is not open!(?)\n");
			return -1;
		}

	}
	std::cout << "All files processed." << std::endl;

	return 0;
}