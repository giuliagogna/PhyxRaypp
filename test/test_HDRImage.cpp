/*
* Copyright (c) 2026 Giulia Gogna, Riccardo Piazza.
 *
 * Licensed under the EUPL, Version 1.2 or – as soon they will be approved by
 * the European Commission - subsequent versions of the EUPL (the "Licence");
 * You may not use this work except in compliance with the Licence.
 * You may obtain a copy of the Licence at:
 *
 * https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the Licence is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the Licence for the specific language governing permissions and
 * limitations under the Licence.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>


// Included libraries to simulate full disk to check the errors in writing files
#include <streambuf>
#include <ostream>

import std;
import HDRImage;
import Color;
import auxiliary_functions;

// =========================================================================
// TEST 1: Coordinates validation
// =========================================================================
TEST_CASE("Testing valid coordinates (_valid_coordinates)") {
    // Creates a test image 5x10
    HDRImage img(5, 10);

    SUBCASE("Valid coordinates (inside the image limits)") {
        CHECK(img._valid_coordinates(0, 0) == true);       // Upper left
        CHECK(img._valid_coordinates(4, 9) == true);       // Lower right
        CHECK(img._valid_coordinates(2, 7) == true);       // Casual point
    }

    SUBCASE("Invalid coordinates (negative values)") {
        CHECK(img._valid_coordinates(-1, 5) == false);     // Negative x
        CHECK(img._valid_coordinates(5, -1) == false);     // Negative y
    }

    SUBCASE("Invalid coordinates (out of borders)") {
        CHECK(img._valid_coordinates(5, 5) == false);      // x over the right border
        CHECK(img._valid_coordinates(5, 10) == false);     // x and y over the limits
        CHECK(img._valid_coordinates(15, 19) == false);    // Casual point out of the borders
    }
}

// =========================================================================
// TEST 2: Pixel offset calculation
// =========================================================================
TEST_CASE("Testing 1D array offset calculation (_pixel_offset)") {
    // Creates a test image 5x7
    HDRImage img(5, 7);

    SUBCASE("Favourable cases") {
        CHECK(img._pixel_offset(3, 2) == 2 * 5 + 3);       // Expected: 13
        CHECK(img._pixel_offset(0, 0) == 0);               // Expected: 0
        CHECK(img._pixel_offset(4, 6) == 34);              // Expected: 6 * 5 + 4 = 34
    }
}

// =========================================================================
// TEST 3: Pixel read and write operations
// =========================================================================
TEST_CASE("Testing pixel read/write (get_pixel and set_pixel)") {
    // Creates a test image 6x10
    HDRImage img(6, 10);

    float r = 0.2f;
    float g = 0.8f;
    float b = 6.9f;

    Color c(r, g, b);

    int x = 5;
    int y = 8;

    // Write the color to the image
    img.set_pixel(x, y, c);

    // Read the color back
    Color pixel_color = img.get_pixel(x, y);

    // Using the is_close() method directly from the Color struct
    CHECK(pixel_color.is_close(c) == true);
}


// =========================================================================
// TEST 4: Endianness and byte swapping
// =========================================================================
TEST_CASE("Testing byte swapping (_swap_if_needed)") {
    // Create a 4-byte buffer in memory with a recognizable sequence
    std::array<uint8_t, 4> buffer = {0x12, 0x34, 0x56, 0x78};

    // Read the buffer into a 32-bit unsigned integer directly
    const auto raw_bytes = std::bit_cast<uint32_t>(buffer);

    // Determine the native endianness of the machine running the test
    HDRImage::Endianness native_endian = (std::endian::native == std::endian::little)
        ? HDRImage::Endianness::little_endian // if the native endianness is LE, native_endian = little_endian
        : HDRImage::Endianness::big_endian;   // else native_endian = big_endian

    // Determine the opposite endianness to force the swap
    HDRImage::Endianness non_native_endian = (native_endian == HDRImage::Endianness::little_endian)
        ? HDRImage::Endianness::big_endian     // if native_endian is LE, then non_native_endian = big_endian
        : HDRImage::Endianness::little_endian; // else non_native_endian = little_endian

    SUBCASE("File endianness matches native endianness (No swap expected)") {
        uint32_t processed_bytes = HDRImage::_swap_if_needed(raw_bytes, native_endian);

        // Convert back to an array of bytes to inspect them individually
        auto result_buffer = std::bit_cast<std::array<uint8_t, 4>>(processed_bytes);

        // The byte order must remain completely untouched
        CHECK(result_buffer == buffer); // result_buffer and buffer are both type array and C++ has the operator ==
                                        // to compare the elements of two arrays
    }

    SUBCASE("File endianness differs from native endianness (Swap expected)") {
        uint32_t processed_bytes = HDRImage::_swap_if_needed(raw_bytes, non_native_endian);

        // Convert back to an array of bytes
        auto result_buffer = std::bit_cast<std::array<uint8_t, 4>>(processed_bytes);

        std::ranges::reverse(buffer);       // reverts the original buffer

        CHECK(result_buffer == buffer);
    }
}

// =========================================================================
// TEST 5: Stream reading functions
// =========================================================================
TEST_CASE("Testing stream reading (_read_float and _read_line)") {

    SUBCASE("Testing _read_float with binary data") {
        // We will test the reading of the float value 1.0f.
        // In IEEE 754, 1.0f is represented as 0x3F800000.
        // Little-Endian memory layout: 00 00 80 3F
        // Big-Endian memory layout:    3F 80 00 00

        // Reading Little-Endian bytes
        // Note: we specify '4' to prevent the string constructor from stopping at \x00
        std::string le_data("\x00\x00\x80\x3F", 4);
        std::istringstream le_stream(le_data);
        auto le_res = HDRImage::_read_float(le_stream, HDRImage::Endianness::little_endian);

        CHECK(le_stream.good() == true);
        CHECK(le_res.value() == 1.0f);

        // Reading Big-Endian bytes
        std::string be_data("\x3F\x80\x00\x00", 4);
        std::istringstream be_stream(be_data);
        auto be_res = HDRImage::_read_float(be_stream, HDRImage::Endianness::big_endian);

        CHECK(be_stream.good() == true);
        CHECK(be_res.value() == 1.0f);

        // Broken stream (too few bytes)
        std::string broken_data("\x3F\x80", 2); // Only 2 bytes instead of 4
        std::istringstream broken_stream(broken_data);
        auto broken_res = HDRImage::_read_float(broken_stream, HDRImage::Endianness::big_endian);

        CHECK(broken_stream.good() == false); // streaming fails silently: red light on
        CHECK(broken_res.has_value() == false); // the function returns an empty optional

    }

    SUBCASE("Testing _read_line with various line endings") {
        // Create a simulated file with UNIX (\n), Windows (\r\n), and empty lines
        std::istringstream stream("First line\nSecond line\r\n\nFourth line");

        // Standard UNIX line: returns with \n
        auto line1 = HDRImage::_read_line(stream);
        REQUIRE(line1.has_value() == true);
        CHECK(line1.value() == "First line");

        // Windows line: returns with \n\r (the \r should be correctly stripped)
        auto line2 = HDRImage::_read_line(stream);
        REQUIRE(line2.has_value() == true);
        CHECK(line2.value() == "Second line");

        // Third line is a empty line, the function should skip it and return the next line
        // in the following call.

        // Last standard line
        auto line4 = HDRImage::_read_line(stream);
        REQUIRE(line4.has_value() == true);
        CHECK(line4.value() == "Fourth line");

        // End of file: trying to read further should result in an expected error
        auto line5 = HDRImage::_read_line(stream);
        CHECK(line5.has_value() == false);
    }
}


// =========================================================================
// TEST 6: Parsing endianness
// =========================================================================
TEST_CASE("Testing string to endianness conversion (_parse_endianness)") {

    SUBCASE("Valid positive numbers (Big Endian)") {
        CHECK(HDRImage::_parse_endianness("1.0").value() == HDRImage::Endianness::big_endian);
        CHECK(HDRImage::_parse_endianness("1").value() == HDRImage::Endianness::big_endian);
        CHECK(HDRImage::_parse_endianness("  +2.5  ").value() == HDRImage::Endianness::big_endian);
    }

    SUBCASE("Valid negative numbers (Little Endian)") {
        CHECK(HDRImage::_parse_endianness("-1.0").value() == HDRImage::Endianness::little_endian);
        CHECK(HDRImage::_parse_endianness("-1").value() == HDRImage::Endianness::little_endian);
        CHECK(HDRImage::_parse_endianness("  -0.5  ").value() == HDRImage::Endianness::little_endian);
    }

    SUBCASE("Invalid inputs (Zero or non-numeric)") {
        CHECK(HDRImage::_parse_endianness("0.0").has_value() == false);
        CHECK(HDRImage::_parse_endianness("0").has_value() == false);
        CHECK(HDRImage::_parse_endianness("hello").has_value() == false);
    }
}


// =========================================================================
// TEST 7: Parsing image size
// =========================================================================

TEST_CASE("Parsing image dimension width and height (_parse_img_size)") {
    SUBCASE("Valid input (positive integer numbers)") {
        std::string line_1_space = "10 3189";
        REQUIRE(HDRImage::_parse_img_size(line_1_space).has_value() == true);
        CHECK(HDRImage::_parse_img_size(line_1_space) == std::make_pair(10, 3189));

    }

    SUBCASE("Invalid input (wrong separation: comma)") {
        CHECK(HDRImage::_parse_img_size("10, 3189").has_value() == false); // add a comma
    }

    SUBCASE("Invalid input (invalid values)") {
        CHECK(HDRImage::_parse_img_size("-10 309").has_value() == false);     // first value is negative
        CHECK(HDRImage::_parse_img_size("80 -497").has_value() == false);     // second value is negative
        CHECK(HDRImage::_parse_img_size("0").has_value() == false);           // finds only one value
        CHECK(HDRImage::_parse_img_size("25 889 998").has_value() == false);  // finds three values
        CHECK(HDRImage::_parse_img_size("10 hello").has_value() == false);    // finds a value that is not a number
        CHECK(HDRImage::_parse_img_size("1.8 9.13").has_value() == false);    // finds two float
        CHECK(HDRImage::_parse_img_size("10.5 9").has_value() == false);      // finds first float
        CHECK(HDRImage::_parse_img_size("10 9.5").has_value() == false);      // finds second float

    }
}

// =========================================================================
// TEST 8: Integration Test (read_pfm_file(stream) and read_pfm_file(string))
// =========================================================================
TEST_CASE("Integration test: reading PFM images (read_pfm_file)") {

    // --- POSITIVE TESTS ---

    SUBCASE("Valid image: Little-Endian (testing both overloads)") {
        std::string filename = "images/reference_le.pfm";

        // Test overload 1: filename
        auto res_filename = HDRImage::read_pfm_file(filename);
        REQUIRE(res_filename.has_value() == true);

        // Test overload 2: stream
        std::ifstream stream(filename, std::ios::binary);
        REQUIRE(stream.is_open() == true);
        auto res_stream = HDRImage::read_pfm_file(stream);
        REQUIRE(res_stream.has_value() == true);

        // Extract both images
        auto& img_from_file = res_filename.value();
        auto& img_from_stream = res_stream.value();

        // Control dimensions for both
        CHECK(img_from_file.width == 3);   CHECK(img_from_stream.width == 3);
        CHECK(img_from_file.height == 2);  CHECK(img_from_stream.height == 2);

        // Verify pixel colors: check only the image from file, because the function uses read_pfm_file from
        // stream to return the image. Test on read_pfm_file(stream) comes free
        auto& img = img_from_file;

        // Row y = 0
        CHECK(img.get_pixel(0, 0).is_close(Color(1.0e1f, 2.0e1f, 3.0e1f)));
        CHECK(img.get_pixel(1, 0).is_close(Color(4.0e1f, 5.0e1f, 6.0e1f)));
        CHECK(img.get_pixel(2, 0).is_close(Color(7.0e1f, 8.0e1f, 9.0e1f)));

        // Row y = 1
        CHECK(img.get_pixel(0, 1).is_close(Color(1.0e2f, 2.0e2f, 3.0e2f)));
        CHECK(img.get_pixel(1, 1).is_close(Color(4.0e2f, 5.0e2f, 6.0e2f)));
        CHECK(img.get_pixel(2, 1).is_close(Color(7.0e2f, 8.0e2f, 9.0e2f)));
    }

    SUBCASE("Valid image: Big-Endian (testing both overloads)") {
        std::string filename = "images/reference_be.pfm";

        auto res_filename = HDRImage::read_pfm_file(filename);
        REQUIRE(res_filename.has_value() == true);

        std::ifstream stream(filename, std::ios::binary);
        REQUIRE(stream.is_open() == true);
        auto res_stream = HDRImage::read_pfm_file(stream);
        REQUIRE(res_stream.has_value() == true);

        auto& img = res_filename.value();

        CHECK(img.width == 3);
        CHECK(img.height == 2);

        // Row y = 0
        CHECK(img.get_pixel(0, 0).is_close(Color(1.0e1f, 2.0e1f, 3.0e1f)));
        CHECK(img.get_pixel(1, 0).is_close(Color(4.0e1f, 5.0e1f, 6.0e1f)));
        CHECK(img.get_pixel(2, 0).is_close(Color(7.0e1f, 8.0e1f, 9.0e1f)));

        // Row y = 1
        CHECK(img.get_pixel(0, 1).is_close(Color(1.0e2f, 2.0e2f, 3.0e2f)));
        CHECK(img.get_pixel(1, 1).is_close(Color(4.0e2f, 5.0e2f, 6.0e2f)));
        CHECK(img.get_pixel(2, 1).is_close(Color(7.0e2f, 8.0e2f, 9.0e2f)));
    }

    // --- NEGATIVE TESTS ---

    SUBCASE("Invalid file: Wrong format (not a PFM)") {
        auto result = HDRImage::read_pfm_file("images/wrong_format.txt");
        REQUIRE(result.has_value() == false);
        CHECK(result.error().message.starts_with("Non valid format. Expected 'PF' read "));
    }

    // --- REDUNDANT: I ALREADY HAVE UNIT TESTS, BUT I AM A BIT PARANOID ---
    SUBCASE("Invalid file: wrong dimensions") {
        auto result = HDRImage::read_pfm_file("images/non_valid_dimensions.pfm");
        REQUIRE(result.has_value() == false);
        CHECK(result.error().message.starts_with("Image dimensions must be greater than zero."));
    }


    // Distinguish between the case of a file that contains more binary data
    // than expected and the case of a file that contains less binary data than expected.
    // GG: Here try to make it a warning and not an error (like in the corresponding function in HDRImage.cppm)
    SUBCASE("Invalid file: more binary data than expected") {
        auto result = HDRImage::read_pfm_file("images/too_long.pfm");
        REQUIRE(result.has_value() == false);
        CHECK(result.error().message == "Unexpected data after reading all pixels.");
    }

    SUBCASE("Invalid file: less binary data than expected") {
        auto result = HDRImage::read_pfm_file("images/too_short.pfm");
        REQUIRE(result.has_value() == false);
        CHECK(result.error().message.starts_with("Truncated file: expected 4 bytes for a float but only read"));
    }
    
    SUBCASE("Invalid file: Non-existent file") {
        auto result = HDRImage::read_pfm_file("images/this_file_does_not_exist_123.pfm");
        REQUIRE(result.has_value() == false);
        CHECK(result.error().message.starts_with("Error in opening input file '"));
    }
}

// =========================================================================
// TEST 9: Stream writing functions (_write_float)
// =========================================================================
TEST_CASE("Testing binary float writing (_write_float)") {

    std::ostringstream stream(std::ios::binary);

    // Write the number 1.0f that in IEEE 754 is 0x3F800000.
    SUBCASE("Writing Little-Endian") {
        HDRImage::_write_float(stream, 1.0f, HDRImage::Endianness::little_endian);
        std::string result = stream.str();

        // Expected order in little endian: 00 00 80 3F
        REQUIRE(result.length() == 4);
        CHECK(result[0] == '\x00');
        CHECK(result[1] == '\x00');
        CHECK(result[2] == '\x80');
        CHECK(result[3] == '\x3F');
    }

    SUBCASE("Writing Big-Endian") {
        HDRImage::_write_float(stream, 1.0f, HDRImage::Endianness::big_endian);
        std::string result = stream.str();

        // Expected order in big endian: 3F 80 00 00
        REQUIRE(result.length() == 4);
        CHECK(result[0] == '\x3F');
        CHECK(result[1] == '\x80');
        CHECK(result[2] == '\x00');
        CHECK(result[3] == '\x00');
    }
}

// =========================================================================
// TEST 10: Image writing (stream and file) matching hardcoded bytes
// =========================================================================

// =========================================================================
// Support class to simulate a full disk
// =========================================================================


class FailingStreamBuf : public std::streambuf { // this class inherits from std::streambuf that is the
                                                 // "worker" that obeys to the manager std::ofstream and
                                                 // directly interacts with the memory
    // In this class I want to create a new "worker" that simulates a finite memory
    int limit = 0; // indicates the maximum bytes we can write on our simulated disk
    int count = 0; // tracks how many bytes the manager asked to write since now

protected:
    // xsputn is the function that takes n bytes from the manager ofstream and actually writes them in memory
    std::streamsize xsputn(const char* s, std::streamsize n) override {
        if (count + n > limit) { // if the number of bytes the manager asked to write since now plus the
                                 //number of bytes is requiring in this moment exceeds the limit
            std::streamsize allowed = limit - count; // calculates the space that is still available
            count += allowed; // updates the count
            return allowed; // returns a value lower than n and the 'failbit' in the stream is raised (red light)
        }
        count += n; // If there is space updates the count
        return n;   // and returns the number of bytes asked
    }
    // This function is called everytime stream << "P" passes a character at a time
    int overflow(int c) override {
        if (count < limit) { // if there is space available it writes the character
            count++;
            return c;
        }
        return traits_type::eof(); // if there is no space returns end of files that raises the
                                   //red light of the manager ofstream
    }
public:
    FailingStreamBuf(int byte_limit) : limit(byte_limit) {}
};

// =========================================================================
// End of support class
// =========================================================================

TEST_CASE("Testing write_pfm_file against hardcoded reference bytes") {

    // Recreate the image in memory
    HDRImage img(3, 2);
    img.set_pixel(0, 0, Color(1.0e1f, 2.0e1f, 3.0e1f));
    img.set_pixel(1, 0, Color(4.0e1f, 5.0e1f, 6.0e1f));
    img.set_pixel(2, 0, Color(7.0e1f, 8.0e1f, 9.0e1f));
    img.set_pixel(0, 1, Color(1.0e2f, 2.0e2f, 3.0e2f));
    img.set_pixel(1, 1, Color(4.0e2f, 5.0e2f, 6.0e2f));
    img.set_pixel(2, 1, Color(7.0e2f, 8.0e2f, 9.0e2f));

    SUBCASE("Matching Little-Endian hardcoded bytes (Stream and File)") {
        std::vector<uint8_t> reference_le_bytes = {
            0x50, 0x46, 0x0a, 0x33, 0x20, 0x32, 0x0a, 0x2d, 0x31, 0x2e, 0x30, 0x0a,
            0x00, 0x00, 0xc8, 0x42, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0x96, 0x43,
            0x00, 0x00, 0xc8, 0x43, 0x00, 0x00, 0xfa, 0x43, 0x00, 0x00, 0x16, 0x44,
            0x00, 0x00, 0x2f, 0x44, 0x00, 0x00, 0x48, 0x44, 0x00, 0x00, 0x61, 0x44,
            0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0xa0, 0x41, 0x00, 0x00, 0xf0, 0x41,
            0x00, 0x00, 0x20, 0x42, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x70, 0x42,
            0x00, 0x00, 0x8c, 0x42, 0x00, 0x00, 0xa0, 0x42, 0x00, 0x00, 0xb4, 0x42
        };

        // --- TEST 1: Stream Overload ---
        std::ostringstream buf(std::ios::binary);
        auto write_res_stream = img.write_pfm_file(buf, HDRImage::Endianness::little_endian);
        REQUIRE(write_res_stream.has_value() == true);

        auto view = buf.view();
        std::vector<uint8_t> generated_bytes_stream(view.begin(), view.end());
        CHECK(generated_bytes_stream == reference_le_bytes);

        // --- TEST 2: File Overload ---
        std::string filename = "images/test_out_le.pfm";
        auto write_res_file = img.write_pfm_file(filename, HDRImage::Endianness::little_endian);
        REQUIRE(write_res_file.has_value() == true);

        // Read back the raw bytes from the file we just created
        std::ifstream in_file(filename, std::ios::binary);
        REQUIRE(in_file.is_open() == true);
        std::vector<uint8_t> generated_bytes_file((std::istreambuf_iterator<char>(in_file)), std::istreambuf_iterator<char>());
        in_file.close();

        CHECK(generated_bytes_file == reference_le_bytes);
        std::filesystem::remove(filename); // Clean up the workspace
    }

    SUBCASE("Matching Big-Endian hardcoded bytes (Stream and File)") {
        std::vector<uint8_t> reference_be_bytes = {
            0x50, 0x46, 0x0a, 0x33, 0x20, 0x32, 0x0a, 0x31, 0x2e, 0x30, 0x0a, 0x42,
            0xc8, 0x00, 0x00, 0x43, 0x48, 0x00, 0x00, 0x43, 0x96, 0x00, 0x00, 0x43,
            0xc8, 0x00, 0x00, 0x43, 0xfa, 0x00, 0x00, 0x44, 0x16, 0x00, 0x00, 0x44,
            0x2f, 0x00, 0x00, 0x44, 0x48, 0x00, 0x00, 0x44, 0x61, 0x00, 0x00, 0x41,
            0x20, 0x00, 0x00, 0x41, 0xa0, 0x00, 0x00, 0x41, 0xf0, 0x00, 0x00, 0x42,
            0x20, 0x00, 0x00, 0x42, 0x48, 0x00, 0x00, 0x42, 0x70, 0x00, 0x00, 0x42,
            0x8c, 0x00, 0x00, 0x42, 0xa0, 0x00, 0x00, 0x42, 0xb4, 0x00, 0x00
        };

        // --- TEST 1: Stream Overload ---
        std::ostringstream buf(std::ios::binary);
        auto write_res_stream = img.write_pfm_file(buf, HDRImage::Endianness::big_endian);
        REQUIRE(write_res_stream.has_value() == true);

        auto view = buf.view();
        std::vector<uint8_t> generated_bytes_stream(view.begin(), view.end());
        CHECK(generated_bytes_stream == reference_be_bytes);

        // --- TEST 2: File Overload ---
        std::string filename = "images/test_out_be.pfm";
        auto write_res_file = img.write_pfm_file(filename, HDRImage::Endianness::big_endian);
        REQUIRE(write_res_file.has_value() == true);

        // Read back the raw bytes
        std::ifstream in_file(filename, std::ios::binary);
        REQUIRE(in_file.is_open() == true);
        std::vector<uint8_t> generated_bytes_file((std::istreambuf_iterator<char>(in_file)), std::istreambuf_iterator<char>());
        in_file.close();

        CHECK(generated_bytes_file == reference_be_bytes);
        std::filesystem::remove(filename); // Clean up
    }

    SUBCASE("Handling file writing errors (Invalid Path)") {
        auto write_res = img.write_pfm_file("fake_dir_12345/impossible.pfm", HDRImage::Endianness::little_endian);
        REQUIRE(write_res.has_value() == false);
        // Here the error is in the opening of the file
        CHECK(write_res.error().starts_with("Error in opening output file"));
    }

    SUBCASE("Simulating disk full during pixel writing") {
        // The PFM header (PF\n3 2\n-1.0\n) occupies 12 bytes.
        // Fake disk limit is 15 bytes.
        FailingStreamBuf failing_buf(15);

        // Create a stream that uses the fake disk
        std::ostream failing_stream(&failing_buf);

        // Try to write the image: fist pixel red value is a float that takes 4 bytes. 12+4=16 that exceeds the
        // limit of the fake disk
        auto write_res = img.write_pfm_file(failing_stream, HDRImage::Endianness::little_endian);

        // The operation has to fail
        REQUIRE(write_res.has_value() == false);

        // Now this error arises
        CHECK(write_res.error() == "Error in writing pixel colors. Memory full or disconnected.");
    }
}


// =========================================================================
// TEST 11: Image average luminosity (average_luminosity)
// =========================================================================

TEST_CASE("Testing average_luminosity that calculates average luminosity of an HDR image") {

    HDRImage img(2, 1);
    img.set_pixel(0, 0, Color(  5.0,   10.0,   15.0));  // Luminosity: 10.0
    img.set_pixel(1, 0, Color(500.0, 1000.0, 1500.0));  // Luminosity: 1000.0

    SUBCASE("Wrong method argument: invalid Color::luminosity_... method passed") {
        REQUIRE(img.average_luminosity(1.0, "invalid_method").has_value() == false);
        CHECK(img.average_luminosity(1.0, "invalid_method").error().starts_with("Invalid luminosity type. Expected 'mid_range', 'arithmetic_mean' or 'bt709'.") );
    }

    SUBCASE("Negative luminosity in a pixel") {
        img.set_pixel(0, 0, Color(-5.0, 1.0, 2.0)); // Luminosity is negative because of the negative red component

        REQUIRE(img.average_luminosity(1.0, "mid_range").has_value() == false);
        CHECK(img.average_luminosity(1.0, "mid_range").error().starts_with("Negative luminosity value encountered"));
    }

    SUBCASE("Average luminosity passing delta=0.0 or negative") {
        // Test zero
        auto result_zero = img.average_luminosity(0.0, "mid_range");
        REQUIRE_FALSE(result_zero.has_value());
        CHECK(result_zero.error().starts_with("Delta value must be strictly greater than zero to avoid logarithm of zero or negative numbers."));

        // Test negative delta
        auto result_neg = img.average_luminosity(-2.0, "mid_range");
        REQUIRE_FALSE(result_neg.has_value());
        CHECK(result_neg.error().starts_with("Delta value must be strictly greater than zero to avoid logarithm of zero or negative numbers."));
    }

    SUBCASE("Average luminosity passing delta=10.0 to a black image") {

        // Img has been modified in the previous tests!!! Need to reset to a dark image
        // Passing delta = 10.0 the expected result 10.0
        // - first pixel: log2(0.0 + 10.0) ≈ 3.3219
        // - second pixel: log2(0.0 + 10.0) ≈ 3.3219
        // - average_luminosity result = 2^{(3.3219 + 3.3219)/2.0} = 2^(3.3219) = 10.0
        img.set_pixel(0, 0, Color(0.0, 0.0, 0.0));
        img.set_pixel(1, 0, Color(0.0, 0.0, 0.0));
        REQUIRE(img.average_luminosity(10.0, "mid_range").has_value());
        CHECK(aux::are_close(10.0, img.average_luminosity(10.0, "mid_range").value()));
    }
}

// =========================================================================
// TEST 12: Image normalization (normalize_image)
// =========================================================================

TEST_CASE("Test normalization of the image (normalize_image)") {
    HDRImage img(2, 1);

    SUBCASE("Normalize passing the luminosity") {

        img.set_pixel(0, 0, Color(  5.0,   10.0,   15.0));
        img.set_pixel(1, 0, Color(500.0, 1000.0, 1500.0));

        img.normalize_image(1000.0, 100.0);

        CHECK(img.get_pixel(0, 0).is_close(Color{50.0, 100.0, 150.0}));
        CHECK(img.get_pixel(1, 0).is_close(Color{5000.0, 10000.0, 15000.0}));

    }

    SUBCASE("Normalize not passing the luminosity") {

        img.set_pixel(0, 0, Color(0.0, 0.0, 0.0));
        img.set_pixel(1, 0, Color(0.0, 0.0, 0.0));

        img.normalize_image(1000.0, "mid_range", 1.0f);

        CHECK(img.get_pixel(0, 0).is_close(Color{0.0, 0.0, 0.0}));
        CHECK(img.get_pixel(1, 0).is_close(Color{0.0, 0.0, 0.0}));

    }

}

// =========================================================================
// TEST 13: Clamping images (clamp_image)
// =========================================================================

TEST_CASE("Testing image clamping (clamp_image)") {

    HDRImage img(2, 1);
    img.set_pixel(0, 0, Color(1.0f, 3.0f, 7.0f));
    img.set_pixel(1, 0, Color(9.0f, 15.0f, 19.0f));

    SUBCASE("Default clamping with factor 1.0") {
        auto result = img.clamp_image();

        REQUIRE(result.has_value());
        CHECK(img.get_pixel(0, 0).is_close(Color{0.5f, 0.75f, 0.875f}));
        CHECK(img.get_pixel(1, 0).is_close(Color{0.9f, 0.9375f, 0.95f}));
    }

    SUBCASE("Clamping with custom factor (0.5)") {
        auto result = img.clamp_image(0.5f);

        CHECK(result.has_value());

        // R: 0.5 / 1.5, G: 1.5 / 2.5, B: 3.5 / 4.5
        CHECK(img.get_pixel(0, 0).is_close(Color{0.5f/1.5f, 1.5f/2.5f, 3.5/4.5}));
        // R: 4.5 / 5.5, G: 7.5 / 8.5, B: 9.5 / 10.5
        CHECK(img.get_pixel(1, 0).is_close(Color{4.5f/5.5f, 7.5f/8.5f, 9.5f/10.5f}));
    }

    SUBCASE("Invalid factor (<= 0)") {
        auto result0 = img.clamp_image(0.0f);
        CHECK_FALSE(result0.has_value());
        CHECK(result0.error().starts_with("Factor needs to be strictly positive. Received: "));

        auto result1 = img.clamp_image(-1.0f);
        CHECK_FALSE(result1.has_value());
        CHECK(result0.error().starts_with("Factor needs to be strictly positive. Received: "));
    }

    SUBCASE("Negative pixel values") {
        img.set_pixel(0, 0, Color(-1.0f, 3.0f, 7.0f));
        auto result = img.clamp_image();
        CHECK_FALSE(result.has_value());
        CHECK(result.error() == ("Found negative pixel value."));
    }
}


// =========================================================================
// TEST 14: Testing gamma correction (apply_gamma_correction)
// =========================================================================


TEST_CASE("Gamma correction of the image.") {
    HDRImage img(2, 1);

    SUBCASE("Invalid gamma value (gamma <= 0), uniform") {
        REQUIRE(img.apply_gamma_correction(0.0001).has_value() == false);
        CHECK(img.apply_gamma_correction(0.0001).error().starts_with("Gamma value must be greater than zero."));
    }

    SUBCASE("Invalid gamma value (gamma <= 0), different for each channel") {
        REQUIRE(img.apply_gamma_correction(1.0f, 0.0f, 2.0f).has_value() == false);
        CHECK(img.apply_gamma_correction(1.0f, 0.0f, 2.0f).error().starts_with("Gamma value for green channel must be greater than zero."));
    }

    SUBCASE("Gamma correction with gamma=2.0, uniform") {
        img.set_pixel(0, 0, Color(0.25f, 0.5f, 0.75f));
        img.set_pixel(1, 0, Color(1.0f, 1.5f, 2.0f));

        img.apply_gamma_correction(2.0f);

        CHECK(img.get_pixel(0, 0).is_close(Color{0.5f, 0.70710678f, 0.8660254f}));
        CHECK(img.get_pixel(1, 0).is_close(Color{1.0f, 1.22474487f, 1.41421356f}));
    }

    SUBCASE("Gamma correction with different gamma values for each channel") {
        img.set_pixel(0, 0, Color(0.25f, 0.5f, 0.75f));
        img.set_pixel(1, 0, Color(1.0f, 1.5f, 2.0f));

        img.apply_gamma_correction(2.0f, 3.0f, 4.0f);

        CHECK(img.get_pixel(0, 0).is_close(Color{0.5f, 0.79370053f, 0.93060486f}));
        CHECK(img.get_pixel(1, 0).is_close(Color{1.0f, 1.14471424f, 1.18920712f}));
    }
}


// =========================================================================
// TEST 15: Integration test LDR image writing (write_ldr_image)
// =========================================================================
TEST_CASE("Integration test: writing LDR PNG images (write_ldr_image)") {

    HDRImage img(4, 2);

    // Row 0: Normal colors in range [0.0, 1.0]
    img.set_pixel(0, 0, Color(0.0f, 0.0f, 0.0f));    // Black
    img.set_pixel(1, 0, Color(1.0f, 1.0f, 1.0f));    // White
    img.set_pixel(2, 0, Color(0.5f, 0.5f, 0.5f));    // Grey
    img.set_pixel(3, 0, Color(0.1f, 0.2f, 0.3f));    // Mixed color

    // Row 1: Out of range colors to test internal clamping
    img.set_pixel(0, 1, Color(-1.0f, 0.0f, 0.0f));   // Negative value (becomes 0)
    img.set_pixel(1, 1, Color(2.0f, 0.0f, 0.0f));    // >1.0 rosso (becomes 255)
    img.set_pixel(2, 1, Color(0.0f, 5.0f, 0.0f));    // >1.0 verde
    img.set_pixel(3, 1, Color(0.0f, 0.0f, 10.0f));   // >1.0 blu

    SUBCASE("Successfully writing a PNG file to disk") {

        std::string filename = "test/test_output_ldr.png";
        auto result = img.write_ldr_image(filename);

        REQUIRE(result.has_value() == true);
        // File exists
        REQUIRE(std::filesystem::exists(filename) == true);
        // File is not empty
        REQUIRE(std::filesystem::file_size(filename) > 0);

        // Remove file: if you want to see this flabbergasting image just comment the following line
        std::filesystem::remove(filename);
    }

    SUBCASE("Handling file writing errors (Invalid Path)") {

        auto result = img.write_ldr_image("non_existent_directory/impossibile.png");

        REQUIRE(result.has_value() == false);
        CHECK(result.error().starts_with("Failed to write LDR PNG image to file"));
    }
}
