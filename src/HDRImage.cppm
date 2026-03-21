module;
// C-style macros (like assert) must be included in the global module fragment
#include <cassert>
#include <cstdint> // includes uint8_t, uint32_t
#include <fstream>

export module HDRImage;

import std;
import Color;
import auxiliary_functions; // contains open_input_file used in read_pfm_image


export struct InvalidPfmFileFormat {
    std::string message;
};

export struct HDRImage {
    // =========================================================================
    // ENUMS & TYPES
    // =========================================================================
    enum class Endianness { little_endian, big_endian };

    // =========================================================================
    // DATA MEMBERS
    // =========================================================================
    int width = 0;   // default value
    int height = 0;  // default value
    std::vector<Color> pixels{}; // default value: empty vector

    // =========================================================================
    // CONSTRUCTORS & DESTRUCTOR
    // =========================================================================

    // Default constructor: creates a valid empty image 0x0
    HDRImage() = default;

    // "Manual" constructor: creates an image of requested dimensions with pixels initialized to 0.
    HDRImage(int w, int h) : width(w), height(h), pixels(w * h, Color(0.0f, 0.0f, 0.0f)) {}

    // Note: constructor from file is delegated to the static function read_pfm_file that reads a pfm file and returns
    // an HDRImage with its content. This allows the safe construction with expected value

    ~HDRImage() = default;

    // =========================================================================
    // PIXEL ACCESS & GEOMETRY
    // =========================================================================

    // Checks the validity of pixel coordinates.
    // Marked 'const' because it does not modify the image state.
    // [[nodiscard]] raises a warning if I call _valid_coordinates without saving its output in a variable
    [[nodiscard]] bool _valid_coordinates(const int x, const int y) const {
        return (x >= 0 && x < width && y >= 0 && y < height);
    }

    // Calculates the 1D array index from 2D coordinates.
    // [[nodiscard]] raises a warning if I call _pixel_offset without saving its output in a variable
    [[nodiscard]] int _pixel_offset(const int x, const int y) const {
        assert(_valid_coordinates(x, y) && "Coordinates are out of bounds");
        return x + (y * width);
    }

    // Retrieves the pixel color at the given coordinates.
    // [[nodiscard]] raises a warning if I call get_pixel without saving its output: why should I call
    // get_pixel and not save its output anywhere
    [[nodiscard]] Color get_pixel(const int x, const int y) const {
        assert(_valid_coordinates(x, y) && "Coordinates are out of bounds");
        return pixels[_pixel_offset(x, y)];
    }

    // Updates the pixel color at the given coordinates.
    void set_pixel(const int x, const int y, const Color& c) {
        assert(_valid_coordinates(x, y) && "Coordinates are out of bounds");
        pixels[_pixel_offset(x, y)] = c;
    }

    // =========================================================================
    // STATIC I/O UTILITIES
    // =========================================================================

    // Support function to check endianness: if the endianness of the file and the native OS endianness
    // are different, it swaps the bytes order
    static uint32_t _swap_if_needed(const uint32_t raw_bytes, Endianness file_endianness) {
        bool is_file_little = (file_endianness == Endianness::little_endian);
        bool is_native_little = (std::endian::native == std::endian::little);

        if (is_file_little != is_native_little) {
            return std::byteswap(raw_bytes);
        }

        return raw_bytes;
    }

    // =========================================================================
    // READING FROM STREAM AND FILE
    // =========================================================================

    // Reads a sequence of 4 bytes from a binary stream and converts it to a float.
    // Here I do not use expectation values because this function will be used in a massive for loop
    // and it would slow it down a lot
    // I will perform a check at the end of the function that reads the pfm
    static float _read_float(std::istream& stream, Endianness file_endianness) {
        uint32_t raw_bytes = 0.0f; // default value if the reading fails

        // Note: when we use uint32_t the type is an integer representation with 32 bit (exactly 4 bytes).
        // The fact that we extract exactly 4 bytes from the stream is ensured by sizeof(raw_bytes) that is the
        // size of a uint32_t, that is 4.

        // Control if the reading of the file fails
        stream.read(reinterpret_cast<char*>(&raw_bytes), sizeof(raw_bytes));

        // stream.read copies the sequence of 4 bytes exactly as it is in the RAM memory
        // we need to swap the bytes if the endianness of the file differs from the native endianness
        // because then we use bit_cast<float>(raw_bytes) that takes those 4 bytes and tells the OS to
        // convert them to float. bit_cast does not know about the endianness of the file we are taking the
        // bytes from, and will interpret them with the native endianness of the system: so we need to write
        // the bytes in the order expected by the native endianness before interpreting them to float

        raw_bytes = _swap_if_needed(raw_bytes, file_endianness);

        return std::bit_cast<float>(raw_bytes);
    }

    // Extracts a string from the stream up to a newline '\n', handling '\r' gracefully.
    static std::expected<std::string, InvalidPfmFileFormat> _read_line(std::istream& stream) {
        std::string result;

        // getline uses '\n' as default stop character: it reads each character and attaches it to
        // the string until '\n' is reached.
        // getline reads '\n' but it does not include it in the string.
        // If getline does not encounter '\n' at the end of the line (example end of file) the string is still valid
        // and rises 'endofbit'
        // If getline encounters an empty line, that only contains '\n', it extracts '\n' but it does not attach it to
        // the string end it returns an empty string without raising errors (the extraction was successful)
        // If getline does not encounter a line (for example tries to read the fifth line in a file that
        // has four lines, it fails and raises error).

        if (!std::getline(stream, result)) {
            return std::unexpected(InvalidPfmFileFormat{"Impossible to read line."});
        }

        // If the file is downloaded on Windows, the line returns with the character '\r' followed by '\n'.
        // getline reads '\r' and includes it in the string, but we are not interested in that: pop_back eliminates the
        // last character, so our line does not contain any '\r'

        if (result.ends_with('\r')) {
            result.pop_back();
        }

        return result;
    }

    // Converts the endianness line to the correspondent Endianness type checking the sign
    static std::expected<Endianness, InvalidPfmFileFormat> _parse_endianness(const std::string& line) {
        std::istringstream stream(line);
        float value;

        // If the line that should contain the endianness contains something that is not a number, raises an error
        if (!(stream >> value)) {
            return std::unexpected(InvalidPfmFileFormat{"Invalid endianness format. Read: '" + line + "'."});
        }

        // Evaluate only the sign of the value read in the line: if positive little_endian, if negative big_endian
        if (value > 0.0f) {
            return Endianness::big_endian;
        } else if (value < 0.0f) {
            return Endianness::little_endian;
        } else {
            // If the value is zero it cannot be interpreted as an endianness
            return std::unexpected(InvalidPfmFileFormat{"Invalid endianness. Value cannot be zero. Read: '" + line + "'."});
        }
    }

    static std::expected<std::pair<int, int>, InvalidPfmFileFormat> _parse_img_size(const std::string& line) {
        std::istringstream stream(line);
        int parsed_width, parsed_height;

        // stream >> extracts the first value, skips the spacing and extracts the second value
        // the fact that stream >> skips the spaces is useful because our values may be separated ny more than one
        // 1 space or be followed by white spaces

        // If one tries to read a first number that is not an integer like 10.1, the first >> will read 10 and
        // put it into parsed_width.
        // The second >> will encounter . that is not a number and it will raise the unexpected value
        if (!(stream >> parsed_width >> parsed_height)) {
            return std::unexpected(InvalidPfmFileFormat{"Cannot extract image dimensions from line. Read: " + line});
        }

        // If one tries to read second number that is not an integer like 10 20.1
        // The first >> will read 10 and put it into parsed_width
        // The second >> will read 20 and put it into parsed_height
        // Then this third stream will be called and it will encounter . and therefore it will raise an error for
        // too many values

        stream >> std::ws; // eliminates the remaining white spaces
        // If after all the remaining white spaces we have not found the end of the line it means that there are too
        // many values
        if (!stream.eof()) {
            return std::unexpected(InvalidPfmFileFormat{"Too many values for dimensions. Invalid format: " + line});
        }

        // Here the sign of the numbers read are controlled
        if (parsed_width <= 0 || parsed_height <= 0) {
            return std::unexpected(InvalidPfmFileFormat{"Image dimensions must be greater than zero. Read: " + line});
        }

        return std::make_pair(parsed_width, parsed_height);
    }

    // Reads a PFM from stream
    static std::expected<HDRImage, InvalidPfmFileFormat> read_pfm_file(std::istream& stream) {
        auto format = _read_line(stream);
        if (!format.has_value()) {
            return std::unexpected(format.error());
        };
        if (format.value() != "PF") {
            return std::unexpected(InvalidPfmFileFormat{"Non valid format. Expected 'PF' read " + format.value()});
        }

        auto size_line = _read_line(stream);
        if (!size_line.has_value()) return std::unexpected(size_line.error());
        auto size_res = _parse_img_size(size_line.value());
        if (!size_res.has_value()) return std::unexpected(size_res.error());
        int width = size_res.value().first;
        int height = size_res.value().second;

        auto endian_line_res = _read_line(stream);
        if (!endian_line_res.has_value()) return std::unexpected(endian_line_res.error());
        auto endian_res = _parse_endianness(endian_line_res.value());
        if (!endian_res.has_value()) return std::unexpected(endian_res.error());

        Endianness endianness = endian_res.value();

        HDRImage img(width, height);

        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                // Read the three color channels for the current pixel
                float r = _read_float(stream, endianness);
                float g = _read_float(stream, endianness);
                float b = _read_float(stream, endianness);

                // Color the pixel
                img.set_pixel(x, y, Color(r, g, b));
            }
        }

        // Final check on the streaming: if there was a streaming error at step 1, the streaming raises the
        // red light and the for loop continues fast filling all the pixels with 0 (default value for _read_float)
        // At the end .good() checks if the red light was on and if so raises an error
        // This ensures maximum performance and safety
        if (!stream.good()) {
            return std::unexpected(InvalidPfmFileFormat{"Error in reading binary data: corrupted file."});
        }

        // Returned the finished image
        return img;
    }

    // Reads a PFM image directly from a filename.
    // This acts as a static factory, keeping the error-handling flow clean with std::expected.
    static std::expected<HDRImage, InvalidPfmFileFormat> read_pfm_file(const std::string& filename) {
        // Open the file using the open_input_file in auxiliary function
        auto stream_res = aux::open_input_file(filename);

        if (!stream_res.has_value()) {
            return std::unexpected(InvalidPfmFileFormat{stream_res.error()});
        }

        // Pass the successfully opened stream to the main reading function
        // here read_pfm_file is the function written above that accepts a stream and not a string
        return read_pfm_file(stream_res.value());
    }




    // =========================================================================
    // WRITING TO STREAM
    // =========================================================================

    // Safely converts a float to a 4-byte sequence and writes it to a binary stream.
    static void _write_float(std::ostream& stream, float value, Endianness file_endianness) {
        // Copy the float's exact bit pattern into an unsigned 32-bit integer.
        // Here the sequence of the 4 byte is in the order imposed by the native endianness of the OS
        uint32_t raw_bytes = std::bit_cast<uint32_t>(value);

        // If the native endianness differs from the endianness I want my file to be written in
        // I need to swap the bytes
        raw_bytes = _swap_if_needed(raw_bytes, file_endianness);

        // When the bytes have the order that I want based on the endianness I have control on
        // I can write them on the stream
        stream.write(reinterpret_cast<const char*>(&raw_bytes), sizeof(raw_bytes));

        // This way when I declare the endianness in the file I am sure that my float values are represented
        // with the correct endianness
    }

    // Writes a pfm on a stream
    // [[nodiscard]]: if a user tries to use it ignoring its return value the compiler gives a warning: if
    // something bad happens, if the error value is not caught in the main the program proceeds. with [[nodiscard]]
    // the user is warned to catch the return value of this function in the main
    [[nodiscard]] std::expected<void, std::string> write_pfm_file(std::ostream& stream, Endianness file_endianness) const {

        // Endianness line
        std::string endianness_string = (file_endianness == Endianness::little_endian) ? "-1.0" : "1.0";

        stream << "PF\n" << width << " " << height << "\n" << endianness_string << "\n";

        // If there was a problem in writing the header catch it: when stream << encounters a problem it raises
        // a red light and all the following writing operations are ignored. At the end stream.good() checks
        // if the red light was turned on and reports it.
        if (!stream.good()) {
            return std::unexpected(std::format("Error in writing header. Memory full or disconnected."));
        }

        for (int y = height -1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                Color c = get_pixel(x, y);
                _write_float(stream, c.r, file_endianness);
                _write_float(stream, c.g, file_endianness);
                _write_float(stream, c.b, file_endianness);
            }
        }

        // If there was a problem in writing the pixel colors catch it: just check it at the end of the for loop
        // If I put a check inside the _write_float function the processor needs to check an if condition at every
        // iteration compromising performance.
        // Checking at the end of the stream is safe enough because if something went wrong at some point
        // the red light is on and .good() reports it
        if (!stream.good()) {
            return std::unexpected(std::format("Error in writing pixel colors. Memory full or disconnected."));
        }

        return {};
    }

    // Writes a pfm directly on file
    [[nodiscard]] std::expected<void, std::string> write_pfm_file(const std::string& filename, Endianness file_endianness) const {

        auto stream_res = aux::open_output_file(filename);

        if (!stream_res.has_value()) {
            return std::unexpected(stream_res.error());
        }

        auto write_res = write_pfm_file(stream_res.value(), file_endianness);

        // Check if the writing on the file was successful: if not return the error with filename and the exact
        // point where the streaming failed (error in the header or error in the pixel colors)
        if (!write_res.has_value()) {
            return std::unexpected(std::format("Failed to write to file '{}': {}", filename, write_res.error()));
        }

        // Return void to signal complete success
        return {};
    }


};