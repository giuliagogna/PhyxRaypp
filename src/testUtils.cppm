export module TestUtils;
import std;
import Color;
import auxiliary_functions;

using namespace aux;

export namespace test {

    // Error codes for validation
    enum class MathErrorCode {
        InvalidValue, // NaN/Inf
        WrongResult   // are_close failed
    };

    // Convert error code to string for error messages
    std::string error_to_string(MathErrorCode error) {
        switch (error) {
            case MathErrorCode::InvalidValue: return "Invalid Value";
            case MathErrorCode::WrongResult: return "Wrong Result";
            default: return "Unknown Error";
        }
    }

    // Error codes for formatting
    enum class FormatErrorCode {
        FormatFault // Formatting did not produce the expected string
    };

    // Convert format error code to string for error messages
    std::string error_to_string(FormatErrorCode error) {
        switch (error) {
            case FormatErrorCode::FormatFault: return "Format Fault";
            default: return "Unknown Format Error";
        }
    }

    // Float validation
    std::expected<void, MathErrorCode> validate(float actual, float expected) {
        if (!std::isfinite(actual)) return std::unexpected(MathErrorCode::InvalidValue);
        if (!are_close(actual, expected)) return std::unexpected(MathErrorCode::WrongResult);
        return {}; // Success (default)
    }

    // Color validation
    std::expected<void, MathErrorCode> validate(const Color& actual, const Color& expected) {
        if (!std::isfinite(actual.r) || !std::isfinite(actual.g) || !std::isfinite(actual.b)) {
            return std::unexpected(MathErrorCode::InvalidValue);
        }

        if (!are_close(actual.r, expected.r) || 
            !are_close(actual.g, expected.g) || 
            !are_close(actual.b, expected.b)) {
            return std::unexpected(MathErrorCode::WrongResult);
        }

        return {}; // Success (default)
    }

    // Formatting validation
    std::expected<void, FormatErrorCode> validate(std::string_view actual, std::string_view expected) {
        if (actual != expected) {
            return std::unexpected(FormatErrorCode::FormatFault);
        }
        return {}; // Success (default)
    }
}