module;

export module auxiliary_functions;
import std;


export namespace aux {

    // Checks if two numerical values are close
    bool are_close(float x, float y, float tolerance=1e-5f) {
        return std::abs(x - y) <= tolerance;
        
    }

    // =========================================================
    // Managing of input/output file opening
    // =========================================================

    // No need to worry about closing streams: ifstream and ofstream close the stream automatically
    // just before they get out of scope wherever they are

    // Opens an input file and checks for anomalies
    std::expected<std::ifstream, std::string> open_input_file(const std::string& filename) {
        // Opens the input file in binary mode: the data in the file will be read as raw bytes
        std::ifstream stream(filename, std::ios::binary);

        if (!stream.is_open()) {
            return std::unexpected(std::format("Error in opening input file '{}'", filename));
        }

        return stream;
    }

    // Opens an output file and checks for anomalies
    std::expected<std::ofstream, std::string> open_output_file(const std::string& filename) {
        // Opens the input file in binary mode: the data in the file will be read as raw bytes
        std::ofstream stream(filename, std::ios::binary);
        if (!stream.is_open()) {
            return std::unexpected(std::format("Error in opening output file '{}'", filename));
        }

        return stream;
    }
}