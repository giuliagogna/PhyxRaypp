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


// Helper function to parse floats
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

struct Parameters {

public:
    std::string command = ""; // "pfm2png" o "demo"

    // Shared parameters
    std::string input_pfm_file_name = "";
    float alpha = 0.2f;
    float gamma = 1.0f;
    std::string output_png_file_name = "";

    [[nodiscard]] std::expected<void, std::string> parse_command_line(std::span<char*> args) {
        std::string program_name = std::filesystem::path(args[0]).filename().string();

        if (args.size() < 2) {
            return std::unexpected(std::format(
                "Error: No command passed.\n"
                "Available commands: pfm2png, demo\n\n"
                "Usage:\n"
                "  xmake run {} pfm2png <INPUT_PFM> <ALPHA_FACTOR> <GAMMA> <OUTPUT_PNG>\n"
                "  xmake run {} demo <ALPHA_FACTOR> <GAMMA> <OUTPUT_PNG>",
                program_name, program_name
            ));
        }

        command = args[1];

        // ==========================================
        // PARSING PFM2PNG
        // ==========================================
        if (command == "pfm2png") {
            if (args.size() != 6) { // [xmake run] <program_name> pfm2png <input> <alpha> <gamma> <output>
                return std::unexpected("Error: Wrong number of parameters for 'pfm2png'. Expected 4 arguments.");
            }
            input_pfm_file_name = args[2];

            auto alpha_res = parse_float(args[3]);
            auto gamma_res = parse_float(args[4]);

            if (!alpha_res) return std::unexpected(alpha_res.error());
            if (!gamma_res) return std::unexpected(gamma_res.error());

            alpha = alpha_res.value();
            gamma = gamma_res.value();
            output_png_file_name = process_output_filename(args[5], alpha, gamma);

            return {};
        }

        // ==========================================
        // PARSING DEMO
        // ==========================================
        else if (command == "demo") {
            if (args.size() != 5) { // [xmake run] <program_name> demo <alpha> <gamma> <output>
                return std::unexpected("Error: Wrong number of parameters for 'demo'. Expected 3 arguments.");
            }

            auto alpha_res = parse_float(args[2]);
            auto gamma_res = parse_float(args[3]);

            if (!alpha_res) return std::unexpected(alpha_res.error());
            if (!gamma_res) return std::unexpected(gamma_res.error());

            alpha = alpha_res.value();
            gamma = gamma_res.value();
            output_png_file_name = process_output_filename(args[4], alpha, gamma);

            return {};
        }

        return std::unexpected(std::format("Error: Unknown command '{}'.", command));
    }

private:
    // Helper function to format output file name
    std::string process_output_filename(std::string_view base_path, float a, float g) {
        std::filesystem::path path(base_path);

        std::string ext = path.extension().string();
        if (ext.empty()) {
            ext = ".png";
            std::println("Added default extension .png");
        }

        std::string new_filename = std::format("{}_alpha{}_gamma{}{}", path.stem().string(), a, g, ext);
        path.replace_filename(new_filename);

        return path.string();
    }
};

// ====================================
// EXECUTION FUNCTIONS (SUBCOMMANDS)
// ====================================

// Function to run pfm2png
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

// Function tu run demo
// Using external functions to build the World, so it can be quickly changed in the running function
// If you want to build a different scene write another function

/// Builds a scene with 10 sphere: 8 on the vertexes of a cube and two in the center of the left and bottom face
World build_sphere_world() {

    World world;

    Transformation scale_01 = Scale(Vec{0.1f, 0.1f, 0.1f});
    world.add(std::make_unique<Sphere>(Trans(Vec{ 0.5f,  0.5f,  0.5f}) * scale_01));
    world.add(std::make_unique<Sphere>(Trans(Vec{ 0.5f,  0.5f, -0.5f}) * scale_01));
    world.add(std::make_unique<Sphere>(Trans(Vec{ 0.5f, -0.5f,  0.5f}) * scale_01));
    world.add(std::make_unique<Sphere>(Trans(Vec{ 0.5f, -0.5f, -0.5f}) * scale_01));
    world.add(std::make_unique<Sphere>(Trans(Vec{-0.5f,  0.5f,  0.5f}) * scale_01));
    world.add(std::make_unique<Sphere>(Trans(Vec{-0.5f,  0.5f, -0.5f}) * scale_01));
    world.add(std::make_unique<Sphere>(Trans(Vec{-0.5f, -0.5f,  0.5f}) * scale_01));
    world.add(std::make_unique<Sphere>(Trans(Vec{-0.5f, -0.5f, -0.5f}) * scale_01));
    world.add(std::make_unique<Sphere>(Trans(Vec{ 0.0f,  0.5f,  0.0f}) * scale_01));
    world.add(std::make_unique<Sphere>(Trans(Vec{ 0.0f,  0.0f, -0.5f}) * scale_01));

    return world;
}

/// Builds a scene with a plane on the bottom half of the scene
World build_plane_world() {
    World world;
    world.add(std::make_unique<Plane>(Trans(Vec{0.0f, 0.0f, -1.0f}) * R_z(std::numbers::pi_v<float> / 2.0f)));
    return world;
}

// When cube is merged uncomment
// World build_cube_world(){}

void run_demo(const Parameters& params) {

    // Change the function you call here to build another world
    World world = build_sphere_world();

    PerspectiveCamera camera(1.0f, 3.0f, Transformation{});
    HDRImage frame(800, 800);
    ImageTracer tracer(frame, camera);

    std::function<Color(const Ray&)> ray_tracing_func = [&world](const Ray& ray) {
        auto hit = world.ray_intersection(ray);
        if (hit.has_value()) {
            return Color{1.0f, 1.0f, 1.0f};
        } else {
            return Color{0.0f, 0.0f, 0.0f};
        }
    };

    std::println("Rendering demo scene...");
    tracer.fire_all_rays(ray_tracing_func);

    auto process_result = tracer.frame.normalize_image(params.alpha)
        .and_then([&]() { return tracer.frame.clamp_image(); })
        .and_then([&]() { return tracer.frame.apply_gamma_correction(params.gamma); })
        .and_then([&]() { return tracer.frame.write_ldr_image(params.output_png_file_name); });

    if (!process_result.has_value()) {
        std::println("Error during image processing: {}", process_result.error());
        return;
    }

    std::println("Demo image \"{}\" correctly writen on disk.\n", params.output_png_file_name);
}

// ====================================
// MAIN FUNCTION
// ====================================

int main(int argc, char* argv[]) {
    Parameters parameters;

    // std::span gives us a safe, modern view over the C-style array
    std::span<char*> args(argv, argc);

    auto parse_res = parameters.parse_command_line(args);
    if (!parse_res.has_value()) {
        std::println("{}", parse_res.error());
        return 1; // Return non-zero for error execution
    }

    if (parameters.command == "pfm2png") {
        run_pfm2png(parameters);
    } else if (parameters.command == "demo") {
        run_demo(parameters);
    }

    return 0;
}