/*
This is a reconstructed verison based on David Taubman's io_bmp.h 
using modern cpp techniques.
@Author: Chris, zqchris1998@163.com
@Date: 2025/05/25
@Version: V1.0
*/

#pragma once

#include <stdio.h> // FILE type
#include <cstdint>
#include <fstream>
#include <vector>
#include <string>
#include <memory>

namespace io_bmp {
	
	// data types 
	using uint32 = std::uint32_t;
	using int32 = std::int32_t;
	using byte = std::uint8_t;

	// define Error Code
	enum class Error {
		None = 0,
		NoFile,
		FileHeader,
		FileTrunc, // if file ends unexpectely
		Unsupported,
		FileNotOpen
	};

	struct Header {
		uint32 size = 40;
		int32 width = 0;
		int32 height = 0;
		uint32 planes_bits = 0;
		uint32 compression = 0; // only accept 0 here (uncompressed RGB data)
		uint32 image_size = 0;
		int32 xpels_per_metre = 0;
		int32 ypels_per_metre = 0;
		uint32 num_colors_used = 0; // 0 means use default 
		uint32 num_color_important = 0; // 0 means all colors are important
	};

	class BmpIn {
	public:
		BmpIn() = default;
		~BmpIn() {};

		// ban copy constructor and copy assignment operator
		BmpIn(const BmpIn&) = delete;
		BmpIn& operator=(const BmpIn&) = delete;

		BmpIn(BmpIn&&) noexcept;
		BmpIn& operator=(BmpIn&&) noexcept;

		// member functions
		Error open(const std::string& filename);
		void close();
		Error get_line(std::vector<byte>& line);

		// getters 
		int get_width() const { return cols_; }
		int get_height() const { return rows_; }
		int get_num_components() const { return num_components_; }

	private:
		int num_components_ = 0;
		int rows_ = 0; 
		int cols_ = 0;
		int num_unread_rows_ = 0;
		int line_bytes_ = 0; // Number of bytes in each line, not including padding
		int alignment_bytes_ = 0; // Bytes at the end of each line  to make a multiple of 4 bytes
		std::unique_ptr<FILE, decltype(&std::fclose)> in_{ nullptr, &std::fclose }; // use user defined deleter
	};

	class BmpOut {
	public:
		BmpOut() = default;
		~BmpOut() {};

		// ban copy constructor and copy assignment operator
		BmpOut(const BmpOut&) = delete;
		BmpOut& operator=(const BmpOut&) = delete;

		BmpOut(BmpOut&&) noexcept;
		BmpOut& operator=(BmpOut&&) noexcept;

		// member functions
		Error open(const std::string filename);
		void close();
		Error put_line(std::vector<byte>& line);

		// setters
		void set_width(int width) { cols_ = width; }
		void set_height(int height) { rows_ = height; }
		void set_num_components(int num_components) { num_components_ = num_components; }

	private:
		int num_components_ = 0;
		int rows_ = 0;
		int cols_ = 0;
		int num_unread_rows_ = 0;
		int line_bytes_ = 0; // Number of bytes in each line, not including padding
		int alignment_bytes_ = 0; // Bytes at the end of each line  to make a multiple of 4 bytes
		std::unique_ptr<FILE, decltype(&std::fclose)> out_{ nullptr, &std::fclose }; // use user defined deleter
	};

} // namespace io_bmp


