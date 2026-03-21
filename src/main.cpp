import std;
import HDRImage;
import Color;

using namespace std;

int main() {
    std::println("==================================================");
    std::println(" PhyxRadpp Ray Tracer - I/O & Engine Showcase");
    std::println("==================================================\n");

    // ==========================================================
    // PART 1: READ ERRORS TESTING
    // ==========================================================
    std::println("--- READING: ERROR SIMULATION ---");

    std::println("\n>>> Attempt 1: Non-existent file...");
    auto img_res_missing = HDRImage::read_pfm_file("images/ghost.pfm");
    if (!img_res_missing.has_value()) {
        std::println("  [ERROR CAUGHT] {}", img_res_missing.error().message);
    }

    std::println("\n>>> Attempt 2: File with wrong dimensions...");
    auto img_res_wrong_dim = HDRImage::read_pfm_file("images/wrong_dimensions.pfm");
    if (!img_res_wrong_dim.has_value()) {
        std::println("  [ERROR CAUGHT] {}", img_res_wrong_dim.error().message);
    }

    std::println("\n>>> Attempt 3: Loading a valid reference image...");
    auto img_res_valid = HDRImage::read_pfm_file("images/reference_le.pfm"); // Make sure this file exists!
    if (!img_res_valid.has_value()) {
        std::println("  [FATAL ERROR] Impossible to load the base image: {}", img_res_valid.error().message);
        return 1; // Exit here because we need the image to proceed
    }

    // Extract the valid image
    auto& img = img_res_valid.value();
    std::println("  [SUCCESS] Image loaded! Dimensions: {}x{}", img.width, img.height);


    // ==========================================================
    // PART 2: WRITE ERRORS TESTING
    // ==========================================================
    std::println("\n\n--- WRITING: ERROR SIMULATION ---");

    std::println("\n>>> Attempt 4: Writing to a non-existent directory...");
    auto write_res_bad = img.write_pfm_file("fake_folder_123/output.pfm", HDRImage::Endianness::little_endian);
    if (!write_res_bad.has_value()) {
        std::println("  [ERROR CAUGHT] {}", write_res_bad.error());
    }

    std::println("\n>>> Attempt 5: Valid writing to disk...");
    auto write_res_good = img.write_pfm_file("images/test_showcase_out.pfm", HDRImage::Endianness::little_endian);
    if (write_res_good.has_value()) {
        std::println("  [SUCCESS] Image successfully saved to disk!");
    } else {
        std::println("  [UNEXPECTED ERROR] {}", write_res_good.error());
    }


    // ==========================================================
    // PART 3: COLOR ALGEBRA
    // ==========================================================
    std::println("\n\n--- COLOR ALGEBRA AND FORMATTING ---");

    // Extracting colors
    Color p1 = img.get_pixel(0, 0); // Bottom-left
    Color p2 = img.get_pixel(1, 0); // Bottom-middle
    Color p3 = img.get_pixel(0, 1); // Top-left

    // Showcasing std::formatter
    std::println("\n>>> Formatting via std::formatter<Color>");
    std::println("  Pixel (0,0) default : {}", p1);
    std::println("  Pixel (0,0) rounded : {:.1f}", p1);
    std::println("  Pixel (1,0) padded  : {:06.1f}", p2);

    // Showcasing Color Algebra
    std::println("\n>>> Mathematical Operations");
    Color sum = p1 + p2;
    std::println("  Addition       (p1 + p2)   : {:.1f}", sum);

    Color diff = p2 - p1;
    std::println("  Subtraction    (p2 - p1)   : {:.1f}", diff);

    Color scaled = p1 * 2.5f;
    std::println("  Scalar Mult    (p1 * 2.5)  : {:.1f}", scaled);

    Color product = p1 * p3;
    std::println("  Color Product  (p1 * p3)   : {:.1f}", product);

    std::println("\n==================================================");
    std::println(" All systems nominal. Ready for the next phase!");
    std::println("==================================================");

    return 0;
}