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
import Pigment;
import Material;
import BRDF;
import Renderer;
import PCG;
import Mesh;
import SceneFiles;

using namespace std;

/// @brief Helper function to parse floats
[[nodiscard]] std::expected<float, std::string> parse_float(std::string_view str) {
    std::string safe_str(str);

    // Strip all whitespace and hidden terminal characters (like \n or \r)
    std::erase_if(safe_str, [](unsigned char c) { return std::isspace(c); });

    // Replace any commas with dots so it is always standardized
    std::replace(safe_str.begin(), safe_str.end(), ',', '.');

    // Parse using an Input String Stream locked to standard programming formatting
    float value = 0.0f;
    std::istringstream iss(safe_str);
    iss.imbue(std::locale::classic());
    iss >> value;

    // Check
    if (iss.fail() || !iss.eof()) {
        return std::unexpected(std::format("Format error: '{}' is not a valid float number.", str));
    }

    return value;
}

/// @brief Helper class to parse the Parameters provided from CLI
struct Parameters {

public:
    std::string command = ""; // "pfm2png", "demo"

    // Shared parameters
    std::string input_pfm_file_name = "";
    std::string input_scene_file_name = "";
    float alpha = 0.2f;
    float gamma = 1.0f;
    std::string algorithm = "flat";
    std::string antialiasing = "no_antialiasing";
    int antialiasing_level = 0;
    std::string output_png_file_name = "";
    std::pair<int, int> image_dimension{800, 800};

    // See PathTracer struct for more details
    int pathtracer_num_of_rays = 4;
    int pathtracer_max_depth = 4;
    int pathtracer_rr_depth = 2;


    [[nodiscard]] std::expected<void, std::string> parse_command_line(std::span<char*> args) {
        std::string program_name = std::filesystem::path(args[0]).filename().string();

        if (args.size() < 2) {
            // Output file name is default the name of the scene file with attached all the other parameters
            return std::unexpected(std::format(
                "Error: No command passed.\n"
                "Available commands: pfm2png, render\n\n"

                "Usage:\n"
                "  xmake run {} pfm2png <INPUT_PFM> <ALPHA> <GAMMA> [FLAGS]\n"
                "  xmake run {} render  <INPUT_SCENE> <ALPHA> <GAMMA> [FLAGS]\n\n"

                "Optional Flags:\n"
                "  --output <filename>             Override automatic naming and specify exact output path\n"
                "  --algorithm <type>              Render engine: 'flat' or 'pathtracing' (default: 'flat')\n"
                "  --antialiasing <N>              Apply anti-aliasing with NxN samples per pixel\n"
                "  --dimensions <width> <height>   Set output image resolution in pixels\n"
                "  --pathtracer_params <rays> <max_depth> <rr_depth>\n"
                "                                  Configure PathTracer settings:\n"
                "                                    <rays>      : Number of rays per hit\n"
                "                                    <max_depth> : Maximum reflection depth\n"
                "                                    <rr_depth>  : Russian Roulette start depth\n",
                program_name, program_name
            ));
        }

        command = args[1];

        // ==========================================
        // PARSING PFM2PNG
        // ==========================================
        if (command == "pfm2png") {
            // It is now customary for the user to specify the output filename
            if (args.size() < 5) {
                return std::unexpected("Error: Wrong number of parameters for 'pfm2png'. Expected at least 3 arguments.");
            }

            std::string filename = std::filesystem::path(args[2]).filename().string();
            std::string stem = std::filesystem::path(args[2]).stem().string();

            // Images to convert are stored in directory pfm_files/
            input_pfm_file_name = "pfm_files/" + filename;

            auto alpha_res = parse_float(args[3]);
            auto gamma_res = parse_float(args[4]);

            if (!alpha_res) return std::unexpected(alpha_res.error());
            if (!gamma_res) return std::unexpected(gamma_res.error());

            alpha = alpha_res.value();
            gamma = gamma_res.value();

            bool custom_output = false;

            // Search for optional flags for pfm2png
            for (int i = 5; i < args.size(); ++i) {
                if (std::string_view(args[i]) == "--output" && i + 1 < args.size()) {
                    output_png_file_name = args[i + 1];
                    custom_output = true;
                }
            }

            // Only generate the automatic name if the user didn't provide one
            if (!custom_output) {
                // The result of a converted pfm file ends up in png_converted
                std::filesystem::create_directories("png_converted");
                output_png_file_name = process_output_filename("png_converted/" + stem);
            }

            return {};
        }

        // ==========================================
        // PARSING RENDER
        // ==========================================
        else if (command == "render") {
            if (args.size() < 5) { // [xmake run] <program_name> render <input> <alpha> <gamma> <flags>
                return std::unexpected("Error: Wrong number of parameters for 'render'. Expected at least 3 arguments.");
            }

            std::string filename = std::filesystem::path(args[2]).filename().string();
            std::string stem = std::filesystem::path(args[2]).stem().string();

            // Scene files to render are stored in directory examples/
            input_scene_file_name = "examples/" + filename;

            auto alpha_res = parse_float(args[3]);
            auto gamma_res = parse_float(args[4]);

            if (!alpha_res) return std::unexpected(alpha_res.error());
            if (!gamma_res) return std::unexpected(gamma_res.error());

            alpha = alpha_res.value();
            gamma = gamma_res.value();

            bool custom_output = false;

            // =========================================================
            // OPTIONAL FLAGS PARSING
            // =========================================================
            // Iterate through the remaining arguments to find optional flags.
            // We start at index 5 because indices 0-4 are reserved for the
            // program name, command, and mandatory file/image parameters.

            for (int i = 5; i < args.size(); ++i) {

                if (std::string_view(args[i]) == "--output" && i + 1 < args.size()) {
                    // ---------------------------------------------------------
                    // FLAG: --output <custom_output_filename>
                    // ---------------------------------------------------------
                    output_png_file_name = args[i + 1];
                    custom_output = true;

                } else if (std::string_view(args[i]) == "--algorithm" && i + 1 < args.size()) {
                    // ---------------------------------------------------------
                    // FLAG: --algorithm <type>
                    // ---------------------------------------------------------
                    algorithm = args[i + 1];
                    if (algorithm != "flat" && algorithm != "pathtracing" && algorithm != "onoff") {
                        return std::unexpected(std::format("Error: Invalid algorithm '{}'. Expected 'flat' or 'pathtracing'.", algorithm));
                    }

                } else if (std::string_view(args[i]) == "--antialiasing" && i + 1 < args.size()) {
                    // ---------------------------------------------------------
                    // FLAG: --antialiasing <N>
                    // ---------------------------------------------------------
                    antialiasing = "apply_AA";
                    auto parse_antialiasing_level = parse_float(args[i + 1]);
                    if(!parse_antialiasing_level) return std::unexpected(parse_antialiasing_level.error());
                    // Clamp the value: AA level cannot be less than 1
                    antialiasing_level = parse_antialiasing_level.value() < 1 ? 1 : parse_antialiasing_level.value();

                } else if (std::string_view(args[i]) == "--dimensions" && i + 2 < args.size()) {
                    // ---------------------------------------------------------
                    // FLAG: --dimensions <width> <height>
                    // ---------------------------------------------------------v
                    auto parse_x = parse_float(args[i+1]);
                    if(!parse_x) return std::unexpected(parse_x.error());
                    auto parse_y = parse_float(args[i+2]);
                    if(!parse_y) return std::unexpected(parse_y.error());
                    image_dimension = std::make_pair(
                        parse_x.value() < 1 ? 1 : parse_x.value(),
                        parse_y.value() < 1 ? 1 : parse_y.value()
                    );

                } else if (std::string_view(args[i]) == "--pathtracer_params" && i + 3 < args.size()) {
                    // ---------------------------------------------------------
                    // FLAG: --pathtracer_params <rays> <max_depth> <rr_depth>
                    // ---------------------------------------------------------
                    // Number of rays emitted per hit
                    auto parse_num_of_rays = parse_float(args[i + 1]);
                    if(!parse_num_of_rays) return std::unexpected(parse_num_of_rays.error());
                    pathtracer_num_of_rays = parse_num_of_rays.value() < 1 ? 1 : parse_num_of_rays.value();

                    // Max number of reflection of a Camera shot ray
                    auto parse_max_depth = parse_float(args[i + 2]);
                    if(!parse_max_depth) return std::unexpected(parse_max_depth.error());
                    pathtracer_max_depth = parse_max_depth.value() < 1 ? 1 : parse_max_depth.value();

                    // Russian Roulette depth condition
                    auto parse_rr_depth = parse_float(args[i + 3]);
                    if(!parse_rr_depth) return std::unexpected(parse_rr_depth.error());
                    pathtracer_rr_depth = parse_rr_depth.value() < 1 ? 1 : parse_rr_depth.value();
                }
            }

            // Only generate the automatic name if the user didn't specify the --output flag
            if (!custom_output) {
                // Generated images with any rendering algorithm get saved in generated_images/
                std::filesystem::create_directories("generated_images");
                output_png_file_name = process_output_filename("generated_images/" + stem);
            }

            return {};
        }

        return std::unexpected(std::format("Error: Unknown command '{}'.", command));
    }

private:
    /// @brief Helper function to format output file name
    std::string process_output_filename(std::string_view base_path) {
        std::filesystem::path path(base_path);

        std::string ext = path.extension().string();
        if (ext.empty()) {
            ext = ".png";
            std::println("Added default extension .png");
        }

        // Start with the base stem, alpha, gamma, and algorithm
        std::string new_filename = std::format("{}_a{}_g{}",
            path.stem().string(), alpha, gamma);

        if (command == "render") {
            new_filename += std::format("_{}_{}x{}", algorithm, image_dimension.first, image_dimension.second);

            // Add Anti-Aliasing level if it is enabled
            if (antialiasing == "apply_AA") {
                new_filename += std::format("_AA{}", antialiasing_level);
            }

            // Add PathTracer specific parameters if that algorithm is selected
            if (algorithm == "pathtracing") {
                new_filename += std::format("_r{}_d{}_rr{}",
                    pathtracer_num_of_rays, pathtracer_max_depth, pathtracer_rr_depth);
            }
        }

        // Append the extension
        new_filename += ext;
        path.replace_filename(new_filename);

        return path.string();
    }
};

// ====================================
// EXECUTION FUNCTIONS (SUBCOMMANDS)
// ====================================

/// @brief Function to run pfm2png
void run_pfm2png(const Parameters& params) {
    auto img_res = HDRImage::read_pfm_file(params.input_pfm_file_name);
    if (!img_res.has_value()) {
        std::println("Error reading image: {}", img_res.error().message);
        return;
    }
    HDRImage img = std::move(img_res.value());
    std::println("File \"{}\" read from disk.", params.input_pfm_file_name);

    // C++23 Monadic operations: chains operations and short-circuits on the first error
    // Error messages are propagated to process_result.error()
    auto process_result = img.normalize_image(params.alpha)
        .and_then([&]() { return img.clamp_image(); })
        .and_then([&]() { return img.apply_gamma_correction(params.gamma); })
        .and_then([&]() { return img.write_ldr_image(params.output_png_file_name); });

    if (!process_result.has_value()) {
        std::println("Error during image processing: {}", process_result.error());
        return;
    }

    std::println("File \"{}\" correctly written on disk.", params.output_png_file_name);
}

/// @brief Function to run demo
void run_render(const Parameters& params) {

    // ======================================
    // PARSING OF THE SCENE
    // ======================================

    // Open the file
    std::ifstream scene_file(params.input_scene_file_name);
    if (!scene_file.is_open()) {
        std::println("Error: Could not open scene file '{}'.", params.input_scene_file_name);
        return;
    }

    // Parse the scene
    std::println("Parsing scene from file {} ...", params.input_scene_file_name);
    InputStream input_stream(scene_file, params.input_scene_file_name);
    auto scene_res = parse_scene(input_stream);

    if (!scene_res.has_value()) {
        std::println("\n[SYNTAX ERROR] in parsing -> {}:{}:{}",
                     scene_res.error().location.filename,
                     scene_res.error().location.line_num,
                     scene_res.error().location.col_num);
        std::println("-> {}", scene_res.error().message);
        return;
    }

    Scene scene = std::move(scene_res.value());

    // Validate that the user actually provided a camera
    if (!scene.camera) {
        std::println("Error: No camera was defined in the scene file!");
        return;
    }

    // ======================================
    // PREPARATION OF RENDERER
    // ======================================

    PCG pcg; //RNG object

    HDRImage frame(params.image_dimension.first, params.image_dimension.second);

    // Pass the parsed camera (dereferenced from the unique_ptr)
    ImageTracer tracer(frame, *scene.camera);

    // Creating default objects
    World world;
    std::unique_ptr<Renderer> renderer;
    Color bkg_color = scene.background_color;

    // Modify the default objects for the selected algorithm
    if (params.algorithm == "onoff") {
        renderer = std::make_unique<OnOffRenderer>(&scene.world);
    } else if (params.algorithm == "flat") {
        renderer = std::make_unique<FlatRenderer>(&scene.world, bkg_color);
    } else if (params.algorithm == "pathtracing") { // Path tracing renderer: a complex scene will be used
        renderer = std::make_unique<PathTracer>(pcg, &scene.world, bkg_color, params.pathtracer_num_of_rays, params.pathtracer_max_depth, params.pathtracer_rr_depth);
    }

    std::println("Rendering demo scene using '{}' algorithm...", params.algorithm);
    if (params.antialiasing == "apply_AA") {
        tracer.fire_all_rays( [&renderer](const Ray& ray) { return (*renderer)(ray); }, pcg, params.antialiasing_level);
    } else {
        tracer.fire_all_rays( [&renderer](const Ray& ray) { return (*renderer)(ray); });
    }

    auto process_result = tracer.frame.normalize_image(params.alpha)
        .and_then([&]() { return tracer.frame.clamp_image(); })
        .and_then([&]() { return tracer.frame.apply_gamma_correction(params.gamma); })
        .and_then([&]() { return tracer.frame.write_ldr_image(params.output_png_file_name); });

    if (!process_result.has_value()) {
        std::println("Error during image processing: {}", process_result.error());
        return;
    }

    std::println("Rendered image \"{}\" correctly writen on disk.\n", params.output_png_file_name);
}

// ====================================
// MAIN FUNCTION
// ====================================

int main(int argc, char* argv[]) {
    Parameters parameters;

    std::span<char*> args(argv, argc);

    auto parse_res = parameters.parse_command_line(args);
    if (!parse_res.has_value()) {
        std::println("{}", parse_res.error());
        return 1;
    }

    if (parameters.command == "pfm2png") {
        run_pfm2png(parameters);
    } else if (parameters.command == "render") {
        run_render(parameters);
    } else {
        std::println("Error: Unknown command '{}'.", parameters.command);
        return 1;
    }

    return 0;
}