import std;
import HDRImage;
import Geometry;
import Color;
import auxiliary_functions;
import Camera;
import Shape;

struct Parameters {
    std::string command = ""; // "pfm2png" o "demo"

    // Shared parameters
    std::string input_pfm_file_name = "";
    float factor = 0.2f;
    float gamma = 1.0f;
    std::string output_png_file_name = "";

    [[nodiscard]] std::expected<void, std::string> parse_command_line(int argc, char* argv[]) {
        std::string program_name = std::filesystem::path(argv[0]).filename().string();

        if (argc < 2) {
            return std::unexpected(std::format(
                "Error: No command passed.\n"
                "Available commands: pfm2png, demo\n\n"
                "Usage:\n"
                "  ./{} pfm2png <INPUT_PFM> <ALPHA_FACTOR> <GAMMA> <OUTPUT_PNG>\n"
                "  ./{} demo <ALPHA_FACTOR> <GAMMA> <OUTPUT_PNG>",
                program_name, program_name
            ));
        }

        command = argv[1];

        // ==========================================
        // PARSING PFM2PNG
        // ==========================================
        if (command == "pfm2png") {
            if (argc != 6) { // ./prog pfm2png input alpha gamma output
                return std::unexpected("Error: Wrong number of parameters for 'pfm2png'. Expected 4 arguments.");
            }
            input_pfm_file_name = argv[2];
            try {
                factor = std::stof(argv[3]);
                gamma = std::stof(argv[4]);
            } catch (...) {
                return std::unexpected("Format error: ALPHA_FACTOR and GAMMA must be numbers.");
            }
            output_png_file_name = process_output_filename(argv[5], factor, gamma);
            return {};
        }

        // ==========================================
        // PARSING DEMO
        // ==========================================
        else if (command == "demo") {
            if (argc != 5) { // ./prog demo alpha gamma output
                return std::unexpected("Error: Wrong number of parameters for 'demo'. Expected 3 arguments.");
            }
            try {
                factor = std::stof(argv[2]);
                gamma = std::stof(argv[3]);
            } catch (...) {
                return std::unexpected("Format error: ALPHA_FACTOR and GAMMA must be numbers.");
            }
            output_png_file_name = process_output_filename(argv[4], factor, gamma);
            return {};
        }

        return std::unexpected(std::format("Error: Unknown command '{}'.", command));
    }

private:
    // Helper function to format output file name
    std::string process_output_filename(const std::string& base_path, float f, float g) {
        std::filesystem::path base(base_path);
        std::filesystem::path parent_dir = base.parent_path();
        std::string stem = base.stem().string();
        std::string ext = base.extension().string();
        if (ext.empty()) {
            ext = ".png";
            std::println("Added default extension .png");
        }
        std::string new_filename = std::format("{}_alpha{}_gamma{}", stem, f, g);
        return (parent_dir / (new_filename + ext)).string();
    }
};

// ====================================
// EXECUTION FUNCTIONS (SUBCOMMANDS)
// ====================================

void run_pfm2png(const Parameters& params) {
    auto img_res = HDRImage::read_pfm_file(params.input_pfm_file_name);
    if (!img_res.has_value()) {
        std::cerr << "Error reading image: " << img_res.error().message << "\n";
        return;
    }
    HDRImage img = std::move(img_res.value());
    std::cout << std::format("File \"{}\" read from disk.\n", params.input_pfm_file_name);

    img.normalize_image(params.factor);
    img.clamp_image();
    img.apply_gamma_correction(params.gamma);
    // TODO: check this later in HDRImage
    img.write_ldr_image(params.output_png_file_name);

    std::cout << std::format("File \"{}\" correctly writen on disk.\n", params.output_png_file_name);
}

void run_demo(const Parameters& params) {
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

    tracer.frame.normalize_image(params.factor);
    tracer.frame.clamp_image();
    tracer.frame.apply_gamma_correction(params.gamma);
    // TODO: check this later in HDRImage
    tracer.frame.write_ldr_image(params.output_png_file_name);

    std::println("Demo image \"{}\" correctly writen on disk.\n", params.output_png_file_name);
}

// ====================================
// MAIN FUNCTION
// ====================================

int main(int argc, char* argv[]) {
    Parameters parameters;

    auto parse_res = parameters.parse_command_line(argc, argv);
    if (!parse_res.has_value()) {
        std::cerr << parse_res.error() << "\n";
        return 0;
    }

    if (parameters.command == "pfm2png") {
        run_pfm2png(parameters);
    } else if (parameters.command == "demo") {
        run_demo(parameters);
    }

    return 0;
}