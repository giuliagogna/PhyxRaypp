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

/**
 * @file HDRImage.cppm
 * @brief High Dynamic Range image representation and file I/O.
 *
 * This module provides:
 * - Storage for HDR images as floating-point RGB pixels.
 * - Reading and writing of Portable Float Map (PFM) files.
 * - Tone-mapping utilities such as normalization, clamping, and gamma correction.
 */

module;
#include <cassert>
#include <fstream>
#include <cmath>
#include <stb/stb_image_write.h>

export module HDRImage;

import std;
import Color;
import auxiliary_functions; // contains open_input_file used in read_pfm_image

/**
 * @brief Error returned when a PFM file violates the expected format.
 */
export struct InvalidPfmFileFormat {
    std::string message;
};

/**
 * @brief High Dynamic Range image stored as floating-point RGB pixels.
 *
 * Pixels are stored in row-major order and represented using the Color type.
 * The class provides image I/O utilities, luminosity estimation, tone mapping, and gamma correction.
 */
export struct HDRImage {

    /// Byte ordering used when reading or writing PFM files.
    enum class Endianness { little_endian, big_endian };

    /// Image width in pixels.
    int width{};

    /// Image height in pixels.
    int height{};

    /// Pixel buffer stored in row-major order.
    std::vector<Color> pixels{};

    // =========================================================================
    // CONSTRUCTORS & DESTRUCTOR
    // =========================================================================

    /**
     * @brief Create an HDR image of the specified size.
     *
     * All pixels are initialized to black.
     * Constructor from file is delegated to the static function read_pfm_file that reads a pfm file and returns
     * an HDRImage with its content. This allows the safe construction with expected value.
     *
     * @param w Image width.
     * @param h Image height.
     *
     * @throws std::invalid_argument
     *         If width or height are not positive.
     */
    HDRImage(int w, int h) : width(w), height(h), pixels(w * h, Color(0.0f, 0.0f, 0.0f)) {
        if (w <= 0 || h <= 0) {
            throw std::invalid_argument("Image dimensions must be greater than zero.");
        }
    }

    // =========================================================================
    // PIXEL ACCESS & GEOMETRY
    // =========================================================================

    /**
     * @brief Check whether pixel coordinates are inside the image.
     */
    [[nodiscard]] bool _valid_coordinates(const int x, const int y) const {
        return (x >= 0 && x < width && y >= 0 && y < height);
    }

    /**
     * @brief Convert pixel coordinates into a linear buffer index.
     *
     * @pre x and y must identify a valid pixel of the image
     * (must be inside image range width - height).
     */
    [[nodiscard]] int _pixel_offset(const int x, const int y) const {
        assert(_valid_coordinates(x, y) && "Coordinates are out of bounds");
        return x + (y * width);
    }

    /**
     * @brief Get the color of a pixel.
     *
     * @param x Pixel column.
     * @param y Pixel row.
     *
     * @pre x and y must identify a valid pixel of the image
     * (must be inside image range width - height).
     */
    [[nodiscard]] Color get_pixel(const int x, const int y) const {
        assert(_valid_coordinates(x, y) && "Coordinates are out of bounds");
        return pixels[_pixel_offset(x, y)];
    }

    /**
     * @brief Set the color of a pixel.
     *
     * @param x Pixel column.
     * @param y Pixel row.
     * @param c New pixel color.
     *
     * @pre x and y must identify a valid pixel of the image
     * (must be inside image range width - height).
     */
    void set_pixel(const int x, const int y, const Color& c) {
        assert(_valid_coordinates(x, y) && "Coordinates are out of bounds");
        pixels[_pixel_offset(x, y)] = c;
    }

    // =========================================================================
    // STATIC I/O UTILITIES
    // =========================================================================

    /**
     * @brief Convert a 32-bit value from file byte order to native byte order.
     *
     * If the file endianness differs from the machine endianness, the bytes are swapped before returning the value.
     */
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

    /**
     * @brief Read a floating-point value from a binary stream.
     *
     * The value is read according to the endianness specified by the PFM file
     * and converted to the native machine representation if necessary.
     *
     * @param stream Input binary stream.
     * @param file_endianness Endianness used by the file.
     *
     * @return The decoded float value, or an InvalidPfmFileFormat error if
     *         the stream cannot provide 4 valid bytes.
     */
    static std::expected<float, InvalidPfmFileFormat> _read_float(std::istream& stream, Endianness file_endianness) {
        uint32_t raw_bytes{};

        stream.read(reinterpret_cast<char*>(&raw_bytes), sizeof(raw_bytes));

        if (stream.fail()) {
            if (stream.gcount() > 0 && stream.gcount() < 4) {
                return std::unexpected(InvalidPfmFileFormat{"Truncated file: expected 4 bytes for a float but only read " + 
                                       std::to_string(stream.gcount()) + " bytes."});
            }
            return std::unexpected(InvalidPfmFileFormat{"Reading error: unable to read 4 bytes for a float."});
        }

        raw_bytes = _swap_if_needed(raw_bytes, file_endianness);

        return std::bit_cast<float>(raw_bytes);
    }


    /**
     * @brief Read the next meaningful line from a PFM file.
     *
     * Empty lines and comment lines beginning with '#' are skipped.
     * Trailing carriage returns are removed to support different line ending conventions.
     *
     * @param stream Input stream.
     *
     * @return The next non-empty, non-comment line, or an
     *         InvalidPfmFileFormat error if no valid line can be read.
     */
    static std::expected<std::string, InvalidPfmFileFormat> _read_line(std::istream& stream) {
        std::string result;
        
        // Read till we find a line that is not a comment (starting with #) and not empty (for example an extra newline)
        while (std::getline(stream, result)) {
            // remove carriage return \r
            if (!result.empty() && result.ends_with('\r')) {
                result.pop_back();
            }

            // Skip comment lines.
            if (!result.empty() && result.starts_with('#')) {
                continue;
            }

            // Skip empty lines.
            if (result.empty()) {
                continue;
            }

            return result;
        }

        return std::unexpected(InvalidPfmFileFormat{"Impossible to read line."});
    }


    /**
     * @brief Parse the PFM scale factor and determine file endianness.
     *
     * In the PFM format, the sign of the scale factor encodes the
     * byte order of the pixel data:
     * - positive value  -> big-endian
     * - negative value  -> little-endian
     *
     * The magnitude of the scale factor is ignored.
     *
     * @param line Header line containing the scale factor.
     *
     * @return The corresponding Endianness value, or an
     *         InvalidPfmFileFormat error if the line does not
     *         contain a valid non-zero number.
     */
    static std::expected<Endianness, InvalidPfmFileFormat> _parse_endianness(const std::string& line) {
        std::istringstream stream(line);
        float value;

        // If the line that should contain the endianness contains something that is not a number, raises an error
        if (!(stream >> value)) {
            return std::unexpected(InvalidPfmFileFormat{"Invalid endianness format. Read: '" + line + "'."});
        }

        // Evaluate only the sign of the value read in the line: if positive big_endian, if negative little_endian
        if (value > 0.0f) {
            return Endianness::big_endian;
        } else if (value < 0.0f) {
            return Endianness::little_endian;
        } else {
            // If the value is zero it cannot be interpreted as an endianness
            return std::unexpected(InvalidPfmFileFormat{"Invalid endianness. Value cannot be zero. Read: '" + line + "'."});
        }
    }

    /**
     * @brief Parse image dimensions from a PFM header line.
     *
     * The line must contain exactly two positive integers: the image width and height.
     *
     * @param line Header line containing the image dimensions.
     *
     * @return A pair (width, height), or an InvalidPfmFileFormat
     *         error if the format is invalid or the dimensions
     *         are not positive.
     */
    static std::expected<std::pair<int, int>, InvalidPfmFileFormat> _parse_img_size(const std::string& line) {
        std::istringstream stream(line);
        int parsed_width{}, parsed_height{};

        if (!(stream >> parsed_width >> parsed_height)) {
            return std::unexpected(InvalidPfmFileFormat{"Cannot extract image dimensions from line. Read: " + line});
        }

        stream >> std::ws; // eliminates the remaining white spaces
        // If after all the remaining white spaces we have not found the end of the line it means that there are too
        // many values
        if (!stream.eof()) {
            return std::unexpected(InvalidPfmFileFormat{"Too many values for dimensions. Invalid format: " + line});
        }

        if (parsed_width <= 0 || parsed_height <= 0) {
            return std::unexpected(InvalidPfmFileFormat{"Image dimensions must be greater than zero. Read: " + line});
        }

        return std::make_pair(parsed_width, parsed_height);
    }

    // ----------------------------------------------------
    // READING A PFM FILE
    // ----------------------------------------------------

    /**
     * @brief Read a PFM image from a stream.
     *
     * The function validates the PFM header, parses the image
     * dimensions and endianness, and loads all floating-point
     * RGB pixel values.
     *
     * Pixel rows are read from bottom to top, following the
     * PFM file format convention.
     *
     * @param stream Input stream containing the PFM data.
     *
     * @return The decoded image, or an InvalidPfmFileFormat
     *         error if the file is malformed or incomplete.
     */
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
            return std::unexpected(InvalidPfmFileFormat{"Unexpected data after reading all pixels."});
        }

        // Return the finished image
        return img;
    }

    /**
     * @brief Read a PFM image from a file.
     *
     * Opens the specified file and forwards the stream
     * to the main PFM loading routine.
     *
     * @param filename Path of the PFM file.
     *
     * @return The decoded image, or an InvalidPfmFileFormat
     *         error if the file cannot be opened or parsed.
     */
    static std::expected<HDRImage, InvalidPfmFileFormat> read_pfm_file(const std::string& filename) {

        auto stream_res = aux::open_input_file(filename);

        if (!stream_res.has_value()) {
            return std::unexpected(InvalidPfmFileFormat{stream_res.error()});
        }

        // Pass the successfully opened stream to the main reading function.
        return read_pfm_file(stream_res.value());
    }


    // ----------------------------–--------------------------
    // WRITING TO STREAM
    // ----------------------------–--------------------------

    /**
     * @brief Write a floating-point value to a binary stream.
     *
     * The float is serialized according to the specified file endianness.
     * If the target endianness differs from the native platform
     * endianness, the underlying 32-bit representation is byte-swapped
     * before being written.
     *
     * @param stream Output binary stream.
     * @param value Floating-point value to write.
     * @param file_endianness Endianness to use in the output file.
     */
    static void _write_float(std::ostream& stream, float value, Endianness file_endianness) {

        uint32_t raw_bytes = std::bit_cast<uint32_t>(value);
        raw_bytes = _swap_if_needed(raw_bytes, file_endianness);

        stream.write(reinterpret_cast<const char*>(&raw_bytes), sizeof(raw_bytes));

    }

    /**
     * @brief Write the image in Portable Float Map (PFM) format.
     *
     * The function writes the complete PFM file, including:
     * - the format identifier ("PF"),
     * - image dimensions,
     * - endianness specification,
     * - floating-point RGB pixel data.
     *
     * Pixel rows are written from bottom to top, following the PFM specification.
     *
     * @param stream Output stream.
     * @param file_endianness Endianness to use for floating-point values.
     *
     * @return An empty expected on success, or an error message if
     *         writing the header or pixel data fails.
     */
    [[nodiscard]] std::expected<void, std::string> write_pfm_file(std::ostream& stream, Endianness file_endianness) const {

        std::string endianness_string = (file_endianness == Endianness::little_endian) ? "-1.0" : "1.0";

        stream << "PF\n" << width << " " << height << "\n" << endianness_string << "\n";

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

        if (!stream.good()) {
            return std::unexpected(std::format("Error in writing pixel colors. Memory full or disconnected."));
        }

        return {};
    }

    /**
     * @brief Write the image to a PFM file.
     *
     * Opens the specified file and delegates the actual serialization
     * to the stream-based overload.
     *
     * @param filename Destination file path.
     * @param file_endianness Endianness to use for floating-point values.
     *
     * @return An empty expected on success, or an error message
     *         describing the failure.
     */
    [[nodiscard]] std::expected<void, std::string> write_pfm_file(const std::string& filename, Endianness file_endianness) const {

        auto stream_res = aux::open_output_file(filename);

        if (!stream_res.has_value()) {
            return std::unexpected(stream_res.error());
        }

        auto write_res = write_pfm_file(stream_res.value(), file_endianness);

        if (!write_res.has_value()) {
            return std::unexpected(std::format("Failed to write to file '{}': {}", filename, write_res.error()));
        }

        return {};
    }

    // ----------------------------–--------------------------
    // WRITING TO PNG. STB_IMAGE_WRITE BASED
    // ----------------------------–--------------------------

    /**
     * @brief Write the image as an 8-bit PNG file.
     *
     * Each color channel is converted from the floating-point range [0,1]
     * to an 8-bit integer in [0,255] and encoded using stb_image_write.
     *
     * Pixel values are expected to have already undergone any required
     * HDR-to-LDR processing (e.g. normalization, tone mapping, and gamma
     * correction). Values are clamped to [0,1] before conversion as a
     * safeguard against small numerical inaccuracies.
     *
     * @param filename Destination PNG file.
     *
     * @return An empty expected on success, or an error message if the
     *         image could not be written.
     */
    [[nodiscard]] std::expected<void, std::string> write_ldr_image(const std::string& filename) const {
        std::vector<uint8_t> image_data(width * height * 3);
        int index = 0;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Color c = get_pixel(x, y);

                // Secure clamping in case of rounding errors: if it encounters negative pixels it clamps
                // them between 0 and 1 (if negative becomes 0, if >1 becomes 1)
                float r = std::clamp(c.r, 0.0f, 1.0f);
                float g = std::clamp(c.g, 0.0f, 1.0f);
                float b = std::clamp(c.b, 0.0f, 1.0f);

                image_data[index++] = static_cast<uint8_t>(std::round(r * 255.0f));
                image_data[index++] = static_cast<uint8_t>(std::round(g * 255.0f));
                image_data[index++] = static_cast<uint8_t>(std::round(b * 255.0f));
            }
        }

        int success = stbi_write_png(filename.c_str(), width, height, 3, image_data.data(), width * 3);

        if (!success) {
            return std::unexpected(std::format("Failed to write LDR PNG image to file '{}'", filename));
        }

        return {};
    }


    // ----------------------------–--------------------------
    // LUMINOSITY CALCULATIONS
    // ----------------------------–--------------------------

    /**
     * @brief Compute the log-average luminosity of the image.
     *
     * The luminosity of each pixel is estimated using one of the
     * supported methods ("bt709", "arithmetic_mean", or "mid_range").
     *
     * The returned value is the geometric mean of all pixel luminosities:
     *
     *     L_avg = exp( (1/N) * Σ log(L_i + delta) )
     *
     * implemented using base-2 logarithms for efficiency.
     *
     * A small positive offset `delta` is added to avoid taking the
     * logarithm of zero.
     *
     * @param delta Small offset added before the logarithm.
     * @param luminosity_type Method used to convert RGB values
     *        into a scalar luminosity estimate.
     *
     * @return Log-average luminosity of the image or an error
     *         if the image is empty or contains invalid values.
     */
    [[nodiscard]] std::expected<float, std::string> average_luminosity(float delta = 1e-10, std::string luminosity_type = "bt709") const {

        float cumsum = 0.0f;
        float tolerance = 1e-6f; // tolerance for negative luminosity values that may arise from rounding errors.

        int length = pixels.size();
        if (!length) {
            return std::unexpected("Cannot compute average luminosity of an empty image.");
        }
        
        if (delta <= 0.0f) {
            return std::unexpected("Delta value must be strictly greater than zero to avoid logarithm of zero or negative numbers. Received: " + std::to_string(delta));
        }

        float current_pixel_luminosity{};

        if (luminosity_type == "mid_range") {
            for (auto& pixel : pixels) {
                current_pixel_luminosity = pixel.luminosity_mid_range();
                if (current_pixel_luminosity < -tolerance) {
                    return std::unexpected("Negative luminosity value encountered in mid_range method. Luminosity values must be non-negative. Check the pixel values in the image.");
                }
                // If pixel is negative but greater than tolerance, set it to 0.0f: does not enter here if it entered the error
                if (current_pixel_luminosity < 0.0f) current_pixel_luminosity = 0.0f;
                cumsum += log2(current_pixel_luminosity + delta);
            }
        } else if (luminosity_type == "arithmetic_mean") {
            for (auto& pixel : pixels) {
                current_pixel_luminosity = pixel.luminosity_arithmetic_mean();
                if (current_pixel_luminosity < -tolerance) {
                    return std::unexpected("Negative luminosity value encountered in arithmetic_mean method. Luminosity values must be non-negative. Check the pixel values in the image.");
                }
                // If pixel is negative but greater than tolerance, set it to 0.0f: does not enter here if it entered the error
                if (current_pixel_luminosity < 0.0f) current_pixel_luminosity = 0.0f;
                cumsum += log2(current_pixel_luminosity + delta);
            }
        } else if (luminosity_type == "bt709") {
            for (auto& pixel : pixels) {
                current_pixel_luminosity = pixel.luminosity_bt709();
                if (current_pixel_luminosity < -tolerance) {
                    return std::unexpected("Negative luminosity value encountered in bt709 method. Luminosity values must be non-negative. Check the pixel values in the image.");
                }
                // If pixel is negative but greater than tolerance, set it to 0.0f: does not enter here if it entered the error
                if (current_pixel_luminosity < 0.0f) current_pixel_luminosity = 0.0f;
                cumsum += log2(current_pixel_luminosity + delta);
            }
        } else {
            return std::unexpected("Invalid luminosity type. Expected 'mid_range', 'arithmetic_mean' or 'bt709'. Received: " + luminosity_type);
        }

        return pow(2.0f, cumsum / length);
    }

    // ----------------------------–--------------------------
    // NORMALIZATION, CLAMPING AND GAMMA CORRECTION
    // ----------------------------–--------------------------

    /**
     * @brief Normalize image intensity using a user-provided luminosity.
     *
     * Every pixel is multiplied by:
     *
     *     normalization / luminosity
     *
     * allowing external control over the reference luminosity.
     *
     * @param normalization Desired average luminosity.
     * @param luminosity Current image luminosity.
     *
     * @return Success or an error if luminosity is non-positive.
     */
    std::expected<void, std::string> normalize_image(float normalization, float luminosity) {
        
        if (luminosity <= 0.0f) {
            return std::unexpected("Luminosity value must be greater than zero to perform normalization. Received: " + std::to_string(luminosity));
        }

        float normalization_factor = normalization / luminosity;

        for (auto& pixel : pixels) {
            pixel = pixel * normalization_factor;
        }

        return {};
    }

    /**
     * @brief Normalize image intensity using the image's
     *        computed average luminosity.
     *
     * The average luminosity is estimated through average_luminosity() and the image is scaled so that
     * its average luminosity becomes `normalization`.
     *
     * @param normalization Desired average luminosity.
     * @param luminosity_type Luminosity estimator used during
     *        the automatic computation.
     * @param delta Small offset used when computing
     *              log-average luminosity.
     */
    std::expected<void, std::string> normalize_image(float normalization, std::string luminosity_type = "bt709", float delta = 1e-10f) {

        // Collects the luminosity value via auto-calculation
        auto luminosity_res = average_luminosity(delta, luminosity_type);
        if (!luminosity_res.has_value()) {
            return std::unexpected("Failed to compute average luminosity for normalization: " + luminosity_res.error());
        }

        return normalize_image(normalization, luminosity_res.value());
    }


    /**
     * @brief Apply Reinhard tone mapping to all pixels.
     *
     * Each color component is transformed according to:
     *
     *     c' = c / (c + 1)
     *
     * This operation compresses the dynamic range of the image, mapping arbitrarily large HDR values
     * into the interval [0,1) while preserving relative brightness differences.
     *
     * Unlike normalize_image(), this function does not modify the overall exposure or target luminosity
     * of the image. Its sole purpose is to compress bright values so that the image can be
     * represented on low-dynamic-range displays.
     *
     * @return Success or an error if invalid pixel values
     *         are encountered.
     */
    std::expected<void, std::string> clamp_image() {

        for (auto& pixel : pixels) {

            if (pixel.r < 0.0f || pixel.g < 0.0f || pixel.b < 0.0f) {
                return std::unexpected("Found negative pixel value.");
            }

            pixel.r = pixel.r / (pixel.r + 1.0f);
            pixel.g = pixel.g / (pixel.g + 1.0f);
            pixel.b = pixel.b / (pixel.b + 1.0f);

        }

        return{};
    }

    /**
     * @brief Apply gamma correction uniformly to all channels.
     *
     * Each component is transformed according to:
     *
     *     c' = c^(1/gamma)
     *
     * assuming pixel values are already mapped into the displayable range [0,1].
     *
     * Typical values are around gamma = 2.2.
     *
     * @param gamma Gamma exponent.
     */
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
    
    /**
     * @brief Apply independent gamma correction to each color channel.
     *
     * Each channel is transformed independently:
     *
     *     R' = R^(1/gamma_r)
     *     G' = G^(1/gamma_g)
     *     B' = B^(1/gamma_b)
     *
     * This overload is mainly intended for experimentation and color calibration workflows.
     *
     * @param gamma_r Gamma exponent for the red channel.
     * @param gamma_g Gamma exponent for the green channel.
     * @param gamma_b Gamma exponent for the blue channel.
     */
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