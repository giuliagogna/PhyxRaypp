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

#include <tbb/global_control.h>

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

/**
 * @brief Converts a string into a floating-point value.
 *
 * Leading and trailing whitespace are ignored. Decimal commas are automatically converted
 * to decimal points before parsing, allowing inputs such as `"3,14"` as well as `"3.14"`.
 *
 * Parsing is performed using the classic C locale to ensure locale-independent behavior.
 *
 * @param str String representation of the floating-point value.
 * @return The parsed float on success, or an error message if the
 *         string does not represent a valid floating-point number.
 */
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

/**
 * @brief Parsed command-line parameters used to configure program execution.
 *
 * This structure stores all options extracted from the CLI, including
 * input/output files, rendering settings, image dimensions, and
 * path-tracing configuration parameters.
 *
 * Members are initialized with the same default values used when the
 * corresponding command-line option is omitted.
 */
struct Parameters {

public:
    /// Selected command/subprogram (e.g. "render", "pfm2png", "demo").
    std::string command = "";

    // -----------------------------------------------------------------
    // Shared parameters
    // -----------------------------------------------------------------

    /// Input PFM image file.
    std::string input_pfm_file_name = "";

    /// Input scene description file.
    std::string input_scene_file_name = "";

    /// Tone-mapping alpha parameter.
    float alpha = 0.2f;

    /// Gamma-correction exponent.
    float gamma = 1.0f;

    /// Rendering algorithm identifier.
    std::string algorithm = "flat";

    /// Anti-aliasing mode identifier.
    std::string antialiasing = "no_antialiasing";
    int antialiasing_level = 0;

    /// Parallelization activation flag (e.g., "parallel" or "no_parallel").
    std::string parallel = "no_parallel";

    /// Number of threads requested for parallel rendering.
    /// If set to 0 or left uninitialized, the program defaults to hardware capabilities or single-threaded mode.
    int number_of_threads = 0;

    /// Output PNG image file.
    std::string output_png_file_name = "";

    /// Output image dimensions in pixels: {width, height}.
    std::pair<int, int> image_dimension{800, 800};

    // -----------------------------------------------------------------
    // Path tracer parameters
    // -----------------------------------------------------------------

    /// Number of rays generated per scattering event.
    int pathtracer_num_of_rays = 4;

    /// Maximum recursion depth.
    int pathtracer_max_depth = 4;

    /// Russian roulette activation depth.
    int pathtracer_rr_depth = 2;


    /**
     * @brief Parse command-line arguments and populate a Parameters structure.
     *
     * Supported commands:
     * - pfm2png
     * - render
     *
     * Supported optional flags:
     * - --output
     * - --parallel
     * - --algorithm
     * - --antialiasing
     * - --dimensions
     * - --pathtracer_params
     *
     * @param args Command-line arguments.
     * @return Parsed Parameters object or an error description.
     */
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
                "  --parallel <N>                  Use TBB parallelization on N threads to render the image\n"
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

                } else if (std::string_view(args[i]) == "--parallel" && i + 1 < args.size()) {
                    // ---------------------------------------------------------
                    // FLAG: --parallel <N>
                    // ---------------------------------------------------------
                    parallel = "parallel";
                    auto parse_number_of_threads = parse_float(args[i + 1]);
                    if(!parse_number_of_threads) return std::unexpected(parse_number_of_threads.error());
                    number_of_threads = parse_number_of_threads.value() < 1 ? 1 : parse_number_of_threads.value();

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

                    // Increment i to skip the values that have just been processed
                    i += 2;

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

                    // Increment i to skip the values that have just been processed
                    i += 3;
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
    /**
     * @brief Generate the output image filename from the current render settings.
     *
     * The generated name preserves the directory of @p base_path and encodes relevant rendering
     * parameters into the filename. Depending on the active command and rendering algorithm,
     * information such as alpha, gamma, image dimensions, anti-aliasing settings, and path-tracer parameters
     * may be appended.
     *
     * If @p base_path does not contain an extension, ".png" is automatically added.
     *
     * Example:
     * @code
     * output.png
     *   -> output_a0.2_g1.0_flat_800x800.png
     *
     * render.png
     *   -> render_a0.2_g1.0_pathtracing_800x800_AA4_r4_d4_rr3.png
     * @endcode
     *
     * @param base_path User-provided output path.
     * @return Fully qualified output filename with embedded render settings.
     */
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

/**
 * @brief Execute the pfm2png subcommand.
 *
 * Loads an HDR image from a PFM file, applies the standard post-processing pipeline
 * (normalization, clamping, and gamma correction), and writes the result as a low-dynamic-range PNG image.
 *
 * Any processing error is reported to the console and terminates the
 * operation early.
 *
 * @param params Parsed command-line parameters.
 */
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

/**
 * @brief Execute the render subcommand.
 *
 * Loads and parses a scene description file, constructs the selected renderer,
 * generates the image, applies the standard post-processing pipeline, and writes the final PNG image to disk.
 *
 * The rendering algorithm, anti-aliasing settings, image dimensions, and path-tracing parameters
 * are taken from @p params.
 *
 * Scene parsing errors, rendering failures, and image-processing errors
 * are reported to the console and terminate the operation early.
 *
 * @param params Parsed command-line parameters.
 */
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
    if (params.antialiasing == "apply_AA" && params.parallel == "parallel") {
        tracer.fire_all_rays_parallel( [&renderer](const Ray& ray) { return (*renderer)(ray); }, pcg, params.antialiasing_level);
    }
    else if (params.antialiasing == "no_antialiasing" && params.parallel == "parallel") {
        tracer.fire_all_rays_parallel( [&renderer](const Ray& ray) { return (*renderer)(ray); });
    }
    else if (params.antialiasing == "apply_AA" && params.parallel == "no_parallel") {
        tracer.fire_all_rays( [&renderer](const Ray& ray) { return (*renderer)(ray); }, pcg, params.antialiasing_level);
    }
    else if (params.antialiasing == "no_antialiasing" && params.parallel == "no_parallel") {
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

/**
 * @brief Program entry point.
 *
 * Parses command-line arguments and dispatches execution to one of the
 * supported subcommands:
 *
 *   - pfm2png : converts HDR PFM images to LDR PNG images
 *   - render  : parses a scene file and renders an image
 *
 * This function also initializes Intel TBB's global control to safely restrict
 * the maximum number of concurrent threads based on hardware capabilities and
 * user configuration before passing execution to the renderer.
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument array.
 *
 * @return EXIT_SUCCESS on success, non-zero on error.
 */
int main(int argc, char* argv[]) {

    Parameters parameters;   

    std::span<char*> args(argv, argc);

    auto parse_res = parameters.parse_command_line(args);
    if (!parse_res.has_value()) {
        std::println("{}", parse_res.error());
        return 1;
    }

    // TBB parallelization: asking a safe number of threads to the machine to claim during the job.
    unsigned int max_threads = std::thread::hardware_concurrency(); // Verify the number of concurrent threads supported by the hardware
    unsigned int safe_threads = std::max(1u, parameters.number_of_threads > max_threads ? max_threads : parameters.number_of_threads);
    std::println("Using {} threads for parallelization.", safe_threads);
    tbb::global_control thread_limit( // claims the specified number of threads for the parallel regions in the program, preventing oversubscription and ensuring efficient resource utilization
        tbb::global_control::max_allowed_parallelism, 
        safe_threads
    );

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