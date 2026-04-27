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

import std;
import HDRImage;
import Geometry;
import Color;
import auxiliary_functions;
import Camera;
import Shape;


struct Parameters {
    std::string input_pfm_file_name = "";
    float factor = 0.2f;
    float gamma = 1.0f;
    std::string output_png_file_name = "";

    [[nodiscard]] std::expected<void, std::string> parse_command_line(int argc, char* argv[]) {

        // Extracts program name (es. "PhyxRadpp")
        std::string program_name = std::filesystem::path(argv[0]).filename().string();

        // Case 1: user does not pass parameters
        if (argc == 1) {
            return std::unexpected(std::format(
                "Error: No parameters passed.\n\n"
                "Correct usage:\n"
                "  ./{} <ALPHA_FACTOR> <GAMMA> <OUTPUT_PNG_FILE>\n\n"
                "Example:\n"
                "  ./{} 0.5 2.2 output.png",
                program_name, program_name
            ));
        }

        // Case 2: user passes wrong number of parameters
        if (argc != 4) {
            return std::unexpected(std::format(
                "Error: Number of parameters wrong. Expected 3, received {}).\n\n"
                "Correct usage:\n"
                "  ./{} <ALPHA_FACTOR> <GAMMA> <OUTPUT_PNG_FILE>",
                argc - 1, program_name
            ));
        }


        // Case 3: user passes text string instead of numbers
        try {
            factor = std::stof(argv[1]); // stof tries to convert string to float - 32 bit
            gamma = std::stof(argv[2]);
        } catch (const std::invalid_argument&) {
            return std::unexpected(std::format(
                "Format error: ALPHA_FACTOR ('{}') and GAMMA ('{}') need to be positive numbers (es. 0.5, 1.0).",
                argv[1], argv[2]
            ));
        } catch (const std::out_of_range&) { // out_of_range: if numbers inserted by user exceed 32-bit representation
            return std::unexpected("Values exceed float 32-bit representation.");
        }

        // Output file name

        // Path passed by user es outputs/output_memorial.png
        std::filesystem::path base_out_path(argv[3]);

        // 2. Smontiamo il percorso nei suoi componenti
        std::filesystem::path parent_dir = base_out_path.parent_path(); // es. "outputs"
        std::string stem = base_out_path.stem().string();               // es. "memorial"
        std::string ext = base_out_path.extension().string();           // es. ".png"

        // If user forgets extension, add .png default
        if (ext.empty()) {
            ext = ".png";
            std::println("Added default extension .png");
        }

        // Create new filename adding alpha_factor, gamma and extension
        std::string new_filename = std::format("{}_alpha{}_gamma{}", stem, factor, gamma);
        std::filesystem::path final_out_path = parent_dir / (new_filename + ext);

        output_png_file_name = final_out_path.string();

        return {};
    }
};


// ====================================
// MAIN FUNCTION
// ====================================

int main(int argc, char* argv[]) {
    Parameters parameters;

    // Parsing command line
    auto parse_res = parameters.parse_command_line(argc, argv);
    if (!parse_res.has_value()) {
        std::cerr << parse_res.error() << "\n";
        return 0; // return 0 to avoid unclear messages by xmake: if it returns an error it is a "success"
    }
    
    // ====================================
    // Scene generation
    // ====================================
    World world;
    // Add shapes to the world as needed
    world.add(std::make_unique<Sphere>(Point{ 0.5f,  0.5f,  0.5f}, 0.1f));
    world.add(std::make_unique<Sphere>(Point{ 0.5f,  0.5f, -0.5f}, 0.1f));
    world.add(std::make_unique<Sphere>(Point{ 0.5f, -0.5f,  0.5f}, 0.1f));
    world.add(std::make_unique<Sphere>(Point{ 0.5f, -0.5f, -0.5f}, 0.1f));
    world.add(std::make_unique<Sphere>(Point{-0.5f,  0.5f,  0.5f}, 0.1f));
    world.add(std::make_unique<Sphere>(Point{-0.5f,  0.5f, -0.5f}, 0.1f));
    world.add(std::make_unique<Sphere>(Point{-0.5f, -0.5f,  0.5f}, 0.1f));
    world.add(std::make_unique<Sphere>(Point{-0.5f, -0.5f, -0.5f}, 0.1f));
    world.add(std::make_unique<Sphere>(Point{ 0.0f,  0.5f,  0.0f}, 0.1f));
    world.add(std::make_unique<Sphere>(Point{ 0.0f,  0.0f, -0.5f}, 0.1f));

    // ====================================
    // Image tracer setup
    // ====================================

    // Camera setup
    // Nel demo.cpp
    PerspectiveCamera camera(1.0f, 3.0f, Transformation{}); // Aspect ratio 1, distance 3, translated along z by +1
    // HDRImage setup
    HDRImage frame(800, 800); // Create an empty HDR image with width and height of 800 pixels
    // Putting in a ImageTracer together
    ImageTracer tracer(frame, camera);

    // ====================================
    // Raytracig the scene
    // ====================================

    std::function<Color(const Ray&)> ray_tracing_func = [&world](const Ray& ray) {
        auto hit = world.ray_intersection(ray);
        if (hit.has_value()) {
            // For demonstration, we return a color based on the normal at the hit point
            return Color{1.0f, 1.0f, 1.0f};
        } else {
            return Color{0.0f, 0.0f, 0.0f}; // Background color
        }
    };

    tracer.fire_all_rays(ray_tracing_func);

    // ====================================
    // Post-processing and output
    // ====================================

    auto norm_res = tracer.frame.normalize_image(parameters.factor);
    if (!norm_res.has_value()) {
        std::cerr << "Error during normalization: " << norm_res.error() << "\n";
        return 0;
    }

    auto clamp_res = tracer.frame.clamp_image();
    if (!clamp_res.has_value()) {
        std::cerr << "Error during clamping: " << clamp_res.error() << "\n";
        return 0;
    }

    auto gamma_res = tracer.frame.apply_gamma_correction(parameters.gamma);
    if (!gamma_res.has_value()) {
        std::cerr << "Error during gamma correction: " << gamma_res.error() << "\n";
        return 0;
    }

    auto write_res = tracer.frame.write_ldr_image(parameters.output_png_file_name);
    if (!write_res.has_value()) {
        std::cerr << "Error during PNG image writing: " << write_res.error() << "\n";
        return 0;
    }

    std::println("File \"{}\" correctly writen on disk.\n", parameters.output_png_file_name);

    return 0;
}