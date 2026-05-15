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

using namespace std;
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
    std::string command = ""; // "pfm2png", "demo", "demo_antialiasing"

    // Shared parameters
    std::string input_pfm_file_name = "";
    float alpha = 0.2f;
    float gamma = 1.0f;
    std::string algorithm = "flat";
    std::string output_png_file_name = "";

    [[nodiscard]] std::expected<void, std::string> parse_command_line(std::span<char*> args) {
        std::string program_name = std::filesystem::path(args[0]).filename().string();

        if (args.size() < 2) {
            return std::unexpected(std::format(
                "Error: No command passed.\n"
                "Available commands: pfm2png, demo, demo_antialiasing\n\n"
                "Usage:\n"
                "  xmake run {} pfm2png <INPUT_PFM> <ALPHA_FACTOR> <GAMMA> <OUTPUT_PNG>\n"
                "  xmake run {} demo <ALPHA_FACTOR> <GAMMA> <OUTPUT_PNG>\n"
                "  xmake run {} demo_antialiasing <ALPHA_FACTOR> <GAMMA> <OUTPUT_PNG>",
                program_name, program_name, program_name
            ));
        }

        command = args[1];

        // Search for the optional flag --algorithm
        for (int i = 2; i < args.size(); ++i) {
            if (std::string_view(args[i]) == "--algorithm" && i + 1 < args.size()) {
                algorithm = args[i + 1];
            }
        }

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
            if (args.size() < 5) { // [xmake run] <program_name> demo <alpha> <gamma> <output>
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

        else if (command == "demo_antialiasing") {
            if (args.size() < 5) { // [xmake run] <program_name> demo_antialiasing <alpha> <gamma> <output>
                return std::unexpected("Error: Wrong number of parameters for 'demo_antialiasing'. Expected 3 arguments.");
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

// Function to run demo
// Using external functions to build the World, so it can be quickly changed in the running function
// If you want to build a different scene write another function

/// Builds a scene with 10 sphere: 8 on the vertexes of a cube and two in the center of the left and bottom face
World build_10_white_spheres_world() {
    World world;
    Transformation scale_01 = Scale(Vec{0.1f, 0.1f, 0.1f});

    // Helper lambda to quickly generate a solid color material
    auto make_solid_mat = [](Color c) {
        auto pigment = std::make_shared<UniformPigment>(c);
        auto brdf = std::make_shared<DiffusiveBRDF>(pigment);
        return std::make_shared<Material>(brdf);
    };

    Color c = Color{0.0f, 0.0f, 0.0f};

    // 8 Spheres on the vertices of a cube, each with a different color
    world.add(std::make_unique<Sphere>(Trans(Vec{ 0.5f,  0.5f,  0.5f}) * scale_01, make_solid_mat(c)));
    world.add(std::make_unique<Sphere>(Trans(Vec{ 0.5f,  0.5f, -0.5f}) * scale_01, make_solid_mat(c)));
    world.add(std::make_unique<Sphere>(Trans(Vec{ 0.5f, -0.5f,  0.5f}) * scale_01, make_solid_mat(c)));
    world.add(std::make_unique<Sphere>(Trans(Vec{ 0.5f, -0.5f, -0.5f}) * scale_01, make_solid_mat(c)));
    world.add(std::make_unique<Sphere>(Trans(Vec{-0.5f,  0.5f,  0.5f}) * scale_01, make_solid_mat(c)));
    world.add(std::make_unique<Sphere>(Trans(Vec{-0.5f,  0.5f, -0.5f}) * scale_01, make_solid_mat(c)));
    world.add(std::make_unique<Sphere>(Trans(Vec{-0.5f, -0.5f,  0.5f}) * scale_01, make_solid_mat(c)));
    world.add(std::make_unique<Sphere>(Trans(Vec{-0.5f, -0.5f, -0.5f}) * scale_01, make_solid_mat(c)));

    // 2 Spheres on the left and bottom faces
    world.add(std::make_unique<Sphere>(Trans(Vec{ 0.0f,  0.5f,  0.0f}) * scale_01, make_solid_mat(c)));
    world.add(std::make_unique<Sphere>(Trans(Vec{ 0.0f,  0.0f, -0.5f}) * scale_01, make_solid_mat(c)));

    return world;
}

/// Builds a scene with a plane on the bottom half of the scene
World build_plane_world() {
    World world;

    // Build a red and green checkerboard material directly inside the function
    auto checkered_pigment = std::make_shared<CheckeredPigment>(Color{1.0f, 0.0f, 0.0f}, Color{0.0f, 1.0f, 0.0f}, 4);
    auto brdf = std::make_shared<DiffusiveBRDF>(checkered_pigment);
    auto plane_material = std::make_shared<Material>(brdf);

    // Attach it to the rotated plane
    world.add(std::make_unique<Plane>(
        // Plane is rotated 90° to have images in the right orientation
        Trans(Vec{0.0f, 0.0f, -1.0f}) * R_z(std::numbers::pi_v<float> / 2.0f),
        plane_material
    ));

    return world;
}

World build_plane_and_sphere_world() {
    World world;

    // Build the Plane (Floor)
    auto plane_pigment = std::make_shared<CheckeredPigment>(Color{0.8f, 0.0f, 0.0f}, Color{0.0f, 0.8f, 0.0f}, 4);
    auto plane_brdf = std::make_shared<DiffusiveBRDF>(plane_pigment);
    auto plane_material = std::make_shared<Material>(plane_brdf);

    world.add(std::make_unique<Plane>(
        Trans(Vec{0.0f, 0.0f, -1.0f}) * R_z(std::numbers::pi_v<float> / 2.0f),
        plane_material
    ));

    // Build the Sphere (Hovering above the floor)
    std::shared_ptr<Material> sphere_material;
    std::string pfm_path = "images/memorial.pfm";
    auto img_res = HDRImage::read_pfm_file(pfm_path);

    if (img_res.has_value()) {
        // If the image loads successfully, create the ImagePigment
        auto image_pigment = std::make_shared<ImagePigment>(std::move(img_res.value()));
        auto sphere_brdf = std::make_shared<DiffusiveBRDF>(image_pigment);
        sphere_material = std::make_shared<Material>(sphere_brdf);
    } else {
        // Safety Fallback: If the image is missing, make the sphere solid Blue
        std::println("Warning: Could not load '{}'. Using blue fallback.", pfm_path);
        auto fallback_pigment = std::make_shared<UniformPigment>(Color{0.0f, 0.0f, 1.0f});
        auto fallback_brdf = std::make_shared<DiffusiveBRDF>(fallback_pigment);
        sphere_material = std::make_shared<Material>(fallback_brdf);
    };

    // Sphere is of ray 1, so keeping it on the origin should do the job
    world.add(std::make_unique<Sphere>(Scale(Vec{0.3f, 0.3f, 0.3f}), sphere_material));

    return world;
}

// When cube is merged uncomment
// World build_cube_world(){}

void run_demo(const Parameters& params) {

    // =============================================================
    // Change the function you call here to build another world
    //World world = build_10_white_spheres_world();
    World world = build_plane_world();
    // =============================================================

    PerspectiveCamera camera(1.0f, 3.0f, Transformation{});
    //OrthogonalCamera camera(1.0f, R_z(std::numbers::pi_v<float>/3.0f));
    HDRImage frame(800, 800);
    ImageTracer tracer(frame, camera);

    // =============================================================
    // Modify this color to have a different background color
    Color sky_color{0.5f, 0.7f, 1.0f};
    //Color sky_color(1.0f, 1.0f, 1.0f);
    //Color sky_color = Color{0.0f, 0.0f, 0.0f};
    // =============================================================

    std::unique_ptr<Renderer> renderer;

    if (params.algorithm == "onoff") {
        // By passing ONLY &world OnOffRenderer constructor automatically fills in your default Black background and White hit color
        // If you ever want to change it, you just add the colors back:
        // renderer = std::make_unique<OnOffRenderer>(&world, Color{1,0,0}, Color{0,1,0});
        renderer = std::make_unique<OnOffRenderer>(&world);
    } else if (params.algorithm == "flat") {
        renderer = std::make_unique<FlatRenderer>(&world, sky_color);
    } else {
        std::println("Warning: Unknown algorithm '{}'. Defaulting to flat.", params.algorithm);
        renderer = std::make_unique<FlatRenderer>(&world, sky_color);
    }

    std::println("Rendering demo scene using '{}' algorithm...", params.algorithm);
    tracer.fire_all_rays( [&renderer](const Ray& ray) { return (*renderer)(ray); });

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

void run_demo_antialiasing(const Parameters& params) {

    // Create RNG object;
    PCG pcg;

    // =============================================================
    // Change the function you call here to build another world
    //World world = build_10_white_spheres_world();
    World world = build_plane_world();
    // =============================================================

    PerspectiveCamera camera(1.0f, 3.0f, Transformation{});
    //OrthogonalCamera camera(1.0f, R_z(std::numbers::pi_v<float>/3.0f));
    HDRImage frame(3200, 3200);
    ImageTracer tracer(frame, camera);

    // =============================================================
    // Modify this color to have a different background color
    Color sky_color{0.5f, 0.7f, 1.0f};
    //Color sky_color(1.0f, 1.0f, 1.0f);
    //Color sky_color = Color{0.0f, 0.0f, 0.0f};
    // =============================================================

    std::unique_ptr<Renderer> renderer;

    if (params.algorithm == "onoff") {
        // By passing ONLY &world OnOffRenderer constructor automatically fills in your default Black background and White hit color
        // If you ever want to change it, you just add the colors back:
        // renderer = std::make_unique<OnOffRenderer>(&world, Color{1,0,0}, Color{0,1,0});
        renderer = std::make_unique<OnOffRenderer>(&world);
    } else if (params.algorithm == "flat") {
        renderer = std::make_unique<FlatRenderer>(&world, sky_color);
    } else {
        std::println("Warning: Unknown algorithm '{}'. Defaulting to flat.", params.algorithm);
        renderer = std::make_unique<FlatRenderer>(&world, sky_color);
    }

    std::println("Rendering demo scene using '{}' algorithm...", params.algorithm);
    tracer.fire_all_rays( [&renderer](const Ray& ray) { return (*renderer)(ray); }, pcg, 10);

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
    } else if (parameters.command == "demo_antialiasing") {
        run_demo_antialiasing(parameters);
    } else {
        std::println("Error: Unknown command '{}'.", parameters.command);
        return 1;
    }

    return 0;
}