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


module;

// Include the C-error header here in the global fragment so the macros exist
#include <cerrno>
#include <cstdlib>
#include <cassert>
#include <unordered_map>

export module SceneFiles;

import std;
import Material;
import Shape;
import Camera;
import Color;
import Pigment;
import BRDF;
import Shape;
import Camera;
import Geometry;
import HDRImage;

/// Language symbols in a string_view object
export constexpr std::string_view SYMBOLS = "()<>[],*";

export struct SourceLocation {
    std::string filename = "";
    int line_num = 0;
    int col_num = 0;
};

// Keyword enumeration
export enum class KeywordEnum {
    NEW,            // 1
    // Shapes and BRDFs
    MATERIAL,       // 2
    PLANE,          // 3
    SPHERE,         // 4
    CUBE,           // 5
    DIFFUSE,        // 6
    SPECULAR,       // 7

    // Pigments
    UNIFORM,        // 8
    CHECKERED,      // 9
    IMAGE,          // 10

    // Transformations
    IDENTITY,       // 11
    TRANSLATION,    // 12
    ROTATION_X,     // 13
    ROTATION_Y,     // 14
    ROTATION_Z,     // 15
    SCALING,        // 16

    // Camera
    CAMERA,         // 17
    ORTHOGONAL,     // 18
    PERSPECTIVE,    // 19

    FLOAT,           // 20

    MESH             // 21
};

// Keyword dictionary for the lexer
export const std::unordered_map<std::string, KeywordEnum> KEYWORDS{
    {"new", KeywordEnum::NEW},
    {"material", KeywordEnum:: MATERIAL},
    {"plane", KeywordEnum:: PLANE},
    {"sphere", KeywordEnum::SPHERE},
    {"cube", KeywordEnum::CUBE},
    {"diffuse", KeywordEnum::DIFFUSE},
    {"specular", KeywordEnum::SPECULAR},
    {"uniform", KeywordEnum::UNIFORM},
    {"checkered", KeywordEnum::CHECKERED},
    {"image", KeywordEnum::IMAGE},
    {"identity", KeywordEnum::IDENTITY},
    {"translation", KeywordEnum::TRANSLATION},
    {"rot_x", KeywordEnum::ROTATION_X},
    {"rot_y", KeywordEnum::ROTATION_Y},
    {"rot_z", KeywordEnum::ROTATION_Z},
    {"scaling", KeywordEnum::SCALING},
    {"camera", KeywordEnum::CAMERA},
    {"orthogonal", KeywordEnum::ORTHOGONAL},
    {"perspective", KeywordEnum::PERSPECTIVE},
    {"float", KeywordEnum::FLOAT},
    {"mesh", KeywordEnum::MESH}
};

// Abstract Token struct
export struct Token {
    /// A lexical token, used when parsing a scene file
    SourceLocation location;
    Token(SourceLocation loc) : location{loc} {}

    virtual ~Token() = default;
};

// StopToken struct: inherits from Token and is returned whenever a stream ends
export struct StopToken : Token {
    /// Token signalling the end of a file
    StopToken(SourceLocation location) : Token{location} {}
};

export struct IdentifierToken : Token {
    /// Token that contains the identifiers (i.e. name of variables that are not reserved keywords)
    std::string identifier;
    IdentifierToken(SourceLocation location, std::string id) : Token{location}, identifier{id} {}

    std::string get(){
        return identifier;
    }
};

export struct KeywordToken : Token {
    /// Toker that contains the keyword (variable type or reserved language word)
    KeywordEnum keyword;
    KeywordToken(SourceLocation location, KeywordEnum kw) : Token{location}, keyword{kw} {}

    KeywordEnum get() {
        return keyword;
    }
};

export struct SymbolToken : Token {
    /// Token that contains a symbol (e.g. variable name, bracket, comma ...)
    char symbol;
    SymbolToken(SourceLocation location, char symbol) : Token{location}, symbol{symbol} {}

    // Calling SymbolToken() it returns the string symbol inside it
    char get(){
        return symbol;
    }
};

export struct LiteralNumberToken : Token {
    /// Token that contains a literal float number (a number written digit by digit: i.e. 150 is 1-5-0)
    /// NOTE: other numerical types are not supported
    float number;
    LiteralNumberToken(SourceLocation location, float num) : Token{location}, number{num} {}

    float get() {
        return number;
    }
};

export struct LiteralStringToken : Token {
    /// Token that contains a string
    std::string string;
    LiteralStringToken(SourceLocation location, std::string str) : Token{location}, string{str} {}

    std::string get() {
        return string;
    }

};

export struct GrammarError {
    /// An error found by the lexer/parser while reading a scene file

    /// The fields of this type are the following:

    ///- `file_name`: the name of the file, or the empty string if there is no real file
    ///- `line_num`: the line number where the error was discovered (starting from 1)
    ///- `col_num`: the column number where the error was discovered (starting from 1)
    ///- `message`: a user-frendly error message

    SourceLocation location;
    std::string message;
};

export struct InputStream {
    std::istream& ifs;
    SourceLocation location;

    // saved_char is optional: there could be or not a previously saved character
    std::optional<char> saved_char;
    SourceLocation saved_location;

    std::optional<std::unique_ptr<Token>> saved_token;

    int tabulations;

    InputStream(std::istream& ifs, const std::string& filename="", int tabulations=8) :
        ifs(ifs),
        location{filename, 1, 1},
        tabulations(tabulations),
        saved_location{filename, 1, 1} {}


    // Reading methods
    void _update_location(std::optional<char> ch) {
        if (!ch.has_value()) {
            // If no character is read, do nothing
            return;
        }
        if (ch.value() == '\n') {
            // Row and columns are numbered starting from 1 (convention)
            location.line_num++;
            location.col_num = 1;
        } else if (ch.value() == '\t') {
            location.col_num += tabulations;
        } else {
            location.col_num++;
        }
    }

    std::optional<char> read_char() {
        /// Read a new character from the stream
        std::optional<char> ch;
        if (saved_char.has_value()) {
            // If there was a character saved in saved_char return that and empty saved_char
            ch = saved_char.value();
            saved_char = std::nullopt;
        } else {
            // If saved_char is empty get the new character from the stream
            char raw_char;
            // If I'm not at the end of file save the new character into raw_char and pass it to ch
            if (ifs.get(raw_char)) {
                ch = raw_char;
            } else {
                // If no character is read the character is filled with null
                ch = std::nullopt;
            };
        }

        // Save the current location in the saved_location
        saved_location = location;

        // Update the current location: if the ch is nullopt _update_location does nothing and we happy
        _update_location(ch);

        return ch;
    }

    void unread_char(std::optional<char> ch) {
        /// Returns from the previous character of the stream
        if (ch.has_value()) {
            // If an actual character was read, put it in saved_char (when we call read_char after
            // we are going to take the next character from saved_character and not read it from stream)
            // SAFETY CHECK: Crash immediately if we try to overwrite an unread character! It is a programmer error
            // not a user error, so it does not need to return error messages
            assert(!saved_char.has_value());
            saved_char = ch;
            // Return to the location saved before updating
            location = saved_location;
        } else {
            // If no character was read, there's nothing to unread, so do nothing
            return;
        }
    }

    // Function that skips whitespaces and comments
    void skip_whitespaces_and_comments() {

        std::optional<char> ch = read_char();

        while (ch.has_value() && (ch.value() == ' ' ||
                                  ch.value() == '\t' ||
                                  ch.value() == '\n' ||
                                  ch.value() == '\r' ||
                                  ch.value() == '#')) {

            if (ch.value() == '#') {
                // Keep reading till the end of the line
                std::optional<char> comment_ch = read_char();
                while (comment_ch.has_value() && comment_ch != '\r' && comment_ch != '\n') {
                    comment_ch = read_char();
                }
            }

            ch = read_char();
            if (!ch.has_value()) {
                return;
            }
        }

        // When a non whitespace, tab, return or comment is read put the character in the saved_character
        unread_char(ch);
    }

    std::expected<std::unique_ptr<Token>, GrammarError>_parse_string_token(SourceLocation token_location) {
        std::string res_string = "";
        std::optional<char> ch = read_char();

        while (ch.has_value() && ch.value() != '"') {
            res_string.push_back(ch.value());
            ch = read_char();
        }

        if (!ch.has_value()) {
            // It ended because we ran out of characters. Error!
            return std::unexpected(GrammarError{token_location, "Unterminated string"});
        }

        return std::make_unique<LiteralStringToken>(token_location, res_string);
    }

    std::expected<std::unique_ptr<Token>, GrammarError> _parse_float_token(char first_char, SourceLocation token_location) {
        std::string float_string = "";
        float_string.push_back(first_char);

        std::optional<char> ch = read_char();

        while (ch.has_value() && (std::isdigit(ch.value()) ||
                                  ch.value() == '.' ||
                                  ch.value() == 'e' ||
                                  ch.value() == 'E' ||
                                  ch.value() == '+' ||
                                  ch.value() == '-')) {

            float_string.push_back(ch.value());
            ch = read_char();
        }

        // If the loop finished and 'ch' still has a value, it means we hit a
        // character that isn't part of a number (like a comma or space). Put it back.
        if (ch.has_value()) {
            unread_char(ch);
        }

        float res_float;
        char* end_ptr = nullptr;

        errno = 0;

        res_float = std::strtof(float_string.c_str(), &end_ptr);

        // Check if the number was too big or too small to fit in a float
        if (errno == ERANGE) {
            return std::unexpected(GrammarError{
                token_location,
                std::format("'{}' is out of the floating-point value range.", float_string)
            });
        }

        // If the end_ptr didn't move from the start, it completely failed to parse a number
        if (end_ptr == float_string.c_str()) {
            return std::unexpected(GrammarError{
                token_location,
                std::format("'{}' is an invalid floating-point number", float_string)
            });
        }

        return std::make_unique<LiteralNumberToken>(token_location, res_float);

    }

    std::expected<std::unique_ptr<Token>, GrammarError> _parse_keyword_or_identifier_token(char first_char, SourceLocation token_location) {
        std::string res_string = "";
        res_string.push_back(first_char);

        std::optional<char> ch = read_char();

        while (ch.has_value() && (std::isalnum(ch.value()) || ch.value() == '_')) {
            res_string.push_back(ch.value());
            ch = read_char();
        }

        // If the loop finished and 'ch' still has a value, it means we hit a
        // character that isn't alphanumeric or underscore. Put it back.
        if (ch.has_value()) {
            unread_char(ch);
        }

        if (KEYWORDS.contains(res_string)) {
            return std::make_unique<KeywordToken>(token_location, KEYWORDS.at(res_string));
        } else {
            return std::make_unique<IdentifierToken>(token_location, res_string);
        }

    }

    std::expected<std::unique_ptr<Token>, GrammarError> read_token() {

        // If token was already read and saved, read it from saved_token
        if (saved_token.has_value()) {
            // If there was a Token saved in saved_char return that and empty saved_char
            auto result = std::move(saved_token.value());
            saved_token = std::nullopt;
            return result;
        }

        // First skip all whitespaces and comments
        skip_whitespaces_and_comments();

        // After all whitespaces and comments have been skipped read the character
        std::optional<char> ch = read_char();
        if (!ch.has_value()) {
            // If no character has been read, the file ended, so return the StopToken
            return std::make_unique<StopToken>(location);
        }

        // Now we have to see which Token starts with ch

        // Save the location of the beginning of the token for return
        SourceLocation token_location = location;

        if (SYMBOLS.contains(ch.value())) {
            // One character symbol like '(', ']', '>'
            return std::make_unique<SymbolToken>(token_location, ch.value());
        } else if (ch.value() == '"') {
            // Here starts a literal string
            return _parse_string_token(token_location);
        } else if (std::isdigit(ch.value()) || ch.value() == '.' || ch.value() == '+' || ch.value() == '-') {
            // Check if the character is the start of a float number
            return _parse_float_token(ch.value(), token_location);
        } else if (std::isalpha(ch.value()) || ch.value() == '_') {
            // Since it begins with an alphabetic character, it must either be a keyword or a identifier
            return _parse_keyword_or_identifier_token(ch.value(), token_location);
        } else {
            // We have an invalid character as '@' or '&'
            return std::unexpected(GrammarError{location, std::format("Invalid character '{}'", ch.value())});
        }
    }

    void unread_token (std::unique_ptr<Token> token){
        assert(!saved_token.has_value());
        saved_token = std::move(token);
    }
};


export struct Scene {
    std::unordered_map<std::string, std::shared_ptr<Material>> materials;
    World world;
    std::unique_ptr<Camera> camera = nullptr;
    std::unordered_map<std::string, float> float_variables;
    std::unordered_set<std::string> overridden_variables;
};

// ============================================================================================
// EXPECT FUNCTIONS
// ============================================================================================

std::expected<void, GrammarError> expect_symbol(InputStream& input_file, char symbol) {
    /// Read a token from `input_file` and check that it matches `symbol`.
    auto res = input_file.read_token();
    if (!res.has_value()) {
        return std::unexpected(res.error());
    }

    // Extract the value from the res variable
    std::unique_ptr<Token> token = std::move(res.value());

    // Is the token actually a SymbolToken?
    auto* sym_token = dynamic_cast<SymbolToken*>(token.get());

    // If it's not a symbol (it's a keyword or ...)
    if (sym_token == nullptr) {
        return std::unexpected(GrammarError{
            token->location,
            std::format("Expected symbol '{}', but got a different token type", symbol)
        });
    }
    // If it's the wrong symbol, throw the GrammarError
    if (sym_token->symbol != symbol) {
        return std::unexpected(GrammarError{
            token->location,
            std::format("Expected symbol '{}', but got {}", symbol, sym_token->symbol)
        });
    }

    // Success: returning empty brackets satisfies std::expected<void, ...>
    return {};
}

std::expected<KeywordEnum, GrammarError> expect_keywords(InputStream& input_file, const std::vector<KeywordEnum>& expected_keywords) {
    /// Read a token from `input_file` and check that it is one of the keywords in `keywords`

    auto res = input_file.read_token();
    if (!res.has_value()) {
        return std::unexpected(res.error());
    }

    std::unique_ptr<Token> token = std::move(res.value());

    // Checks if it's a KeywordToken
    auto* kw_token = dynamic_cast<KeywordToken*>(token.get());
    if (kw_token == nullptr) {
        return std::unexpected(GrammarError{token->location,"Expected a keyword, but got a different token type"});
    }

    // Checks if the keyword is inside our list of allowed keywords
    // std::ranges::find searches the vector. If it hits the end(), it didn't find it.
    if (std::ranges::find(expected_keywords, kw_token->keyword) == expected_keywords.end()) {
        return std::unexpected(GrammarError{token->location,"Got an unexpected keyword for this specific grammar rule"
        });
    }

    // Success: return the specific KeywordEnum so the parser knows which one it was.
    return kw_token->keyword;
}

std::expected<std::string, GrammarError> expect_identifier(InputStream& input_file) {
    /// Read a token from `input_file` and check that it is an identifier.

    auto res = input_file.read_token();
    if (!res.has_value()) {
        return std::unexpected(res.error());
    }

    std::unique_ptr<Token> token = std::move(res.value());
    auto* id_token = dynamic_cast<IdentifierToken*>(token.get());
    if (id_token == nullptr) {
        return std::unexpected(GrammarError{token->location, "Expected an Identifier, but got a different kind of token."});
    }

    // Success: return the identifier
    return id_token->identifier;
}

std::expected<std::string, GrammarError> expect_string(InputStream& input_file) {
    /// Read a token from `input_file` and check that it is a literal string.
    /// Returns a string
    auto res = input_file.read_token();
    if (!res.has_value()) {
        return std::unexpected(res.error());
    }

    std::unique_ptr<Token> token = std::move(res.value());
    auto* string_token = dynamic_cast<LiteralStringToken*>(token.get());
    if (string_token == nullptr) {
        return std::unexpected(GrammarError{token->location, "Expected an LiteralString, but got a different kind of token."});
    }

    // Success: return the string
    return string_token->string;
}

std::expected<float, GrammarError> expect_number(InputStream& input_file, Scene& scene) {
    /// Read a token from `input_file` and check that it is either a literal number or a variable in `scene`
    auto res = input_file.read_token();
    if (!res.has_value()) {
        return std::unexpected(res.error());
    }

    std::unique_ptr<Token> token = std::move(res.value());

    if (auto* num_token = dynamic_cast<LiteralNumberToken*>(token.get())) {
        return num_token->number;
    }

    if (auto* var_token = dynamic_cast<IdentifierToken*>(token.get())) {
        std::string var_name = var_token->identifier;
        if (!scene.float_variables.contains(var_name)) {
            return std::unexpected(GrammarError{token->location,std::format("Unknown variable '{}': variable not in the float_variables list", var_name)});
        }

        // Return the mapped float value from the dictionary
        return scene.float_variables.at(var_name);
    }

    return std::unexpected(GrammarError{token->location,"Expected a literal number or a variable, but got a different kind of token"});

}

// =====================================================================
// PARSE FUNCTIONS
// =====================================================================

std::expected<Vec, GrammarError> parse_vec(InputStream& input_file, Scene& scene) {

    auto open_square_brk = expect_symbol(input_file, '[');
    if (!open_square_brk.has_value()) { return std::unexpected(open_square_brk.error()); }

    auto vx_res = expect_number(input_file, scene);
    if (!vx_res.has_value()) { return std::unexpected(vx_res.error()); }
    float vx = vx_res.value();

    auto comma1 = expect_symbol(input_file, ',');
    if (comma1.has_value()) { return std::unexpected(comma1.error()); }

    auto vy_res = expect_number(input_file, scene);
    if (!vy_res.has_value()) { return std::unexpected(vy_res.error()); }
    float vy = vy_res.value();

    auto comma2 = expect_symbol(input_file, ',');
    if (!comma2.has_value()) { return std::unexpected(comma2.error()); }

    auto vz_res = expect_number(input_file, scene);
    if (!vz_res.has_value()) { return std::unexpected(vz_res.error()); }
    float vz = vz_res.value();

    expect_symbol(input_file, ']');

    return Vec{vx, vy, vz};

}

std::expected<Color, GrammarError> parse_color(InputStream& input_file, Scene& scene) {
    // Expect '<'
    auto sym1 = expect_symbol(input_file, '<');
    if (!sym1.has_value()) { return std::unexpected(sym1.error()); }

    // Expect red
    auto red = expect_number(input_file, scene);
    if (!red.has_value()) { return std::unexpected(red.error()); }

    // Expect ','
    auto sym2 = expect_symbol(input_file, ',');
    if (!sym2.has_value()) { return std::unexpected(sym2.error()); }

    // Expect Green
    auto green = expect_number(input_file, scene);
    if (!green.has_value()) { return std::unexpected(green.error()); }

    // Expect ','
    auto sym3 = expect_symbol(input_file, ',');
    if (!sym3.has_value()) { return std::unexpected(sym3.error()); }

    // Expect Blue
    auto blue = expect_number(input_file, scene);
    if (!blue.has_value()) { return std::unexpected(blue.error()); }

    // Expect '>'
    auto sym4 = expect_symbol(input_file, '>');
    if (!sym4.has_value()) { return std::unexpected(sym4.error()); }

    // Success: return the constructed Color
    return Color{red.value(), green.value(), blue.value()};
}

std::expected<std::unique_ptr<Pigment>, GrammarError> parse_pigment(InputStream& input_file, Scene& scene) {
    auto keyword_res = expect_keywords(input_file, {KeywordEnum::UNIFORM, KeywordEnum::CHECKERED, KeywordEnum::IMAGE});
    if (!keyword_res.has_value()) { return std::unexpected(keyword_res.error()); }
    KeywordEnum keyword = keyword_res.value();

    auto open_brk_res = expect_symbol(input_file, '(');
    if (open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

    if (keyword == KeywordEnum::UNIFORM) {
        auto color_res = parse_color(input_file, scene);
        if (!color_res.has_value()) { return std::unexpected(color_res.error()); }
        Color color = color_res.value();

        // Expect the closed parenthesis after the color
        auto close_brk_res = expect_symbol(input_file, ')');
        if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

        UniformPigment pigment(color);

        return std::make_unique<UniformPigment>(pigment);

    } else if (keyword == KeywordEnum::CHECKERED) {

        // First color
        auto color_res1 = parse_color(input_file, scene);
        if (!color_res1.has_value()) { return std::unexpected(color_res1.error()); }
        Color color1 = color_res1.value();

        // Comma
        auto comma_res = expect_symbol(input_file, ',');
        if (!comma_res.has_value()) { return std::unexpected(comma_res.error()); }

        // Second color
        auto color_res2 = parse_color(input_file, scene);
        if (!color_res2.has_value()) { return std::unexpected(color_res2.error()); }
        Color color2 = color_res2.value();

        // Comma
        auto comma2_res = expect_symbol(input_file, ',');
        if (!comma2_res.has_value()) { return std::unexpected(comma2_res.error()); }

        // Number of subdivisions
        auto subdiv_res = expect_number(input_file, scene);
        if (!subdiv_res.has_value()) { return std::unexpected(subdiv_res.error()); }
        int subdiv = static_cast<int>(subdiv_res.value());

        // Bracket close
        auto close_brk_res = expect_symbol(input_file, ')');
        if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

        CheckeredPigment checkered_pigment(color1, color2, subdiv);

        return std::make_unique<CheckeredPigment>(checkered_pigment);

    } else if (keyword == KeywordEnum::IMAGE) {

        auto filename_res = expect_string(input_file);
        if (!filename_res.has_value()) { return std::unexpected(filename_res.error()); }
        std::string filename = filename_res.value();

        // Closing bracket
        auto close_brk_res = expect_symbol(input_file, ')');
        if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

        // Load the pfm image
        auto image_res = HDRImage::read_pfm_file(filename.c_str());
        // If the file doesn't exist or is corrupted, convert the InvalidPfmFileFormat
        // into a GrammarError so the parser can report it safely
        if (!image_res.has_value()) {
            return std::unexpected(GrammarError{
                input_file.location,
                std::format("Failed to load image '{}': {}", filename, image_res.error().message)});
        }

        // Using move() to transfer the HDRImage inside the ImagePigment without copying it
        return std::make_unique<ImagePigment>(std::move(image_res.value()));

    }

    return std::unexpected(GrammarError{input_file.location, "Failed to parse pigment keyword."});

}

std::expected<std::unique_ptr<BRDF>, GrammarError> parse_brdf(InputStream& input_file, Scene& scene) {

    auto keyword_res = expect_keywords(input_file, {KeywordEnum::DIFFUSE, KeywordEnum::SPECULAR});
    if (!keyword_res.has_value()) { return std::unexpected(keyword_res.error()); }
    KeywordEnum keyword = keyword_res.value();

    auto open_brk_res = expect_symbol(input_file, '(');
    if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

    auto pigment_res = parse_pigment(input_file, scene);
    if (!pigment_res.has_value()) { return std::unexpected(pigment_res.error()); }
    std::unique_ptr<Pigment> pigment = std::move(pigment_res.value());

    auto close_brk_res = expect_symbol(input_file, ')');
    if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

    if (keyword == KeywordEnum::DIFFUSE) {
        return std::make_unique<DiffusiveBRDF>(std::move(pigment));
    } else if (keyword == KeywordEnum::SPECULAR) {
        return std::make_unique<SpecularBRDF>(std::move(pigment));
    }

    return std::unexpected(GrammarError{input_file.location, "Failed to parse BRDF."});

}

std::expected<std::pair<std::string, Material>, GrammarError> parse_material(InputStream& input_file, Scene& scene) {

    auto identifier_res = expect_string(input_file);
    if (!identifier_res.has_value()) { return std::unexpected(identifier_res.error()); }
    std::string name = identifier_res.value();

    auto open_brk_res = expect_symbol(input_file, '(');
    if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

    auto brdf_res = parse_brdf(input_file, scene);
    if (!brdf_res.has_value()) { return std::unexpected(brdf_res.error()); }
    std::unique_ptr<BRDF> brdf = std::move(brdf_res.value());

    auto comma = expect_symbol(input_file, ',');
    if (!comma.has_value()) { return std::unexpected(comma.error()); }

    auto emitted_rad_res = parse_pigment(input_file, scene);
    if (!emitted_rad_res.has_value()) { return std::unexpected(emitted_rad_res.error()); }
    std::unique_ptr<Pigment> emitted_rad = std::move(emitted_rad_res.value());

    auto close_brk_res = expect_symbol(input_file, ')');

    Material material{std::move(brdf), std::move(emitted_rad)};

    return std::pair<std::string, Material>{name, std::move(material)};
}

std::expected<Transformation, GrammarError> parse_transformation(InputStream& input_file, Scene& scene) {
    auto result = Transformation{};

    // State flag controlling the loop
    bool has_next_transformation = true;

    while (has_next_transformation) {
        auto kw_res = expect_keywords(input_file, std::vector<KeywordEnum>{
                                                                         KeywordEnum::IDENTITY,
                                                                         KeywordEnum::TRANSLATION,
                                                                         KeywordEnum::ROTATION_X,
                                                                         KeywordEnum::ROTATION_Y,
                                                                         KeywordEnum::ROTATION_Z,
                                                                         KeywordEnum::SCALING
                                                                         });
        if (!kw_res.has_value()) { return std::unexpected(kw_res.error()); }
        KeywordEnum kw = kw_res.value();

        if (kw == KeywordEnum::IDENTITY) {
            // Do nothing (Primitive optimization)
        } else if (kw == KeywordEnum::TRANSLATION) {

            auto open_brk_res = expect_symbol(input_file, '(');
            if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

            auto vec_res = parse_vec(input_file, scene);
            if (!vec_res.has_value()) { return std::unexpected(vec_res.error()); }
            Vec vec = vec_res.value();

            auto close_brk_res = expect_symbol(input_file, ')');
            if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

            result = result * Trans(vec);

        } else if (kw == KeywordEnum::ROTATION_X) {

            auto open_brk_res = expect_symbol(input_file, '(');
            if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

            auto angle_res = expect_number(input_file, scene);
            if (!angle_res.has_value()) { return std::unexpected(angle_res.error()); }
            float angle = angle_res.value();

            auto close_brk_res = expect_symbol(input_file, ')');
            if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

            result = result * R_x(angle);

        } else if (kw == KeywordEnum::ROTATION_Y) {

            auto open_brk_res = expect_symbol(input_file, '(');
            if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

            auto angle_res = expect_number(input_file, scene);
            if (!angle_res.has_value()) { return std::unexpected(angle_res.error()); }
            float angle = angle_res.value();

            auto close_brk_res = expect_symbol(input_file, ')');
            if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

            result = result * R_y(angle);

        } else if (kw == KeywordEnum::ROTATION_Z) {

            auto open_brk_res = expect_symbol(input_file, '(');
            if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

            auto angle_res = expect_number(input_file, scene);
            if (!angle_res.has_value()) { return std::unexpected(angle_res.error()); }
            float angle = angle_res.value();

            auto close_brk_res = expect_symbol(input_file, ')');
            if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

            result = result * R_z(angle);

        } else if (kw == KeywordEnum::SCALING) {

            auto open_brk_res = expect_symbol(input_file, '(');
            if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

            auto vec_res = parse_vec(input_file, scene);
            if (!vec_res.has_value()) { return std::unexpected(vec_res.error()); }
            Vec vec = vec_res.value();

            auto close_brk_res = expect_symbol(input_file, ')');
            if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

            result = result * Scale(vec);

        }

        // We must peek the next token to check if there is another transformation that is being
        // chained or if the sequence ends. Thus, this is a LL(1) parser

        auto next_token_res = input_file.read_token();
        if (!next_token_res.has_value()) { return std::unexpected(next_token_res.error()); }
        std::unique_ptr<Token> next_token = std::move(next_token_res.value());

        auto* sym_token = dynamic_cast<SymbolToken*>(next_token.get());

        // Update the state flag instead of using break/continue
        if (sym_token != nullptr && sym_token->symbol == '*') {
            // A '*' was found. Keep the flag true so the while loop repeats.
            has_next_transformation = true;
        } else {
            // The chain is finished. Put the token back and cleanly toggle the flag to false.
            input_file.unread_token(std::move(next_token));
            has_next_transformation = false;
        }
    }

    return result;

}

// Use 'T' to specify which concrete shape to build (e.g., Sphere, Cube, Plane)
template <typename T>
std::expected<std::unique_ptr<Shape>, GrammarError> parse_shape(InputStream& input_file, Scene& scene) {

    auto open_brk_res = expect_symbol(input_file, '(');
    if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

    auto transformation_res = parse_transformation(input_file, scene);
    if (!transformation_res.has_value()) { return std::unexpected(transformation_res.error()); }
    Transformation transformation = transformation_res.value();

    auto comma_res = expect_symbol(input_file, ',');
    if (!comma_res.has_value()) { return std::unexpected(comma_res.error()); }

    // Read the material name
    auto mat_name_res = expect_identifier(input_file);
    if (!mat_name_res.has_value()) { return std::unexpected(mat_name_res.error()); }
    std::string material_name = mat_name_res.value();

    // Look up the name in the dictionary
    if (!scene.materials.contains(material_name)) {
        return std::unexpected(GrammarError{
            input_file.location,
            std::format("Unknown material '{}' applied to shape.", material_name)
        });
    }

    // Grab the existing shared pointer from the dictionary
    std::shared_ptr<Material> material = scene.materials.at(material_name);

    auto close_brk_res = expect_symbol(input_file, ')');
    if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

    // Return the "T" shape and cast it to unique_ptr in order to put it in World
    return std::make_unique<T>(transformation, material);
}

std::expected<std::unique_ptr<Camera>, GrammarError> parse_camera(InputStream& input_file, Scene& scene) {

    auto open_brk_res = expect_symbol(input_file, '(');
    if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

    auto keyword_res = expect_keywords(input_file, {KeywordEnum::PERSPECTIVE, KeywordEnum::ORTHOGONAL});
    if (!keyword_res.has_value()) { return std::unexpected(keyword_res.error()); }
    KeywordEnum keyword = keyword_res.value();

    auto comma1_res = expect_symbol(input_file, ',');
    if (!comma1_res.has_value()) { return std::unexpected(comma1_res.error()); }

    auto aspect_ratio_res = expect_number(input_file, scene);
    if (!aspect_ratio_res.has_value()) { return std::unexpected(aspect_ratio_res.error()); }
    float aspect_ratio = aspect_ratio_res.value();

    float distance=0.0f;
    if (keyword == KeywordEnum::PERSPECTIVE) {
        auto comma2_res = expect_symbol(input_file, ',');
        if (!comma2_res.has_value()) { return std::unexpected(comma2_res.error()); }

        auto distance_res = expect_number(input_file, scene);
        if (!distance_res.has_value()) { return std::unexpected(distance_res.error()); }
        distance = distance_res.value();
    }

    auto comma3_res = expect_symbol(input_file, ',');
    if (!comma3_res.has_value()) { return std::unexpected(comma3_res.error()); }

    auto transformation_res = parse_transformation(input_file, scene);
    if (!transformation_res.has_value()) { return std::unexpected(transformation_res.error()); }
    Transformation transformation = transformation_res.value();

    auto close_brk_res = expect_symbol(input_file, ')');
    if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

    if (keyword == KeywordEnum::ORTHOGONAL) {
        return std::make_unique<OrthogonalCamera>(aspect_ratio, transformation);
    } else if (keyword == KeywordEnum::PERSPECTIVE) {
        return std::make_unique<PerspectiveCamera>(aspect_ratio, distance, transformation);
    }

    return std::unexpected(GrammarError{input_file.location, "Failed to parse Camera"});

}