/*
This is a reconstructed verison based on David Taubman's io_bmp.cpp
using modern cpp techniques.
@Author: Chris, zqchris1998@163.com
@Date: 2025/05/25
@Version: V1.0
*/

#include "io_bmp_new.h"
#include <iostream>

namespace io_bmp {

	/* To little endian */
	static void
		to_little_endian(int32* words, int num_words) {
		// see if it already uses little-endian architecture
		int32 test = 1;
		byte* first_byte = reinterpret_cast<byte*>(&test);
		if (*first_byte) { return; } // if the first byte is not 0, then it's little-endian

		// otherwise change to little endian
		int32 tmp;
		for (; num_words--; words++) {
			tmp = *words;
			*words = ((tmp >> 24) & 0x000000FF) +
				((tmp >> 8) & 0x0000FF00) +
				((tmp << 8) & 0x00FF0000) +
				((tmp << 24) & 0xFF000000);
		}
	}
 
	/* From littlie endian */
	static inline void
		from_little_endian(int32* words, int num_words) {
		to_little_endian(words, num_words);
	}

	/***************************************/
	/*			BmpIn Implementation       */	
	/***************************************/
	BmpIn::BmpIn(BmpIn&& other) noexcept
		: num_components_(other.num_components_),
		rows_(other.rows_),
		cols_(other.cols_),
		num_unread_rows_(other.num_unread_rows_),
		line_bytes_(other.line_bytes_),
		alignment_bytes_(other.alignment_bytes_),
		in_(std::move(other.in_)) {};

	BmpIn& BmpIn::operator=(BmpIn&& other) noexcept {
		if (this != &other) {
			num_components_ = other.num_components_;
			rows_ = other.rows_;
			cols_ = other.cols_;
			num_unread_rows_ = other.num_unread_rows_;
			line_bytes_ = other.line_bytes_;
			alignment_bytes_ = other.alignment_bytes_;
			in_ = std::move(other.in_);
		}
		return *this;
	}

	Error BmpIn::open(const std::string& filename) {
		*this = BmpIn(); // reset all members via default constructor

		in_.reset(std::fopen(filename.c_str(), "rb")); // read in binary
		if (!in_) { return Error::NoFile; }

		byte magic[14];
		std::fread(magic, 1, 14, in_.get());
		if (magic[0] != 'B' || magic[1] != 'M') { return Error::FileHeader; } // 

		Header header;
		if (std::fread(&header, 1, 40, in_.get()) != 40) { return Error::FileTrunc; }

		from_little_endian(reinterpret_cast<int32*>(&header), 10);

		cols_ = header.width;
		rows_ = header.height;
		int bit_count = header.planes_bits >> 16;

		if (bit_count == 24)
			num_components_ = 3;
		else if (bit_count == 8)
			num_components_ = 1;
		else
			return Error::Unsupported;

		int palette_entries = 0;
		if (num_components_ == 1) {
			palette_entries = (header.num_colors_used == 0) ? (1 << bit_count) : header.num_colors_used;
		}

		int header_size = 14 + 40 + palette_entries * 4; // 4 bytes for each palette (BGR + reserved byte(0 by default)) 

		// offset parsed from BMP file
		int offset = 0;
		offset |= magic[10];
		offset |= magic[11] << 8;
		offset |= magic[12] << 16;
		offset |= magic[13] << 24;

		if (offset < header.size) { return Error::FileHeader; }

		// jump over palette
		if (palette_entries > 0)
			std::fseek(in_.get(), palette_entries * 4, SEEK_CUR);
		if (offset > header.size)
			std::fseek(in_.get(), offset - header.size, SEEK_CUR);

		num_unread_rows_ = rows_;
		line_bytes_ = cols_ * num_components_;
		alignment_bytes_ = (4 - (line_bytes_ % 4)) % 4; // padding to 4 bytes

		return Error::None;
	}

	void BmpIn::close() {
		in_.reset();

		// clear all resources
		num_components_ = 0;
		rows_ = 0;
		cols_ = 0;
		num_unread_rows_ = 0;
		line_bytes_ = 0;
		alignment_bytes_ = 0;
	}

	Error BmpIn::get_line(std::vector<byte>& line) {
		if (!in_ || num_unread_rows_ <= 0) {
			return Error::FileNotOpen;
		}

		// make sure line is large enough as buffer 
		line.resize(static_cast<size_t>(line_bytes_));

		// read each row of data 
		if (std::fread(line.data(), 1, static_cast<size_t>(line_bytes_), in_.get()) != static_cast<size_t>(line_bytes_)) {
			return Error::FileTrunc;
		}

		// read and dispose aligned bytes
		if (alignment_bytes_ > 0) {
			byte padding[3];
			std::fread(padding, 1, static_cast<size_t>(alignment_bytes_), in_.get());
		}

		--num_unread_rows_;

		return Error::None;
	}
	/***************************************/
	/*			BmpOut Implementation      */
	/***************************************/
	// Move constructor for BmpOut
	BmpOut::BmpOut(BmpOut&& other) noexcept
		: num_components_(other.num_components_),
		rows_(other.rows_),
		cols_(other.cols_),
		num_unread_rows_(other.num_unread_rows_),
		line_bytes_(other.line_bytes_),
		alignment_bytes_(other.alignment_bytes_),
		out_(std::move(other.out_)) {
	}

	// Move assignment operator
	BmpOut& BmpOut::operator=(BmpOut&& other) noexcept {
		if (this != &other) {
			num_components_ = other.num_components_;
			rows_ = other.rows_;
			cols_ = other.cols_;
			num_unread_rows_ = other.num_unread_rows_;
			line_bytes_ = other.line_bytes_;
			alignment_bytes_ = other.alignment_bytes_;
			out_ = std::move(other.out_);
		}
		return *this;
	}

	// Open function for BmpOut
	Error BmpOut::open(const std::string filename) {
		*this = BmpOut(); // reset all members

		// Example dimensions and pixel format should be set outside before calling open
		if (num_components_ != 1 && num_components_ != 3)
			return Error::Unsupported;

		int header_bytes = 14 + sizeof(Header);
		if (num_components_ == 1) header_bytes += 1024; // Color palette

		line_bytes_ = num_components_ * cols_;
		alignment_bytes_ = (4 - line_bytes_) & 3;

		int file_bytes = header_bytes + (line_bytes_ + alignment_bytes_) * rows_;

		byte magic[14];
		magic[0] = 'B'; magic[1] = 'M';
		magic[2] = static_cast<byte>(file_bytes);
		magic[3] = static_cast<byte>(file_bytes >> 8);
		magic[4] = static_cast<byte>(file_bytes >> 16);
		magic[5] = static_cast<byte>(file_bytes >> 24);
		magic[6] = magic[7] = magic[8] = magic[9] = 0;
		magic[10] = static_cast<byte>(header_bytes);
		magic[11] = static_cast<byte>(header_bytes >> 8);
		magic[12] = static_cast<byte>(header_bytes >> 16);
		magic[13] = static_cast<byte>(header_bytes >> 24);

		Header header;
		header.size = 40;
		header.width = cols_;
		header.height = rows_;
		header.planes_bits = 1;
		header.planes_bits |= ((num_components_ == 1) ? 8 : 24) << 16;
		header.compression = 0;
		header.image_size = 0;
		header.xpels_per_metre = 0;
		header.ypels_per_metre = 0;
		header.num_colors_used = 0;
		header.num_color_important = 0;

		to_little_endian(reinterpret_cast<int32*>(&header), 10);

		out_.reset(std::fopen(filename.c_str(), "wb"));
		if (!out_) return Error::NoFile;

		std::fwrite(magic, 1, 14, out_.get());
		std::fwrite(&header, 1, 40, out_.get());

		if (num_components_ == 1) {
			for (int n = 0; n < 256; ++n) {
				fputc(n, out_.get()); fputc(n, out_.get());
				fputc(n, out_.get()); fputc(0, out_.get());
			}
		}

		num_unread_rows_ = rows_;
		return Error::None;
	}

	// Close function
	void BmpOut::close() {
		out_.reset();
		num_components_ = 0;
		rows_ = 0;
		cols_ = 0;
		num_unread_rows_ = 0;
		line_bytes_ = 0;
		alignment_bytes_ = 0;
	}

	// Write one line of pixels
	Error BmpOut::put_line(std::vector<byte>& line) {
		if (!out_ || num_unread_rows_ <= 0) return Error::FileNotOpen;

		if (std::fwrite(line.data(), 1, static_cast<size_t>(line_bytes_), out_.get()) != static_cast<size_t>(line_bytes_)) {
			std::cerr << "put_line: line size mismatch! Expected " << line_bytes_
				<< ", but got " << line.size() << std::endl;
			return Error::FileTrunc;
		}

		if (alignment_bytes_ > 0) {
			byte padding[3] = { 0, 0, 0 };
			std::fwrite(padding, 1, static_cast<size_t>(alignment_bytes_), out_.get());
		}

		--num_unread_rows_;
		return Error::None;
	}
} // namespace io_bmp
