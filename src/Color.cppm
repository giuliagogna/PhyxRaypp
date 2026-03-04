export module Color;
import std;

export struct Color {
    float r, g, b;

    Color(float red, float gre, float blu) : r(red), g(gre), b(blu) {
        // Add control on sign of r, g and b: they cannot be negative (right?)
    };

    // Add operations on colors (Piazza)

    // Convert the RGB triplet in a string and displays
    std::string to_string() const {
        return std::format("{} {} {}", r, g, b);
    }

};

