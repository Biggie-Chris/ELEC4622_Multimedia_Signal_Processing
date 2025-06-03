#pragma once
#include "io_bmp.h"

class io_comp {
public:
	int width = 0;
	int height = 0;
	int stride = 0; // sample num for each row

	// 1. buf points to the top left of image component, we can use negatie indices to access samples
	// outside the original image boundaries
	// 2. handle points to the start of alloacted memory block (extened version)
	int* buf = nullptr;
	int* handle = nullptr;

public:
	void allocate_with_border(int border = 16) { // 16 by default
		stride = width + border * 2;
		int true_height = height + border * 2;

		handle = new int[stride * true_height];
		buf = handle + stride * border + border; // buf is a pointer shifted from handle
												 // no need to delete
	}

	io_comp() = default;
	~io_comp() { delete[] handle; }
};

// image contains several color components
class io_image {
public:
	int num_components = 0;
	io_comp* comps = nullptr;

	io_image() = default;
	~io_image() { delete[] comps; }
};

