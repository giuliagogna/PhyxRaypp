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
import Mesh;

/// @brief A string view containing all single-character symbols recognized by the language lexer.
export constexpr std::string_view SYMBOLS = "()<>[],*=;";

/// @brief Represents a specific position within a source file.
/// Used to precisely locate syntax errors for the user.
/// This class has the following fields:
///
/// - file_name: the name of the file, or the empty string if there is no file associated with this location
/// (e.g., because the source code was provided as a memory stream, or through a network connection)
/// - line_num: number of the line (starting from 1)
/// - col_num: number of the column (starting from 1)
export struct SourceLocation {
    std::string filename = "";
    int line_num = 0;
    int col_num = 0;
};

/// @brief Enumeration of all reserved keywords in the scene description language.
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

/// @brief Dictionary linking literal text strings to their corresponding KeywordEnum.
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

/// @brief Abstract base class representing a single lexical token.
export struct Token {
    SourceLocation location;
    Token(SourceLocation loc) : location{loc} {}

    virtual ~Token() = default;
};

/// @brief Token signaling the end of the input stream (EOF).
export struct StopToken : Token {
    StopToken(SourceLocation location) : Token{location} {}
};

/// @brief Token containing an identifier (e.g., custom variable or material names).
export struct IdentifierToken : Token {
    std::string identifier;
    IdentifierToken(SourceLocation location, std::string id) : Token{location}, identifier{id} {}

    std::string get(){
        return identifier;
    }
};

/// @brief Token containing a reserved language keyword.
export struct KeywordToken : Token {
    KeywordEnum keyword;
    KeywordToken(SourceLocation location, KeywordEnum kw) : Token{location}, keyword{kw} {}

    KeywordEnum get() {
        return keyword;
    }
};

/// @brief Token containing a syntax symbol (e.g., brackets, commas, equals signs).
export struct SymbolToken : Token {
    char symbol;
    SymbolToken(SourceLocation location, char symbol) : Token{location}, symbol{symbol} {}

    // Calling SymbolToken() it returns the string symbol inside it
    char get(){
        return symbol;
    }
};

/// @brief Token containing a literal floating-point number.
///  NOTE: other numerical types are not supported
export struct LiteralNumberToken : Token {
    float number;
    LiteralNumberToken(SourceLocation location, float num) : Token{location}, number{num} {}

    float get() {
        return number;
    }
};

/// @brief Token containing a literal string enclosed in quotes (e.g., file paths).
export struct LiteralStringToken : Token {
    std::string string;
    LiteralStringToken(SourceLocation location, std::string str) : Token{location}, string{str} {}

    std::string get() {
        return string;
    }

};


/// @brief An error found by the lexer or parser while reading a scene file.
/// Contains the location of the error and a user-friendly message.
export struct GrammarError {

    SourceLocation location;
    std::string message;
};

/// @brief A high-level stream wrapper used to safely parse scene files.
///
/// This struct tracks line and column numbers automatically and allows the parser
/// to "unread" characters and tokens, a requirement for LL(1) lookahead parsing.
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


    // Internal helper to keep track of columns and lines
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

    /// @brief Reads the next character from the stream or the unread buffer.
    ///
    ///- If there was a character saved in `saved_char` return that and empty saved_char
    ///- If `saved_char` is empty get the new character from the stream
    ///- If end of file has not been reached save the new character into ch
    ///- If no character is read ch is filled with null
    ///- Save the current location and update it
    /// @return An optional containing the char, or `nullopt` if the stream is empty.
    std::optional<char> read_char() {
        /// Read a new character from the stream
        std::optional<char> ch;
        if (saved_char.has_value()) {
            ch = saved_char.value();
            saved_char = std::nullopt;
        } else {
            char raw_char;
            if (ifs.get(raw_char)) {
                ch = raw_char;
            } else {
                ch = std::nullopt;
            };
        }

        saved_location = location;
        _update_location(ch);
        return ch;
    }

    /// @brief Pushes a character back into the stream's buffer.
    ///
    /// The next call to read_char() will consume this saved character instead of pulling
    /// from the physical file and returns to the location where it was before reading.
    /// If an empty character (nullopt) is passed, this function silently does nothing.
    ///
    /// @param ch The character to unread.
    /// @warning Calling this when a character is already buffered is a fatal programmer error (use assert).
    void unread_char(std::optional<char> ch) {
        if (ch.has_value()) {
            assert(!saved_char.has_value());
            saved_char = ch;
            location = saved_location;
        } else {
            return;
        }
    }

    /// @brief Advances the stream past any whitespace characters, tabs, or # comments.
    void skip_whitespaces_and_comments() {

        std::optional<char> ch = read_char();

        while (ch.has_value() && (ch.value() == ' ' ||
                                  ch.value() == '\t' ||
                                  ch.value() == '\n' ||
                                  ch.value() == '\r' ||
                                  ch.value() == '#')) {

            if (ch.value() == '#') {
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

        unread_char(ch);
    }

    // ==========================================================================================
    // Helper functions for parse elements of a token
    // ==========================================================================================

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

    // ==========================================================================================

    /// @brief Reads the next full lexical token from the stream.
    ///
    /// - If token was already read and saved, it reads it from saved_token
    /// @return A unique pointer to the parsed Token, or a GrammarError if lexing fails.
    std::expected<std::unique_ptr<Token>, GrammarError> read_token() {

        if (saved_token.has_value()) {
            // If there was a Token saved in saved_char return that and empty saved_char
            auto result = std::move(saved_token.value());
            saved_token = std::nullopt;
            return result;
        }

        skip_whitespaces_and_comments();

        std::optional<char> ch = read_char();
        if (!ch.has_value()) {
            return std::make_unique<StopToken>(location);
        }

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

    /// @brief Pushes an entire token back into the stream's buffer.
    /// @param token The Token unique_ptr to return to the stream.
    void unread_token (std::unique_ptr<Token> token){
        assert(!saved_token.has_value());
        saved_token = std::move(token);
    }
};


/// @brief Represents a fully parsed scene containing all objects needed for rendering.
export struct Scene {
    std::unordered_map<std::string, std::shared_ptr<Material>> materials;
    World world;
    std::unique_ptr<Camera> camera = nullptr;
    std::unordered_map<std::string, float> float_variables;
    std::unordered_set<std::string> overridden_variables;
};


export {

    // ============================================================================================
    // EXPECT FUNCTIONS
    // ============================================================================================

    /// @brief Reads a token and validates that it is a SymbolToken matching the provided char.
    /// @param input_file The InputStream to read from.
    /// @param symbol The exact character symbol expected (e.g., '(', ')', '=').
    /// @return An empty expected object on success, or a GrammarError if mismatched.
    std::expected<void, GrammarError> expect_symbol(InputStream& input_file, char symbol) {
        /// Read a token from `input_file` and check that it matches `symbol`.
        auto res = input_file.read_token();
        if (!res.has_value()) {
            return std::unexpected(res.error());
        }
        std::unique_ptr<Token> token = std::move(res.value());

        auto* sym_token = dynamic_cast<SymbolToken*>(token.get());

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
                std::format("Expected symbol '{}', but got '{}'", symbol, sym_token->symbol)
            });
        }

        return {};
    }

    /// @brief Reads a token and validates that it is a KeywordToken matching one of the options.
    /// @param input_file The InputStream to read from.
    /// @param expected_keywords A vector of valid KeywordEnums.
    /// @return The specific KeywordEnum that was found.
    std::expected<KeywordEnum, GrammarError> expect_keywords(InputStream& input_file, const std::vector<KeywordEnum>& expected_keywords) {

        auto res = input_file.read_token();
        if (!res.has_value()) {
            return std::unexpected(res.error());
        }

        std::unique_ptr<Token> token = std::move(res.value());

        auto* kw_token = dynamic_cast<KeywordToken*>(token.get());
        if (kw_token == nullptr) {
            return std::unexpected(GrammarError{token->location,"Expected a keyword, but got a different token type"});
        }

        if (std::ranges::find(expected_keywords, kw_token->keyword) == expected_keywords.end()) {
            return std::unexpected(GrammarError{token->location,"Got an unexpected keyword for this specific grammar rule"
            });
        }

        return kw_token->keyword;
    }

    /// @brief Reads a token and validates that it is an IdentifierToken.
    /// @param input_file The InputStream to read from.
    /// @return The string name of the identifier.
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

        return id_token->identifier;
    }


    /// @brief Reads a token and validates that it is a LiteralStringToken.
    /// @param input_file The InputStream to read from.
    /// @return The string value parsed from the quotes.
    std::expected<std::string, GrammarError> expect_string(InputStream& input_file) {

        auto res = input_file.read_token();
        if (!res.has_value()) {
            return std::unexpected(res.error());
        }

        std::unique_ptr<Token> token = std::move(res.value());
        auto* string_token = dynamic_cast<LiteralStringToken*>(token.get());
        if (string_token == nullptr) {
            return std::unexpected(GrammarError{token->location, "Expected an LiteralString, but got a different kind of token."});
        }

        return string_token->string;
    }

    /// @brief Reads a token and checks if it is a literal number or a previously defined float variable.
    /// @param input_file The InputStream to read from.
    /// @param scene The Scene dictionary containing saved variables.
    /// @return The floating point value.
    std::expected<float, GrammarError> expect_number(InputStream& input_file, Scene& scene) {

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

            return scene.float_variables.at(var_name);
        }

        return std::unexpected(GrammarError{token->location,"Expected a literal number or a variable, but got a different kind of token"});

    }

    // =====================================================================
    // PARSE FUNCTIONS
    // =====================================================================

    /// @brief Parses a 3D Geometry vector enclosed in brackets `[x, y, z]`.
    /// @param input_file The InputStream pointing to the vector text.
    /// @param scene The Scene object (used for variable lookups).
    /// @return The constructed Vec object.
    std::expected<Vec, GrammarError> parse_vec(InputStream& input_file, Scene& scene) {

        auto open_square_brk = expect_symbol(input_file, '[');
        if (!open_square_brk.has_value()) { return std::unexpected(open_square_brk.error()); }

        auto vx_res = expect_number(input_file, scene);
        if (!vx_res.has_value()) { return std::unexpected(vx_res.error()); }
        float vx = vx_res.value();

        auto comma1 = expect_symbol(input_file, ',');
        if (!comma1.has_value()) { return std::unexpected(comma1.error()); }

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

    /// @brief Parses an RGB Color enclosed in angle brackets `<r, g, b>`.
    /// @param input_file The InputStream pointing to the color text.
    /// @param scene The Scene object (used for variable lookups).
    /// @return The constructed Color object.
    std::expected<Color, GrammarError> parse_color(InputStream& input_file, Scene& scene) {

        auto sym1 = expect_symbol(input_file, '<');
        if (!sym1.has_value()) { return std::unexpected(sym1.error()); }

        auto red = expect_number(input_file, scene);
        if (!red.has_value()) { return std::unexpected(red.error()); }

        auto sym2 = expect_symbol(input_file, ',');
        if (!sym2.has_value()) { return std::unexpected(sym2.error()); }

        auto green = expect_number(input_file, scene);
        if (!green.has_value()) { return std::unexpected(green.error()); }

        auto sym3 = expect_symbol(input_file, ',');
        if (!sym3.has_value()) { return std::unexpected(sym3.error()); }

        auto blue = expect_number(input_file, scene);
        if (!blue.has_value()) { return std::unexpected(blue.error()); }

        auto sym4 = expect_symbol(input_file, '>');
        if (!sym4.has_value()) { return std::unexpected(sym4.error()); }


        return Color{red.value(), green.value(), blue.value()};
    }

    /// @brief Parses a Pigment type (uniform, checkered, or image mapping).
    /// @param input_file The InputStream pointing to the pigment keyword.
    /// @param scene The Scene object.
    /// @return A unique pointer safely wrapped around the polymorphic Pigment object.
    std::expected<std::unique_ptr<Pigment>, GrammarError> parse_pigment(InputStream& input_file, Scene& scene) {
        auto keyword_res = expect_keywords(input_file, {KeywordEnum::UNIFORM, KeywordEnum::CHECKERED, KeywordEnum::IMAGE});
        if (!keyword_res.has_value()) { return std::unexpected(keyword_res.error()); }
        KeywordEnum keyword = keyword_res.value();

        auto open_brk_res = expect_symbol(input_file, '(');
        if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

        if (keyword == KeywordEnum::UNIFORM) {
            auto color_res = parse_color(input_file, scene);
            if (!color_res.has_value()) { return std::unexpected(color_res.error()); }
            Color color = color_res.value();

            auto close_brk_res = expect_symbol(input_file, ')');
            if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

            UniformPigment pigment(color);

            return std::make_unique<UniformPigment>(pigment);

        } else if (keyword == KeywordEnum::CHECKERED) {

            auto color_res1 = parse_color(input_file, scene);
            if (!color_res1.has_value()) { return std::unexpected(color_res1.error()); }
            Color color1 = color_res1.value();

            auto comma_res = expect_symbol(input_file, ',');
            if (!comma_res.has_value()) { return std::unexpected(comma_res.error()); }

            auto color_res2 = parse_color(input_file, scene);
            if (!color_res2.has_value()) { return std::unexpected(color_res2.error()); }
            Color color2 = color_res2.value();

            auto comma2_res = expect_symbol(input_file, ',');
            if (!comma2_res.has_value()) { return std::unexpected(comma2_res.error()); }

            auto subdiv_res = expect_number(input_file, scene);
            if (!subdiv_res.has_value()) { return std::unexpected(subdiv_res.error()); }
            int subdiv = static_cast<int>(subdiv_res.value());

            auto close_brk_res = expect_symbol(input_file, ')');
            if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

            CheckeredPigment checkered_pigment(color1, color2, subdiv);

            return std::make_unique<CheckeredPigment>(checkered_pigment);

        } else if (keyword == KeywordEnum::IMAGE) {

            auto filename_res = expect_string(input_file);
            if (!filename_res.has_value()) { return std::unexpected(filename_res.error()); }
            std::string filename = filename_res.value();

            auto close_brk_res = expect_symbol(input_file, ')');
            if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

            auto image_res = HDRImage::read_pfm_file(filename.c_str());
            if (!image_res.has_value()) {
                return std::unexpected(GrammarError{
                    input_file.location,
                    std::format("Failed to load image '{}': {}", filename, image_res.error().message)});
            }

            return std::make_unique<ImagePigment>(std::move(image_res.value()));

        }

        return std::unexpected(GrammarError{input_file.location, "Failed to parse pigment keyword."});

    }

    /// @brief Parses a Bidirectional Reflectance Distribution Function (diffuse or specular).
    /// @param input_file The InputStream.
    /// @param scene The active Scene object.
    /// @return A unique pointer to the polymorphic BRDF object.
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

    /// @brief Parses a Material containing its identifier, BRDF, and Emitted Radiance pigment.
    /// @param input_file The InputStream.
    /// @param scene The active Scene object.
    /// In the scene definition the `emitted_radiance` can be omitted: if none is provided, the default
    /// black will be applied.
    /// @return A key-value pair of the Material's string identifier and the constructed Material object.
    std::expected<std::pair<std::string, Material>, GrammarError> parse_material(InputStream& input_file, Scene& scene) {

        auto identifier_res = expect_identifier(input_file);
        if (!identifier_res.has_value()) { return std::unexpected(identifier_res.error()); }
        std::string name = identifier_res.value();

        auto open_brk_res = expect_symbol(input_file, '(');
        if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

        auto brdf_res = parse_brdf(input_file, scene);
        if (!brdf_res.has_value()) { return std::unexpected(brdf_res.error()); }
        std::unique_ptr<BRDF> brdf = std::move(brdf_res.value());

        // ========================================
        // Optional parameter emitted_radiance
        // ========================================
        auto next_tok_res = input_file.read_token();
        if (!next_tok_res.has_value()) { return std::unexpected(next_tok_res.error()); }
        std::unique_ptr<Token> next_tok = std::move(next_tok_res.value());

        auto* sym_tok = dynamic_cast<SymbolToken*>(next_tok.get());

        // If the ) is found, use the constructor with default emitted_radiance
        if (sym_tok != nullptr && sym_tok->symbol == ')') {
            Material material{std::move(brdf)};
            return std::pair<std::string, Material>{name, std::move(material)};
        }
        else if (sym_tok != nullptr && sym_tok->symbol == ',') {
            auto emitted_rad_res = parse_pigment(input_file, scene);
            if (!emitted_rad_res.has_value()) { return std::unexpected(emitted_rad_res.error()); }
            std::unique_ptr<Pigment> emitted_rad = std::move(emitted_rad_res.value());

            auto close_brk_res = expect_symbol(input_file, ')');
            if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

            Material material{std::move(brdf), std::move(emitted_rad)};

            return std::pair<std::string, Material>{name, std::move(material)};
        }
        else {
            return std::unexpected(GrammarError{next_tok->location, "Expected ',' or ')' after BRDF definition."});
        }
    }

    /// @brief Parses an LL(1) chain of affine transformations connected by '*'.
    /// @param input_file The InputStream.
    /// @param scene The active Scene object.
    /// @return The aggregated composite Transformation matrix.
    std::expected<Transformation, GrammarError> parse_transformation(InputStream& input_file, Scene& scene) {
        auto result = Transformation{};

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
                // Do nothing if the transformation is identity
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

            // Lookahead parsing logic
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

    /// @brief Parses a specific shape geometry and its applied material from the input stream.
    ///
    /// Reads the transformation matrix and material identifier for a shape.
    /// Performs a dictionary lookup to bind the shape to an already-defined material in the scene.
    ///
    /// @tparam T The specific Shape class to instantiate (e.g., Sphere, Plane, Cube).
    /// @param input_file The input stream currently pointing to the shape's parameter list.
    /// @param scene The active Scene object containing the material dictionary.
    /// @return A unique pointer to the newly allocated Shape.
    /// @warning Throws a GrammarError if the material name is not found in the scene's dictionary.
    template <typename T>
    std::expected<std::unique_ptr<Shape>, GrammarError> parse_shape(InputStream& input_file, Scene& scene) {

        auto open_brk_res = expect_symbol(input_file, '(');
        if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

        auto transformation_res = parse_transformation(input_file, scene);
        if (!transformation_res.has_value()) { return std::unexpected(transformation_res.error()); }
        Transformation transformation = transformation_res.value();

        auto comma_res = expect_symbol(input_file, ',');
        if (!comma_res.has_value()) { return std::unexpected(comma_res.error()); }

        auto mat_name_res = expect_identifier(input_file);
        if (!mat_name_res.has_value()) { return std::unexpected(mat_name_res.error()); }
        std::string material_name = mat_name_res.value();

        if (!scene.materials.contains(material_name)) {
            return std::unexpected(GrammarError{
                input_file.location,
                std::format("Unknown material '{}' applied to shape.", material_name)
            });
        }

        std::shared_ptr<Material> material = scene.materials.at(material_name);

        auto close_brk_res = expect_symbol(input_file, ')');
        if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

        return std::make_unique<T>(transformation, material);
    }

    /// @brief Parses a 3D Mesh object from an external .obj file.
    ///
    /// Reads the transformation, material identifier, and the file path.
    /// Optionally accepts two additional integer parameters for the BVH builder: (n_bins, leaf_threshold).
    ///
    /// @param input_file The InputStream pointing to the mesh parameters.
    /// @param scene The active Scene object containing the material dictionary.
    /// @return A unique pointer to the constructed Mesh safely upcast to Shape.
    /// @warning Throws a GrammarError if the material is unknown or the file cannot be opened.
    std::expected<std::unique_ptr<Shape>, GrammarError> parse_mesh(InputStream& input_file, Scene& scene) {

        auto open_brk_res = expect_symbol(input_file, '(');
        if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

        auto file_res = expect_string(input_file);
        if (!file_res.has_value()) { return std::unexpected(file_res.error()); }
        std::string filename = file_res.value();

        // Safety Check: Prevent the constructor from throwing an uncatchable exception
        std::ifstream fs(filename);
        if (!fs.is_open()) {
            return std::unexpected(GrammarError{
                input_file.location,
                std::format("Cannot open mesh file '{}'", filename)
            });
        }
        fs.close();

        auto comma1 = expect_symbol(input_file, ',');
        if (!comma1.has_value()) { return std::unexpected(comma1.error()); }

        auto mat_name_res = expect_identifier(input_file);
        if (!mat_name_res.has_value()) { return std::unexpected(mat_name_res.error()); }
        std::string material_name = mat_name_res.value();

        if (!scene.materials.contains(material_name)) {
            return std::unexpected(GrammarError{
                input_file.location,
                std::format("Unknown material '{}' applied to mesh.", material_name)
            });
        }
        std::shared_ptr<Material> material = scene.materials.at(material_name);

        auto comma2 = expect_symbol(input_file, ',');
        if (!comma2.has_value()) { return std::unexpected(comma2.error()); }

        auto transformation_res = parse_transformation(input_file, scene);
        if (!transformation_res.has_value()) { return std::unexpected(transformation_res.error()); }
        Transformation transformation = transformation_res.value();

        // =========================================================
        // OPTIONAL PARAMETER PARSING
        // =========================================================

        auto next_tok_res = input_file.read_token();
        if (!next_tok_res.has_value()) { return std::unexpected(next_tok_res.error()); }
        std::unique_ptr<Token> tok = std::move(next_tok_res.value());
        auto* sym_tok = dynamic_cast<SymbolToken*>(tok.get());

        if (sym_tok != nullptr && sym_tok->symbol == ')') {
            // Only 3 arguments provided.
            // Call the constructor with 3 args
            // Mesh.cppm automatically injects its own defaults
            return std::make_unique<Mesh>(filename, material, transformation);
        }
        else if (sym_tok != nullptr && sym_tok->symbol == ',') {
            // A 4th argument was provided (n_bins)
            auto bins_res = expect_number(input_file, scene);
            if (!bins_res.has_value()) { return std::unexpected(bins_res.error()); }
            int bvh_n_bins = static_cast<int>(bins_res.value());

            auto next_tok2_res = input_file.read_token();
            if (!next_tok2_res.has_value()) { return std::unexpected(next_tok2_res.error()); }
            std::unique_ptr<Token> tok2 = std::move(next_tok2_res.value());
            auto* sym_tok2 = dynamic_cast<SymbolToken*>(tok2.get());

            if (sym_tok2 != nullptr && sym_tok2->symbol == ')') {
                // Finished: 4 args passed
                // Mesh.cppm injects its default for threshold (3)
                return std::make_unique<Mesh>(filename, material, transformation, bvh_n_bins);
            }
            else if (sym_tok2 != nullptr && sym_tok2->symbol == ',') {
                // Case 3: A 5th argument was provided (threshold)
                auto thresh_res = expect_number(input_file, scene);
                if (!thresh_res.has_value()) { return std::unexpected(thresh_res.error()); }
                int bvh_threshold = static_cast<int>(thresh_res.value());

                auto final_close = expect_symbol(input_file, ')');
                if (!final_close.has_value()) { return std::unexpected(final_close.error()); }

                // Finished: all 5 args passed explicitly
                return std::make_unique<Mesh>(filename, material, transformation, bvh_n_bins, bvh_threshold);
            }
            else {
                return std::unexpected(GrammarError{tok2->location, "Expected ',' or ')' after mesh BVH bins."});
            }
        }
        else {
            return std::unexpected(GrammarError{tok->location, "Expected ',' or ')' after mesh filename."});
        }
    }

    /// @brief Helper function to check if a keyword belongs to the Transformation family.
    bool is_transformation_keyword(KeywordEnum kw) {
        return kw == KeywordEnum::IDENTITY ||
               kw == KeywordEnum::TRANSLATION ||
               kw == KeywordEnum::ROTATION_X ||
               kw == KeywordEnum::ROTATION_Y ||
               kw == KeywordEnum::ROTATION_Z ||
               kw == KeywordEnum::SCALING;
    }

    /// @brief Parses a Camera declaration from the scene file.
    /// @param input_file The InputStream.
    /// @param scene The active Scene object.
    /// @return A unique pointer to the Camera polymorphism wrapper.
    /// Aspect ratio, distance, and transformation are all optional.
    std::expected<std::unique_ptr<Camera>, GrammarError> parse_camera(InputStream& input_file, Scene& scene) {

        auto open_brk_res = expect_symbol(input_file, '(');
        if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

        auto keyword_res = expect_keywords(input_file, {KeywordEnum::PERSPECTIVE, KeywordEnum::ORTHOGONAL});
        if (!keyword_res.has_value()) { return std::unexpected(keyword_res.error()); }
        KeywordEnum keyword = keyword_res.value();

        // Query the structs for their exact defaults
        float aspect_ratio = PerspectiveCamera{}.aspect_ratio;
        float distance = PerspectiveCamera{}.d;
        Transformation transformation{};


        // Peek at the next token
        auto tok1_res = input_file.read_token();
        if (!tok1_res.has_value()) { return std::unexpected(tok1_res.error()); }
        std::unique_ptr<Token> tok1 = std::move(tok1_res.value());
        auto* sym1 = dynamic_cast<SymbolToken*>(tok1.get());

        if (sym1 != nullptr && sym1->symbol == ')') {
            if (keyword == KeywordEnum::ORTHOGONAL) return std::make_unique<OrthogonalCamera>();
            else return std::make_unique<PerspectiveCamera>();
        }
        if (sym1 == nullptr || sym1->symbol != ',') {
            return std::unexpected(GrammarError{tok1->location, "Expected ',' or ')' after camera type."});
        }

        auto peek2_res = input_file.read_token();
        if (!peek2_res.has_value()) { return std::unexpected(peek2_res.error()); }
        std::unique_ptr<Token> peek2 = std::move(peek2_res.value());
        auto* kw2 = dynamic_cast<KeywordToken*>(peek2.get());

        if (kw2 != nullptr && is_transformation_keyword(kw2->keyword)) {
            // User skipped aspect ratio and distance to go straight to transformation
            input_file.unread_token(std::move(peek2));
            auto trans_res = parse_transformation(input_file, scene);
            if (!trans_res.has_value()) return std::unexpected(trans_res.error());

            auto close_res = expect_symbol(input_file, ')');
            if (!close_res.has_value()) return std::unexpected(close_res.error());

            if (keyword == KeywordEnum::ORTHOGONAL) return std::make_unique<OrthogonalCamera>(aspect_ratio, trans_res.value());
            else return std::make_unique<PerspectiveCamera>(aspect_ratio, distance, trans_res.value());
        }

        // It wasn't a transformation, so it must be the aspect ratio
        input_file.unread_token(std::move(peek2));
        auto ar_res = expect_number(input_file, scene);
        if (!ar_res.has_value()) { return std::unexpected(ar_res.error()); }
        aspect_ratio = ar_res.value();

        // Peek at the next token
        auto tok2_res = input_file.read_token();
        if (!tok2_res.has_value()) { return std::unexpected(tok2_res.error()); }
        std::unique_ptr<Token> tok2 = std::move(tok2_res.value());
        auto* sym2 = dynamic_cast<SymbolToken*>(tok2.get());

        if (sym2 != nullptr && sym2->symbol == ')') {
            if (keyword == KeywordEnum::ORTHOGONAL) return std::make_unique<OrthogonalCamera>(aspect_ratio);
            else return std::make_unique<PerspectiveCamera>(aspect_ratio);
        }
        if (sym2 == nullptr || sym2->symbol != ',') {
            return std::unexpected(GrammarError{tok2->location, "Expected ',' or ')' after aspect ratio."});
        }

        if (keyword == KeywordEnum::ORTHOGONAL) {
            // Orthogonal doesn't have distance, so this MUST be a transformation
            auto trans_res = parse_transformation(input_file, scene);
            if (!trans_res.has_value()) return std::unexpected(trans_res.error());

            auto close_res = expect_symbol(input_file, ')');
            if (!close_res.has_value()) return std::unexpected(close_res.error());

            return std::make_unique<OrthogonalCamera>(aspect_ratio, trans_res.value());
        }
        else {
            // Perspective: is it distance or transformation?
            auto peek3_res = input_file.read_token();
            if (!peek3_res.has_value()) { return std::unexpected(peek3_res.error()); }
            std::unique_ptr<Token> peek3 = std::move(peek3_res.value());
            auto* kw3 = dynamic_cast<KeywordToken*>(peek3.get());

            if (kw3 != nullptr && is_transformation_keyword(kw3->keyword)) {
                // User skipped distance and went straight to transformation
                input_file.unread_token(std::move(peek3));
                auto trans_res = parse_transformation(input_file, scene);
                if (!trans_res.has_value()) return std::unexpected(trans_res.error());

                auto close_res = expect_symbol(input_file, ')');
                if (!close_res.has_value()) return std::unexpected(close_res.error());

                // distance is the default value
                return std::make_unique<PerspectiveCamera>(aspect_ratio, distance, trans_res.value());
            }

            // Parse the distance
            input_file.unread_token(std::move(peek3));
            auto dist_res = expect_number(input_file, scene);
            if (!dist_res.has_value()) { return std::unexpected(dist_res.error()); }
            distance = dist_res.value();

            // Peek at the next token
            auto tok3_res = input_file.read_token();
            if (!tok3_res.has_value()) { return std::unexpected(tok3_res.error()); }
            std::unique_ptr<Token> tok3 = std::move(tok3_res.value());
            auto* sym3 = dynamic_cast<SymbolToken*>(tok3.get());

            if (sym3 != nullptr && sym3->symbol == ')') {
                return std::make_unique<PerspectiveCamera>(aspect_ratio, distance);
            }
            if (sym3 == nullptr || sym3->symbol != ',') {
                return std::unexpected(GrammarError{tok3->location, "Expected ',' or ')' after distance."});
            }

            // Parse the transformation
            auto trans_res = parse_transformation(input_file, scene);
            if (!trans_res.has_value()) return std::unexpected(trans_res.error());

            auto close_res = expect_symbol(input_file, ')');
            if (!close_res.has_value()) return std::unexpected(close_res.error());

            return std::make_unique<PerspectiveCamera>(aspect_ratio, distance, trans_res.value());
        }
    }


    /// @brief Type alias for a function pointer to a template instantiation of parse_shape<T>.
    using ShapeParserFunc = std::expected<std::unique_ptr<Shape>, GrammarError>(*)(InputStream&, Scene&);

    /// @brief Static lookup table linking the keyword to the correct template instantiation of `parse_shape`.
    inline const std::unordered_map<KeywordEnum, ShapeParserFunc> SHAPE_PARSERS = {
        {KeywordEnum::SPHERE, &parse_shape<Sphere>},
        {KeywordEnum::PLANE,  &parse_shape<Plane>},
        {KeywordEnum::CUBE,   &parse_shape<Cube>}
        // To add a cylinder later, just add one line here:
        // {KeywordEnum::CYLINDER, &parse_shape<Cylinder>}
    };

    /// @brief Parses an entire scene file from top to bottom, building the World, Camera, and Materials.
    ///
    /// This function acts as the root of the recursive descent parser. It sequentially reads
    /// top-level tokens (Float variables, Materials, Cameras, and Shapes) and populates a Scene object.
    ///
    /// @param input_file The InputStream pointing to the .txt scene file.
    /// @param overridden_variables An optional dictionary of float variables injected from the command line.
    /// @return A fully populated Scene object, or a GrammarError if a syntax violation occurs.
    std::expected<Scene, GrammarError> parse_scene(InputStream& input_file,
                                                   const std::unordered_map<std::string, float>& overridden_variables = {}) {

        Scene scene;

        // Initialize the scene's float variables with the overrides provided (e.g., from command line)
        scene.float_variables = overridden_variables;

        // Populate the overridden_variables set so we know which ones to protect from redefinition
        for (const auto& [key, value] : overridden_variables) {
            scene.overridden_variables.insert(key);
        }

        auto token_res = input_file.read_token();
        if (!token_res.has_value()) { return std::unexpected(token_res.error()); }
        std::unique_ptr<Token> token = std::move(token_res.value());

        // Parse top-level statements until the stream reaches EOF
        while (dynamic_cast<StopToken*>(token.get()) == nullptr) {
            // First token should always be a keyword
            auto* kw_token = dynamic_cast<KeywordToken*>(token.get());
            if (kw_token == nullptr) { // If what the read token is not a keyword throw an error message
                return std::unexpected(GrammarError{token->location, "Expected a keyword token."});
            }
            KeywordEnum kw = kw_token->keyword;

            if (kw == KeywordEnum::FLOAT) {

                // Float variables get to be defined as
                // float var_name = value;

                auto variable_name_res = expect_identifier(input_file);
                if (!variable_name_res.has_value()) { return std::unexpected(variable_name_res.error()); }
                std::string variable_name = variable_name_res.value();
                SourceLocation variable_location = input_file.location;

                auto assign_res = expect_symbol(input_file, '=');
                if (!assign_res.has_value()) { return std::unexpected(assign_res.error()); }

                auto val_res = expect_number(input_file, scene);
                if (!val_res.has_value()) { return std::unexpected(val_res.error()); }
                float val = val_res.value();

                // Check for illegal redefinition
                if (scene.float_variables.contains(variable_name) && !scene.overridden_variables.contains(variable_name)) {
                    return std::unexpected(GrammarError{
                        variable_location,
                        std::format("Variable '{}' cannot be redefined.", variable_name)
                    });
                }

                auto semicolon_res = expect_symbol(input_file, ';');
                if (!semicolon_res.has_value()) { return std::unexpected(semicolon_res.error()); }

                // Define the variable if it wasn't overridden from the command line
                if (!scene.overridden_variables.contains(variable_name)) {
                    scene.float_variables[variable_name] = val;
                }

            } else if (kw == KeywordEnum::MATERIAL) {

                // Materials get to be defined as
                // material material_name(brdf, emitted_radiance);

                auto material_res = parse_material(input_file, scene);
                if (!material_res.has_value()) { return std::unexpected(material_res.error()); }

                std::string material_name = material_res.value().first;
                Material material = std::move(material_res.value().second);

                scene.materials[material_name] = std::make_shared<Material>(std::move(material));

                auto semicolon_res = expect_symbol(input_file, ';');
                if (!semicolon_res.has_value()) { return std::unexpected(semicolon_res.error()); }

            } else if (kw == KeywordEnum::CAMERA) {

                if (scene.camera) {
                    return std::unexpected(GrammarError{token->location, "You cannot define more than one Camera", });
                }
                auto camera_res = parse_camera(input_file, scene);
                if (!camera_res.has_value()) { return std::unexpected(camera_res.error()); }

                scene.camera = std::move(camera_res.value());

                auto semicolon_res = expect_symbol(input_file, ';');
                if (!semicolon_res.has_value()) { return std::unexpected(semicolon_res.error()); }

            } else if (SHAPE_PARSERS.contains(kw)) {

                // SHAPE_PARSER.at(kw) is just the template instantiation parse_shape function with the shape indicated by the shape keyword
                auto shape_res = SHAPE_PARSERS.at(kw)(input_file, scene);
                if (!shape_res.has_value()) { return std::unexpected(shape_res.error()); }

                scene.world.add(std::move(shape_res.value()));

                auto semicolon_res = expect_symbol(input_file, ';');
                if (!semicolon_res.has_value()) { return std::unexpected(semicolon_res.error()); }

            } else {
                // We know it is a keyword, but it's the wrong keyword for the top level
                // For instance you cannot define uniform() by itself, it has to be inside a material or shape

                std::string kw_str = "unknown";
                for (const auto& [key_string, enum_val] : KEYWORDS) {
                    if (enum_val == kw) {
                        kw_str = key_string;
                        break;
                    }
                }

                return std::unexpected(GrammarError{
                    token->location,
                    std::format("Keyword '{}' is not allowed at the top level of the scene file.", kw_str)
                });

            }

            // At the very end of the while loop read the next token to update the pointer that controls the while loop
            token_res = input_file.read_token();
            if (!token_res.has_value()) { return std::unexpected(token_res.error()); }
            token = std::move(token_res.value());

        }

        return scene;
    }

}