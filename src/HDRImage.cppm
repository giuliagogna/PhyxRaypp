module;
// C-style macros (like assert) must be included in the global module fragment
#include <cassert>
#include <fstream>
#include <cmath>

export module HDRImage;

import std;
import Color;
import auxiliary_functions; // contains open_input_file used in read_pfm_image



// RP: What if whe just use strings as error values instead of defining a custom struct for the error?
//     Pros: simpler code, no need to define a custom struct for the error, can use std::format to create detailed error messages on the fly.
//     For example: in HDRImage::average_luminosity there could be an invalid method atgument or an empty image. I would not say these
//     error type are the same since they are different errors. In fact, the type I choose is std::expected<float, std::string>.

export struct InvalidPfmFileFormat {
    std::string message;
};

//export struct InvalidMethodArgument {
//    std::string message;
//};

export struct HDRImage {
    // =========================================================================
    // ENUMS & TYPES
    // =========================================================================
    enum class Endianness { little_endian, big_endian };

    // =========================================================================
    // DATA MEMBERS
    // =========================================================================
    int width{};   // default value
    int height{};  // default value
    std::vector<Color> pixels{}; // default value: empty vector

    // =========================================================================
    // CONSTRUCTORS & DESTRUCTOR
    // =========================================================================

    // Default constructor: creates a valid empty image 0x0
    //HDRImage() = default;  // RP: don't need to be explicitly defaulted, the compiler will generate it for us.

    // "Manual" constructor: creates an image of requested dimensions with pixels initialized to 0.
    HDRImage(int w, int h) : width(w), height(h), pixels(w * h, Color(0.0f, 0.0f, 0.0f)) {
        // Control if the dimensions are valid: if not throw an exception
        if (w <= 0 || h <= 0) {
            throw std::invalid_argument("Image dimensions must be greater than zero.");
        }
    }

    // Note: constructor from file is delegated to the static function read_pfm_file that reads a pfm file and returns
    // an HDRImage with its content. This allows the safe construction with expected value

    // ~HDRImage() = default;  // RP: don't need to be explicitly defaulted, the compiler will generate it for us.

    // =========================================================================
    // PIXEL ACCESS & GEOMETRY
    // =========================================================================

    // Checks the validity of pixel coordinates.
    // Marked 'const' because it does not modify the image state.
    // [[nodiscard]] raises a warning if I call _valid_coordinates without saving its output in a variable    RP: very cool!
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
            return std::byteswap(raw_bytes);    // efficiently swaps the byte order of the 32-bit integer
        }

        return raw_bytes;
    }

    // =========================================================================
    // READING FROM STREAM AND FILE
    // =========================================================================

    // Reads a sequence of 4 bytes from a binary stream and converts it to a float.
    // GG: Here I do not use expectation values because this function will be used in a massive for loop
    //     and it would slow it down a lot
    //     I will perform a check at the end of the function that reads the pfm

    // RP: It's massive but it's done just once. We should avoid overhead just for raytracing calculations,
    //     but for reading a file that is done just once we can afford to have a more robust function that checks the errors with expected values.

//    static float _read_float(std::istream& stream, Endianness file_endianness) {
//        // uint32_t raw_bytes = 0.0f; // default value if the reading fails     // RP: nosense. I can just declare it 0 without conversion from float.
//        uint32_t raw_bytes{}; // devault value will be 0 (representation on 0.0f in bits is 0x00000000, that is also 0 in uint32_t)
//
//        // Note: when we use uint32_t the type is an integer representation with 32 bit (exactly 4 bytes).
//        // The fact that we extract exactly 4 bytes from the stream is ensured by sizeof(raw_bytes) that is the
//        // size of a uint32_t, that is 4.
//
//        // Control if the reading of the file fails
//        stream.read(reinterpret_cast<char*>(&raw_bytes), sizeof(raw_bytes)); // reinterpret_cast<char*>(&raw_bytes) tells the compiler to treat the address of raw_bytes as a pointer to char (byte)
//
//        // stream.read copies the sequence of 4 bytes exactly as it is in the RAM memory
//        // we need to swap the bytes if the endianness of the file differs from the native endianness
//        // because then we use bit_cast<float>(raw_bytes) that takes those 4 bytes and tells the OS to
//        // convert them to float. bit_cast does not know about the endianness of the file we are taking the
//        // bytes from, and will interpret them with the native endianness of the system: so we need to write
//        // the bytes in the order expected by the native endianness before interpreting them to float
//
//        raw_bytes = _swap_if_needed(raw_bytes, file_endianness);
//
//        return std::bit_cast<float>(raw_bytes);
//    }

    // Reads a sequence of 4 bytes from a binary stream and converts it to a float.
    // This function is designed to be robust against various file format issues, such as truncated files or invalid data.
    static std::expected<float, InvalidPfmFileFormat> _read_float(std::istream& stream, Endianness file_endianness) {
        uint32_t raw_bytes{};

        // Reading 4 bytes from the stream into raw_bytes.
        stream.read(reinterpret_cast<char*>(&raw_bytes), sizeof(raw_bytes));

        // catches file faults and end of file.
        if (stream.fail()) {
            if (stream.gcount() > 0 && stream.gcount() < 4) {
                return std::unexpected(InvalidPfmFileFormat{"Truncated file: expected 4 bytes for a float but only read " + 
                                       std::to_string(stream.gcount()) + " bytes."});
            }
            return std::unexpected(InvalidPfmFileFormat{"Reading error: unable to read 4 bytes for a float."});
        }

        // Endianness handling
        raw_bytes = _swap_if_needed(raw_bytes, file_endianness);

        // Interpret raw bites with corrrect endianness as a float and return it
        return std::bit_cast<float>(raw_bytes);
    }


    // Reads a line in the header, skipping comments (lines starting with #) and empty lines.
    static std::expected<std::string, InvalidPfmFileFormat> _read_line(std::istream& stream) {
        std::string result;
        
        // read till we find a line that is not a comment (starting with #) and not empty (for example an extra newline)
        while (std::getline(stream, result)) {
            // remove carriage return \r
            if (!result.empty() && result.ends_with('\r')) {
                result.pop_back();
            }

            // if the line starts with # it is a comment and we skip it (flexibility)
            if (!result.empty() && result.starts_with('#')) {
                continue;
            }

            // if the line is empty we skip it (for example an extra newline) (flexibility)
            if (result.empty()) {
                continue;
            }

            return result;
        }

        return std::unexpected(InvalidPfmFileFormat{"Impossible to read line."});
    }

    // Converts the endianness line to the correspondent Endianness type checking the sign.
    // At this point the file is not binary yet, we are still reading the header that is in ASCII, so there is no need to read float
    // values with _read_float.

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

    // Parses the image dimensions from the size line, ensuring correct format and valid values.
    static std::expected<std::pair<int, int>, InvalidPfmFileFormat> _parse_img_size(const std::string& line) {
        std::istringstream stream(line);
        int parsed_width{}, parsed_height{};

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

    /////////////////////////////////////////////////////////////////////////
    // READING A PFM FILE
    /////////////////////////////////////////////////////////////////////////

    // Reads a PFM from stream. Handles the entire reading process, including format validation, dimension parsing, endianness handling, and pixel data extraction.
    // Errors are propagated using std::expected, allowing the caller to handle them gracefully.
    static std::expected<HDRImage, InvalidPfmFileFormat> read_pfm_file(std::istream& stream) {
        auto format = _read_line(stream);
        if (!format.has_value()) { // if the line that should contain the format cannot be read, return the error
            return std::unexpected(format.error());
        }
        if (format.value() != "PF") {
            return std::unexpected(InvalidPfmFileFormat{"Non valid format. Expected 'PF' read " + format.value()});
        }

        // Image size line
        auto size_line = _read_line(stream);
        if (!size_line.has_value()) return std::unexpected(size_line.error());
        auto size_res = _parse_img_size(size_line.value());
        if (!size_res.has_value()) return std::unexpected(size_res.error());
        int width = size_res.value().first;
        int height = size_res.value().second;

        // Endianness line
        auto endian_line_res = _read_line(stream);
        if (!endian_line_res.has_value()) return std::unexpected(endian_line_res.error());
        auto endian_res = _parse_endianness(endian_line_res.value());
        if (!endian_res.has_value()) return std::unexpected(endian_res.error());

        Endianness endianness = endian_res.value();

        HDRImage img(width, height);

        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                auto r_res = _read_float(stream, endianness);
                if (!r_res.has_value()) return std::unexpected(r_res.error());

                auto g_res = _read_float(stream, endianness);
                if (!g_res.has_value()) return std::unexpected(g_res.error());

                auto b_res = _read_float(stream, endianness);
                if (!b_res.has_value()) return std::unexpected(b_res.error());

                img.set_pixel(x, y, Color(r_res.value(), g_res.value(), b_res.value()));
            }
        }

        // Consume final white spaces or end of line characters (\n, \r, ' ')
        stream >> std::ws;

        // Try to read the next character
        if (!stream.eof()) {
            // If the file is not at the end return this error.
            // GG: I don't like this being an error, would like to make it a warning
            return std::unexpected(InvalidPfmFileFormat{"Unexpected data after reading all pixels."});
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

    // Provide file name and endianness, and this function will handle the entire writing process, including file opening,
    // header writing, pixel data streaming, and error handling with detailed messages.
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

    // =========================================================================
    // WRITING TO PNG. STB_IMAGE_WRITE BASED
    // =========================================================================
    
    // Writes a PNG file from the HDR image data.
    [[nodiscard]] std::expected<void, std::string> write_png_file(const std::string& filename) const {
        // Implementation for writing PNG files would go here.
        // This is a placeholder to indicate where PNG writing logic would be implemented.
        return std::unexpected("PNG writing not implemented yet.");
    }
    
    // Computes the average luminosity of the picture according to Shirley & Morley (2003) algorithm
    // RP: I'm passing from log10 to log2 which is more direct. Since these two logarithms differ by a constant factor log2(10), the final result is the same.
    //     pf: log2(x) = log10(x) / log10(2) => sum log2(x) = sum log10(x) / log10(2) => 10^(sum log10(x) / N) = 2^(sum log2(x) / N)
    [[nodiscard]] std::expected<float, std::string> average_luminosity(float delta = 1e-10, std::string luminosity_type = "bt709") const {

        float cumsum = 0.0f;

        float tollerance = 1e-6f; // tolerance for checking if the luminosity value is negative
        int length = pixels.size();
        if (!length) {
            return std::unexpected("Cannot compute average luminosity of an empty image.");
        }
        if (delta < tollerance) {
            return std::unexpected("Delta value must be greater than zero to avoid logarithm of zero or negative numbers. Received: " + std::to_string(delta));
        }

        float current_pixel_luminosity{};

        // Code duplication, I know, but I don't want to check which is the method via string each time...
        if (luminosity_type == "mid_range") {
            for (auto& pixel : pixels) {
                current_pixel_luminosity = pixel.luminosity_mid_range();
                if (current_pixel_luminosity < -tollerance) {
                    return std::unexpected("Negative luminosity value encountered in mid_range method. Luminosity values must be non-negative. Check the pixel values in the image.");
                }
                cumsum += log2(current_pixel_luminosity + delta);
            }
        } else if (luminosity_type == "arithmetic_mean") {
            for (auto& pixel : pixels) {
                current_pixel_luminosity = pixel.luminosity_arithmetic_mean();
                if (current_pixel_luminosity < -tollerance) {
                    return std::unexpected("Negative luminosity value encountered in arithmetic_mean method. Luminosity values must be non-negative. Check the pixel values in the image.");
                }
                cumsum += log2(current_pixel_luminosity + delta);
            }
        } else if (luminosity_type == "bt709") {
            for (auto& pixel : pixels) {
                current_pixel_luminosity = pixel.luminosity_bt709();
                if (current_pixel_luminosity < -tollerance) {
                    return std::unexpected("Negative luminosity value encountered in bt709 method. Luminosity values must be non-negative. Check the pixel values in the image.");
                }
                cumsum += log2(current_pixel_luminosity + delta);
            }
        } else {
            return std::unexpected("Invalid luminosity type. Expected 'mid_range', 'arithmetic_mean' or 'bt709'. Received: " + luminosity_type);
        }

        return pow(2.0f, cumsum / length);
    }

    // Linear renormalization of the image luminosity using a manual value
    std::expected<void, std::string> normalize_image(float normalization, float luminosity) {
        
        if (luminosity <= 0.0f) {
            return std::unexpected("Luminosity value must be greater than zero to perform normalization. Received: " + std::to_string(luminosity));
        }

        float normalization_factor = normalization / luminosity;

        // exploit iterators
        for (auto& pixel : pixels) {
            // exploit scalar multiplication operator Color::operator*(float)
            pixel = pixel * normalization_factor;
        }

        return {};
    }

    // Linear renormalization of the image luminosity by auto-calculating average luminosity
    std::expected<void, std::string> normalize_image(float normalization, std::string luminosity_type = "bt709", float delta = 1e-10f) {

        // Collects the luminosity value via auto-calculation
        auto luminosity_res = average_luminosity(delta, luminosity_type);
        if (!luminosity_res.has_value()) {
            return std::unexpected("Failed to compute average luminosity for normalization: " + luminosity_res.error());
        }

        // Call the standard method (avoid code duplication)
        return normalize_image(normalization, luminosity_res.value());
    }

    // RP: I just put a denominator shift value as a parameter since this compress the values
    //     in the range [0, 1) but my wanted normalization could be greater

    
    /// Non-linear compression of the image luminosity with a simple function that does not require to compute
    /// the average luminosity of the image and that is applied directly on the pixel values. The function is 
    /// factor * x / (factor + x) where x is the value of each color component of each pixel. 
    /// The image is then compressed in the range [0, factor) and the compression is stronger for higher values.
    std::expected<void, std::string> clamp_image(float factor = 1.0f) {
        

        // RP: not consistent, we never usaed lambdas around the code
//        auto clamp_val = [](float x) { return x / (1.0f + x); };
//
//        for (unsigned int i = 0; i < pixels.size(); i++) {
//            pixels[i].r = clamp_val(pixels[i].r);
//            pixels[i].g = clamp_val(pixels[i].g);
//            pixels[i].b = clamp_val(pixels[i].b);
//        }

        Color denominator_shift(factor, factor, factor);

        for (auto& pixel : pixels) {
            // Exploitation of component-wise division operator Color::operator/(Color)
            (pixel /= (denominator_shift + pixel)) *= factor;
        }

        return{};
    }

    /// Applies gamma correction to the image with a simple power function that does not require to compute
    /// the average luminosity of the image and that is applied directly on the pixel values.
    std::expected<void, std::string> apply_gamma_correction(float gamma) {
        if (gamma <= 0.001f) {
            return std::unexpected("Gamma value must be greater than zero. Received: " + std::to_string(gamma));
        }

        float inverse_gamma = 1.0f / gamma;

        for (auto& pixel : pixels) {
            pixel.r = pow(pixel.r, inverse_gamma);
            pixel.g = pow(pixel.g, inverse_gamma);
            pixel.b = pow(pixel.b, inverse_gamma);
        }
        return {};
    }
    
    /// Overloeading of the apply_gamma_correction function to allow different gamma values for each color channel.
    std::expected<void, std::string> apply_gamma_correction(float gamma_r, float gamma_g, float gamma_b) {

        
        if (gamma_r <= 0.001f) {
            return std::unexpected("Gamma value for red channel must be greater than zero. Received: " + std::to_string(gamma_r));
        }
        if (gamma_g <= 0.001f) {
            return std::unexpected("Gamma value for green channel must be greater than zero. Received: " + std::to_string(gamma_g));
        }
        if (gamma_b <= 0.001f) {
            return std::unexpected("Gamma value for blue channel must be greater than zero. Received: " + std::to_string(gamma_b));
        }

        for (auto& pixel : pixels) {
            pixel.r = pow(pixel.r, 1.0f / gamma_r);
            pixel.g = pow(pixel.g, 1.0f / gamma_g);
            pixel.b = pow(pixel.b, 1.0f / gamma_b);
        }
        return {};
    }
};