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
import CSG;
import auxiliary_functions;

/// A string view containing all single-character symbols recognized by the language lexer.
export constexpr std::string_view SYMBOLS = "()<>[],*=;";

/**
 * @brief Represents a position inside a scene source file.
 *
 * Used by the lexer and parser to report precise error locations.
 *
 * Line and column numbers start from 1.
 *
 * If the source is not associated with a physical file, filename may be empty.
 */
export struct SourceLocation {
    std::string filename = "";
    int line_num = 0;
    int col_num = 0;
};

/// Enumeration of all reserved keywords in the scene language.
export enum class KeywordEnum {
    NEW,
    MATERIAL,

    // Shapes
    PLANE,
    SPHERE,
    CUBE,
    CYLINDER,
    MESH,
    // CSG Shapes
    CSG_UNION,
    CSG_INTERSECTION,
    CSG_DIFFERENCE,

    // BRDFs
    DIFFUSE,
    SPECULAR,

    // Pigments
    UNIFORM,
    CHECKERED,
    IMAGE,

    // Transformations
    IDENTITY,
    TRANSLATION,
    ROTATION_X,
    ROTATION_Y,
    ROTATION_Z,
    SCALING,

    // Camera
    CAMERA,
    ORTHOGONAL,
    PERSPECTIVE,

    // General
    FLOAT,
    BACKGROUND
};

/// Maps textual keywords appearing in a scene file to their corresponding KeywordEnum values.
export const std::unordered_map<std::string, KeywordEnum> KEYWORDS{
    {"new", KeywordEnum::NEW},
    {"material", KeywordEnum::MATERIAL},
    {"plane", KeywordEnum::PLANE},
    {"sphere", KeywordEnum::SPHERE},
    {"cube", KeywordEnum::CUBE},
    {"cylinder", KeywordEnum::CYLINDER},
    {"mesh", KeywordEnum::MESH},
    {"csg_union", KeywordEnum::CSG_UNION},
    {"csg_intersection", KeywordEnum::CSG_INTERSECTION},
    {"csg_difference", KeywordEnum::CSG_DIFFERENCE},
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
    {"background", KeywordEnum::BACKGROUND}
};

/**
 * @brief Base class for all lexical tokens.
 *
 * Every token stores the source location from which it was read.
 */
export struct Token {
    SourceLocation location;
    Token(SourceLocation loc) : location{loc} {}

    virtual ~Token() = default;
};

/// Token signaling the end of the input stream (EOF).
export struct StopToken : Token {
    StopToken(SourceLocation location) : Token{location} {}
};

/// Token containing a user-defined identifier (e.g., custom variable or material names).
export struct IdentifierToken : Token {
    std::string identifier;
    IdentifierToken(SourceLocation location, std::string id) : Token{location}, identifier{id} {}

    std::string get(){
        return identifier;
    }
};

/// Token containing a reserved language keyword.
export struct KeywordToken : Token {
    KeywordEnum keyword;
    KeywordToken(SourceLocation location, KeywordEnum kw) : Token{location}, keyword{kw} {}

    KeywordEnum get() {
        return keyword;
    }
};

/// Token containing a syntax symbol (e.g., brackets, commas, equals signs).
export struct SymbolToken : Token {
    char symbol;
    SymbolToken(SourceLocation location, char symbol) : Token{location}, symbol{symbol} {}

    char get(){
        return symbol;
    }
};

/// @brief Token containing a literal floating-point number.
/// @note: other numerical types are not supported
export struct LiteralNumberToken : Token {
    float number;
    LiteralNumberToken(SourceLocation location, float num) : Token{location}, number{num} {}

    float get() {
        return number;
    }
};

/// Token containing a literal string enclosed in quotes "" (e.g., file paths).
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

/**
 * @brief Stream wrapper used by the lexer and parser.
 *
 * Besides reading characters from an std::istream, this class:
 *
 * - tracks line and column numbers,
 * - supports one-character pushback,
 * - supports one-token lookahead,
 * - provides source locations for diagnostics.
 *
 * This functionality is required by the recursive-descent parser.
 */
export struct InputStream {
    /// Underlying source stream.
    std::istream& ifs;

    /// Current reading position in the source.
    SourceLocation location;

    /// Character buffered by unread_char(), if any.
    std::optional<char> saved_char;

    /// Source location associated with the buffered character.
    SourceLocation saved_location;

    /// Token returned by unread_token() for LL(1) parser lookahead, if any.
    std::optional<std::unique_ptr<Token>> saved_token;

    /// Number of columns advanced when a tab character is encountered.
    int tabulations;

    /**
     * @brief Construct an input stream wrapper.
     *
     * Initializes source location tracking and lookahead buffers.
     * Line and column numbering start from 1.
     *
     * @param ifs Input stream from which characters are read.
     * @param filename Optional source filename used in diagnostics.
     * @param tabulations Number of columns advanced when a tab
     *        character ('\t') is encountered. Defaults to 8.
     */
    InputStream(std::istream& ifs, const std::string& filename="", int tabulations=8) :
        ifs(ifs),
        location{filename, 1, 1}, // initialize line and column to 1
        tabulations(tabulations),
        saved_location{filename, 1, 1}  // keep unread state consistent with initial position
    {}


    /**
     * @brief Updates the current source location after consuming a character.
     *
     * Newlines advance the line counter and reset the column number.
     * Tabs advance by the configured tab width.
     * All other characters advance the column by one.
     */
    void _update_location(std::optional<char> ch) {
        if (!ch.has_value()) {
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

    /**
     * @brief Reads the next character from the input.
     *
     * Characters previously pushed back with unread_char()
     * are returned before reading from the underlying stream.
     *
     * The source location is updated automatically.
     *
     * @return The next character, or std::nullopt at end-of-file.
     */
    std::optional<char> read_char() {
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

    /**
     * @brief Pushes a character back into the input stream.
     *
     * The next call to read_char() will return this character instead of reading from the underlying stream.
     *
     * The source location is restored to the position that preceded the original read operation.
     *
     * Passing std::nullopt has no effect.
     *
     * @param ch Character to push back.
     *
     * @warning Only one character of pushback is supported.
     *          Calling this while another character is already
     *          buffered is a programmer error and triggers an assert.
     */
    void unread_char(std::optional<char> ch) {
        if (ch.has_value()) {
            assert(!saved_char.has_value());
            saved_char = ch;
            location = saved_location;
        } else {
            return;
        }
    }

    /**
     * @brief Skips whitespace and comment lines.
     *
     * Consumes spaces, tabs, newlines, carriage returns, and comments starting with '#'.
     *
     * When the function returns, the next call to read_char() will yield the first non-whitespace, non-comment
     * character in the stream.
     */
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

    /**
     * @brief Parses a string literal token.
     *
     * Assumes the opening quotation mark has already been consumed by the caller.
     *
     * Characters are collected until the matching closing quotation mark is found.
     *
     * @param token_location Source location of the opening quote.
     *
     * @return A LiteralStringToken containing the extracted text,
     *         or a GrammarError if the string is not terminated before end-of-file.
     */
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

    /**
     * @brief Parses a floating-point literal.
     *
     * Assumes the first character of the number has already been consumed by the caller.
     *
     * Characters compatible with scientific notation are accumulated and then converted using std::strtof().
     *
     * Parsing stops at the first character that cannot belong to a floating-point literal.
     * That character is pushed back into the stream for subsequent parsing.
     *
     * @param first_char First character of the numeric literal.
     * @param token_location Source location of the first character.
     *
     * @return A LiteralNumberToken containing the parsed value,
     *         or a GrammarError if the number is malformed or
     *         outside the representable float range.
     */
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

    /**
     * @brief Parses either a keyword or an identifier.
     *
     * Assumes the first character has already been consumed.
     *
     * Alphanumeric characters and underscores are collected until a non-identifier character is encountered.
     * The terminating character is pushed back into the stream.
     *
     * The resulting lexeme is compared against the language keyword table. If a match is found a KeywordToken is
     * produced; otherwise an IdentifierToken is returned.
     *
     * @param first_char First character of the lexeme.
     * @param token_location Source location of the first character.
     *
     * @return A KeywordToken or IdentifierToken.
     */
    std::unique_ptr<Token> _parse_keyword_or_identifier_token(char first_char, SourceLocation token_location) {
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
    // Reader functions
    // ==========================================================================================

    /**
     * @brief Reads the next lexical token from the input stream.
     *
     * Leading whitespace and comments are skipped automatically.
     * The function recognizes:
     *
     * - symbols (e.g. '(', ')', ',', '=')
     * - string literals enclosed in double quotes ""
     * - floating-point literals
     * - identifiers
     * - reserved keywords
     * - end-of-file markers
     *
     * If a token was previously returned to the stream through unread_token(),
     * that token is returned instead of reading new characters from the underlying stream.
     *
     * @return A parsed Token on success, or a GrammarError if an
     *         invalid character or malformed token is encountered.
     */
    std::expected<std::unique_ptr<Token>, GrammarError> read_token() {

        if (saved_token.has_value()) {
            // Return the token stored by unread_token() without reading from the stream.
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

    /**
     * @brief Pushes a token back into the stream.
     *
     * The next call to read_token() will return this token
     * instead of lexing a new one from the underlying stream.
     *
     * Only one token can be buffered at a time.
     *
     * @param token Ownership of the token to be returned.
     *
     * @warning Only one token of pushback is supported.
     *          Calling this function when another token is already
     *          buffered is a programmer error and triggers an assert.
     */
    void unread_token (std::unique_ptr<Token> token){
        assert(!saved_token.has_value());
        saved_token = std::move(token);
    }
};


/**
 * @brief Represents a fully parsed scene ready for rendering.
 *
 * A Scene contains all data extracted from a scene description file,
 * including geometry, materials, camera configuration, user-defined
 * variables, and rendering parameters.
 *
 * The parser progressively fills the World field while processing the scene file.
 */
export struct Scene {

    /// Materials declared in the scene, indexed by their symbolic name.
    std::unordered_map<std::string, std::shared_ptr<Material>> materials;

    /// Collection of all geometric objects composing the scene.
    World world;

    /// Active camera used to generate primary rays.
    /// May be null until a camera declaration is parsed.
    std::unique_ptr<Camera> camera = nullptr;

    /// User-defined floating-point variables available during parsing.
    std::unordered_map<std::string, float> float_variables;

    /// Names of variables whose values were overridden from the command line.
    /// These variables cannot be redefined by assignments inside the scene file.
    std::unordered_set<std::string> overridden_variables;

    /// Environment color returned when a ray leaves the scene without
    /// intersecting any object.
    Color background_color{0.0f, 0.0f, 0.0f};
};


export {

    // ============================================================================================
    // EXPECT FUNCTIONS
    // ============================================================================================

    /**
     * @brief Reads the next token and verifies that it is the expected symbol.
     *
     * This helper is typically used to validate punctuation required by
     * the grammar, such as parentheses, commas, assignment operators,
     * or angle brackets.
     *
     * @param input_file Stream from which the next token is read.
     * @param symbol The symbol expected by the grammar rule.
     *
     * @return Success if the next token is a SymbolToken matching
     *         the requested character; otherwise a GrammarError.
     */
    std::expected<void, GrammarError> expect_symbol(InputStream& input_file, char symbol) {

        auto res = input_file.read_token();
        if (!res.has_value()) {
            return std::unexpected(res.error());
        }
        std::unique_ptr<Token> token = std::move(res.value());

        auto* sym_token = dynamic_cast<SymbolToken*>(token.get());

        // If what comes next is not a Symbol token, throw the GrammarError
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

    /**
     * @brief Reads the next token and verifies that it is one of the
     *        expected keywords.
     *
     * This helper is useful when a grammar rule accepts multiple
     * alternatives, such as different shape types, pigments,
     * transformations, or camera models.
     *
     * @param input_file Stream from which the next token is read.
     * @param expected_keywords Set of keywords accepted by the current
     *        grammar rule.
     *
     * @return The keyword that was found, or a GrammarError if the next
     *         token is not a keyword or does not belong to the allowed set.
     */
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

    /**
     * @brief Reads the next token and verifies that it is an identifier.
     *
     * Identifiers are user-defined names appearing in the scene file,
     * such as material names or variable names.
     *
     * @param input_file Stream from which the next token is read.
     *
     * @return The identifier string, or a GrammarError if the next
     *         token is not an IdentifierToken.
     */
    std::expected<std::string, GrammarError> expect_identifier(InputStream& input_file) {

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


    /**
     * @brief Reads the next token and verifies that it is a string literal.
     *
     * String literals are enclosed in double quotes "" and are typically
     * used for file paths or external resource names.
     *
     * @param input_file Stream from which the next token is read.
     *
     * @return The string value without quotation marks, or a
     *         GrammarError if the next token is not a LiteralStringToken.
     */
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

    /**
     * @brief Reads the next token and resolves it to a floating-point value.
     *
     * The token may be either:
     * - a numeric literal directly appearing in the scene file, or
     * - an identifier referring to a previously defined float variable.
     *
     * If an identifier is encountered, its value is looked up in the
     * scene's variable table.
     *
     * @param input_file Stream from which the next token is read.
     * @param scene Scene containing the currently defined float variables.
     *
     * @return The resolved floating-point value, or a GrammarError if
     *         the token is neither a number nor a valid variable name.
     */
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

    /**
     * @brief Parse a 3D vector from the scene description language.
     *
     * Expected grammar:
     *
     *     [ number , number , number ]
     *
     * where each component may be either a numeric literal or a previously defined float variable.
     *
     * @param input_file Source stream currently positioned at '['.
     * @param scene Scene containing variable definitions.
     *
     * @return The parsed Vec object, or a GrammarError if the
     *         syntax is invalid.
     */
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

    /**
     * @brief Parse an RGB color from the scene description language.
     *
     * Expected grammar:
     *
     *     < number , number , number >
     *
     * where each component may be either a numeric literal or a previously defined float variable.
     *
     * @param input_file Source stream currently positioned at '<'.
     * @param scene Scene containing variable definitions.
     *
     * @return The parsed Color object, or a GrammarError if the
     *         syntax is invalid.
     */
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

    /**
     * @brief Parse a pigment definition.
     *
     * Supported grammar:
     *
     *     uniform(<r,g,b>)
     *     checkered(<r,g,b>, <r,g,b>, subdivisions)
     *     image("filename.pfm")
     *
     * Depending on the keyword, constructs the corresponding Pigment-derived object.
     *
     * Image pigments load the referenced PFM image immediately.
     *
     * @param input_file Source stream positioned at a pigment keyword.
     * @param scene Scene context used for variable lookup.
     *
     * @return A polymorphic Pigment instance, or a GrammarError
     *         if the syntax is invalid or an image cannot be loaded.
     */
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

    /**
     * @brief Parse a BRDF definition.
     *
     * Supported grammar:
     *
     *     diffuse(pigment)
     *     specular(pigment)
     *
     * where pigment is any valid pigment definition accepted by parse_pigment().
     *
     * @param input_file Source stream positioned at a BRDF keyword.
     * @param scene Scene context used for variable lookup.
     *
     * @return A polymorphic BRDF instance, or a GrammarError
     *         if the syntax is invalid.
     */
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

    /**
     * @brief Parse a material definition.
     *
     * Expected grammar:
     *
     *     identifier ( brdf [, emitted_radiance] )
     *
     * The BRDF definition is mandatory. The emitted radiance pigment
     * is optional; if omitted, the material is created with the default emitted radiance.
     *
     * Example:
     *
     *     my_mat(diffuse(uniform(<1,0,0>)))
     *
     *     lamp(diffuse(uniform(<1,1,1>)),
     *          uniform(<10,10,10>))
     *
     * @param input_file Input stream containing the material definition.
     * @param scene Scene currently being constructed. Used to resolve
     *              variables and nested scene elements.
     *
     * @return A pair containing:
     *         - the material identifier
     *         - the constructed Material object
     *
     * @retval std::unexpected If the material syntax is invalid or any
     *         nested element (BRDF or pigment) cannot be parsed.
     */
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

    /**
     * @brief Parse a chain of affine transformations.
     *
     * Expected grammar:
     *
     *     transformation ::= elementary_transformation
     *                      | elementary_transformation
     *                        * elementary_transformation
     *                        * ...
     *
     * Supported elementary transformations:
     *
     *     identity
     *     translation([x,y,z])
     *     scaling([x,y,z])
     *     rot_x(angle)
     *     rot_y(angle)
     *     rot_z(angle)
     *
     * Consecutive transformations are combined through matrix multiplication in the
     * same order in which they appear in the scene description.
     *
     * The parser uses one-token lookahead to determine whether additional transformations follow in the chain.
     *
     * Example:
     *
     *     translation([1,0,0])
     *     * rot_y(45)
     *     * scaling([2,2,2])
     *
     * @param input_file Input stream containing the transformation chain.
     * @param scene Scene currently being constructed. Used to resolve
     *              numerical variables appearing in parameters.
     *
     * @return The composed Transformation.
     *
     * @retval std::unexpected If the transformation syntax is invalid
     *         or one of the parameters cannot be parsed.
     */
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
                // The chain is finished. Put the token back and toggle the flag to false.
                input_file.unread_token(std::move(next_token));
                has_next_transformation = false;
            }
        }

        return result;

    }

    /**
     * @brief Parse a geometric shape instance and bind it to a material.
     *
     * Expected grammar:
     *
     *     shape ::= '(' material_name ')'
     *             | '(' transformation ',' material_name ')'
     *
     * The transformation is optional. If omitted, the identity transformation is used.
     *
     * The material identifier must refer to a material that has already been defined in the scene.
     *
     * Examples:
     *
     *     sphere(red_material)
     *
     *     sphere(
     *         translation([1,0,0]) * rot_y(45),
     *         red_material
     *     )
     *
     * @tparam T Concrete Shape type to instantiate
     *           (e.g. Sphere, Plane, Cube).
     *
     * @param input_file Input stream containing the shape definition.
     * @param scene Scene currently being constructed. Used to resolve
     *              material references.
     *
     * @return A polymorphic Shape instance of type T.
     *
     * @retval std::unexpected If the shape syntax is invalid or the
     *         referenced material does not exist.
     */
    template <typename T>
    std::expected<std::unique_ptr<Shape>, GrammarError> parse_shape(InputStream& input_file, Scene& scene) {

        auto open_brk_res = expect_symbol(input_file, '(');
        if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

        Transformation transformation{}; // Default identity
        std::string material_name;

        // Peek at the next token to see if it's an identifier or a transformation keyword
        auto next_tok_res = input_file.read_token();
        if (!next_tok_res.has_value()) { return std::unexpected(next_tok_res.error()); }
        std::unique_ptr<Token> tok = std::move(next_tok_res.value());

        if (auto* id_tok = dynamic_cast<IdentifierToken*>(tok.get())) {
            // User skipped transformation and just provided the material name
            material_name = id_tok->identifier;
        } else {
            // It's not an identifier, so it's a transformation: put back the token
            input_file.unread_token(std::move(tok));

            auto transformation_res = parse_transformation(input_file, scene);
            if (!transformation_res.has_value()) { return std::unexpected(transformation_res.error()); }
            transformation = transformation_res.value();

            auto comma_res = expect_symbol(input_file, ',');
            if (!comma_res.has_value()) { return std::unexpected(comma_res.error()); }

            auto mat_name_res = expect_identifier(input_file);
            if (!mat_name_res.has_value()) { return std::unexpected(mat_name_res.error()); }
            material_name = mat_name_res.value();

        }

        // Link material
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

    /**
     * @brief Parse a mesh loaded from an external OBJ file.
     *
     * Expected grammar:
     *
     *     mesh(
     *         "filename.obj",
     *         material_name,
     *         transformation
     *     )
     *
     * Optional BVH parameters may also be provided:
     *
     *     mesh(
     *         "filename.obj",
     *         material_name,
     *         transformation,
     *         n_bins
     *     )
     *
     *     mesh(
     *         "filename.obj",
     *         material_name,
     *         transformation,
     *         n_bins,
     *         leaf_threshold
     *     )
     *
     * If BVH parameters are omitted, the Mesh constructor applies its default values.
     *
     * Before constructing the mesh, the parser verifies that:
     *
     *  - the OBJ file can be opened;
     *  - the referenced material exists in the scene.
     *
     * @param input_file Input stream containing the mesh definition.
     * @param scene Scene currently being constructed.
     *
     * @return A Mesh instance safely upcast to Shape.
     *
     * @retval std::unexpected If:
     *         - the mesh syntax is invalid;
     *         - the OBJ file cannot be opened;
     *         - the referenced material does not exist.
     */
    std::expected<std::unique_ptr<Shape>, GrammarError> parse_mesh(InputStream& input_file, Scene& scene) {

        auto open_brk_res = expect_symbol(input_file, '(');
        if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

        auto file_res = expect_string(input_file);
        if (!file_res.has_value()) { return std::unexpected(file_res.error()); }
        std::string filename = file_res.value();

        // Safety Check: Prevent the constructor from throwing an uncatchable exception
        auto open_file_res = aux::open_input_file(filename);
        if (!open_file_res.has_value()) {
            return std::unexpected(GrammarError{
                input_file.location,
                open_file_res.error()
            });
        }

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
                // A 5th argument was provided (threshold)
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

    /**
     * @brief Checks whether a keyword can start a transformation expression.
     *
     * Used during LL(1) lookahead parsing to distinguish optional transformation arguments
     * from numerical camera parameters.
     *
     * @param kw Keyword to test.
     * @return true if the keyword belongs to the transformation family.
     */
    bool is_transformation_keyword(KeywordEnum kw) {
        return kw == KeywordEnum::IDENTITY ||
               kw == KeywordEnum::TRANSLATION ||
               kw == KeywordEnum::ROTATION_X ||
               kw == KeywordEnum::ROTATION_Y ||
               kw == KeywordEnum::ROTATION_Z ||
               kw == KeywordEnum::SCALING;
    }

    /**
     * @brief Parses a camera declaration.
     *
     * Supported syntax:
     *
     *   camera(orthogonal)
     *   camera(orthogonal, aspect_ratio)
     *   camera(orthogonal, aspect_ratio, transformation)
     *   camera(orthogonal, transformation)
     *
     *   camera(perspective)
     *   camera(perspective, aspect_ratio)
     *   camera(perspective, aspect_ratio, distance)
     *   camera(perspective, aspect_ratio, distance, transformation)
     *   camera(perspective, aspect_ratio, transformation)
     *   camera(perspective, transformation)
     *
     * Any omitted parameter is replaced with the corresponding camera class default value.
     *
     * The parser uses LL(1) lookahead to distinguish whether the next argument represents a
     * numerical parameter or the start of a transformation chain.
     *
     * @param input_file Input stream currently positioned at the
     *                   camera parameter list.
     * @param scene Scene context used for variable resolution.
     *
     * @return A dynamically allocated Camera instance wrapped in a std::unique_ptr,
     *         or a GrammarError if the syntax is invalid.
     */
    std::expected<std::unique_ptr<Camera>, GrammarError> parse_camera(InputStream& input_file, Scene& scene) {

        auto open_brk_res = expect_symbol(input_file, '(');
        if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

        auto keyword_res = expect_keywords(input_file, {KeywordEnum::PERSPECTIVE, KeywordEnum::ORTHOGONAL});
        if (!keyword_res.has_value()) { return std::unexpected(keyword_res.error()); }
        KeywordEnum keyword = keyword_res.value();

        // Query the structs for their exact defaults
        float aspect_ratio = PerspectiveCamera{}.aspect_ratio;
        float distance = PerspectiveCamera{}.d;
        Transformation transformation{}; // Transformation i always defaulted to identity

        // Peek at the next token
        auto tok1_res = input_file.read_token();
        if (!tok1_res.has_value()) { return std::unexpected(tok1_res.error()); }
        std::unique_ptr<Token> tok1 = std::move(tok1_res.value());
        auto* sym1 = dynamic_cast<SymbolToken*>(tok1.get());

        if (sym1 != nullptr && sym1->symbol == ')') {
            if (keyword == KeywordEnum::ORTHOGONAL) return std::make_unique<OrthogonalCamera>();
            else /*if (keyword == KeywordEnum::PERSPECTIVE)*/ return std::make_unique<PerspectiveCamera>();
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
            else /*if (keyword == KeywordEnum::PERSPECTIVE)*/ return std::make_unique<PerspectiveCamera>(aspect_ratio, distance, trans_res.value());
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
            else /*if (keyword == KeywordEnum::PERSPECTIVE)*/ return std::make_unique<PerspectiveCamera>(aspect_ratio);
        }
        if (sym2 == nullptr || sym2->symbol != ',') {
            return std::unexpected(GrammarError{tok2->location, "Expected ',' or ')' after aspect ratio."});
        }

        if (keyword == KeywordEnum::ORTHOGONAL) {
            // Orthogonal doesn't have distance, so this must be a transformation
            auto trans_res = parse_transformation(input_file, scene);
            if (!trans_res.has_value()) return std::unexpected(trans_res.error());

            auto close_res = expect_symbol(input_file, ')');
            if (!close_res.has_value()) return std::unexpected(close_res.error());

            return std::make_unique<OrthogonalCamera>(aspect_ratio, trans_res.value());
        }
        else /*if (keyword == KeywordEnum::PERSPECTIVE)*/ {
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


    // =========================================================================
    // CSG PARSING FUNCTIONS
    // =========================================================================

    // Forward declaration to allow recursive parsing of nested CSG nodes
    std::expected<std::unique_ptr<Shape>, GrammarError> parse_any_shape(InputStream& input_file, Scene& scene);

    /**
     * @brief Parses a Constructive Solid Geometry (CSG) operation node.
     *
     * Supported syntax:
     * csg_op(left_shape, right_shape)
     * csg_op(left_shape, right_shape, transformation)
     *
     * @param input_file Input stream containing scene definitions.
     * @param scene Current scene object being populated.
     * @param op CSG operation type (Union, Intersection, Difference).
     * @return std::unique_ptr<Shape> containing the constructed CSG node.
     */
    std::expected<std::unique_ptr<Shape>, GrammarError> parse_csg_operation(
        InputStream& input_file, Scene& scene, CSGOperations op) 
    {
        auto open_brk_res = expect_symbol(input_file, '(');
        if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

        // Parse left child shape
        auto left_res = parse_any_shape(input_file, scene);
        if (!left_res.has_value()) { return std::unexpected(left_res.error()); }

        auto comma1_res = expect_symbol(input_file, ',');
        if (!comma1_res.has_value()) { return std::unexpected(comma1_res.error()); }

        // Parse right child shape
        auto right_res = parse_any_shape(input_file, scene);
        if (!right_res.has_value()) { return std::unexpected(right_res.error()); }

        // Parse optional transformation or closing parenthesis
        Transformation transformation{}; // Default identity

        auto next_tok_res = input_file.read_token();
        if (!next_tok_res.has_value()) { return std::unexpected(next_tok_res.error()); }
        std::unique_ptr<Token> tok = std::move(next_tok_res.value());

        if (auto* sym_tok = dynamic_cast<SymbolToken*>(tok.get()); sym_tok && sym_tok->symbol == ',') {
            auto trans_res = parse_transformation(input_file, scene);
            if (!trans_res.has_value()) { return std::unexpected(trans_res.error()); }
            transformation = trans_res.value();

            auto close_brk_res = expect_symbol(input_file, ')');
            if (!close_brk_res.has_value()) { return std::unexpected(close_brk_res.error()); }

        } else if (auto* sym_tok = dynamic_cast<SymbolToken*>(tok.get()); sym_tok && sym_tok->symbol == ')') {
            // No transformation provided, regular closing
        } else {
            return std::unexpected(GrammarError{
                tok->location,
                "Expected ',' or ')' in CSG operation definition."
            });
        }

        auto csg_shape = std::make_unique<CSG>(std::move(left_res.value()), std::move(right_res.value()), op);
        csg_shape->trans = transformation;
        return csg_shape;
    }

    /// Helper parser functions for specific CSG operations.
    inline std::expected<std::unique_ptr<Shape>, GrammarError> parse_csg_union(InputStream& input_file, Scene& scene) {
        return parse_csg_operation(input_file, scene, CSGOperations::Union);
    }

    inline std::expected<std::unique_ptr<Shape>, GrammarError> parse_csg_intersection(InputStream& input_file, Scene& scene) {
        return parse_csg_operation(input_file, scene, CSGOperations::Intersection);
    }

    inline std::expected<std::unique_ptr<Shape>, GrammarError> parse_csg_difference(InputStream& input_file, Scene& scene) {
        return parse_csg_operation(input_file, scene, CSGOperations::Difference);
    }

    /// Type alias for a function pointer to a template instantiation of parse_shape<T>.
    using ShapeParserFunc = std::expected<std::unique_ptr<Shape>, GrammarError>(*)(InputStream&, Scene&);

    /// Maps shape-related keywords to the parser responsible for constructing the corresponding Shape instance.
    inline const std::unordered_map<KeywordEnum, ShapeParserFunc> SHAPE_PARSERS = {
        {KeywordEnum::SPHERE,           &parse_shape<Sphere>},
        {KeywordEnum::PLANE,            &parse_shape<Plane>},
        {KeywordEnum::CUBE,             &parse_shape<Cube>},
        {KeywordEnum::CYLINDER,         &parse_shape<Cylinder>},
        {KeywordEnum::MESH,             &parse_mesh},
        {KeywordEnum::CSG_UNION,        &parse_csg_union},
        {KeywordEnum::CSG_INTERSECTION, &parse_csg_intersection},
        {KeywordEnum::CSG_DIFFERENCE,   &parse_csg_difference}
    };

    /**
     * @brief Parses any valid shape keyword (primitives or CSG nodes), allowing for parsing the entire shape hierarchy.
     * 
     * @param input_file Input stream containing shape definition.
     * @param scene Current scene object being populated.
     * @return std::unique_ptr<Shape> containing the parsed shape object.
     */
    std::expected<std::unique_ptr<Shape>, GrammarError> parse_any_shape(InputStream& input_file, Scene& scene) {
        auto kw_res = expect_keywords(input_file, {
            KeywordEnum::SPHERE, 
            KeywordEnum::PLANE, 
            KeywordEnum::CUBE,
            KeywordEnum::CYLINDER, 
            KeywordEnum::MESH,
            KeywordEnum::CSG_UNION, 
            KeywordEnum::CSG_INTERSECTION, 
            KeywordEnum::CSG_DIFFERENCE
        });
        if (!kw_res.has_value()) { return std::unexpected(kw_res.error()); }

        return SHAPE_PARSERS.at(kw_res.value())(input_file, scene);
    }

    /**
     * @brief Parses a complete scene description file.
     *
     * This is the root entry point of the recursive-descent parser.
     * It repeatedly reads top-level declarations until the end of the input stream is reached
     * and constructs the corresponding Scene object.
     *
     * Supported top-level declarations are:
     *
     *   float
     *   material
     *   camera
     *   background
     *   sphere
     *   plane
     *   cube
     *   cylinder
     *   mesh
     *   csg_union
     *   csg_intersection
     *   csg_difference
     *
     * Declarations may appear in any order, subject to the following
     * constraints:
     *
     *  - Materials must be defined before they are referenced by shapes or meshes.
     *  - Float variables must be defined before they are used.
     *  - At most one camera may be defined.
     *  - Command-line overridden variables cannot be redefined inside the scene file.
     *
     * The parser performs semantic validation while parsing, including material lookup,
     * variable lookup, duplicate definitions, and top-level grammar checks.
     *
     * @param input_file Input stream containing the scene description.
     * @param overridden_variables Optional dictionary of variables
     *        supplied externally (e.g. from command-line arguments).
     *
     * @return A fully populated Scene object on success, or a
     *         GrammarError describing the first parsing or semantic
     *         error encountered.
     */
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
                    return std::unexpected(GrammarError{token->location, "You cannot define more than one Camera"});
                }
                auto camera_res = parse_camera(input_file, scene);
                if (!camera_res.has_value()) { return std::unexpected(camera_res.error()); }

                scene.camera = std::move(camera_res.value());

                auto semicolon_res = expect_symbol(input_file, ';');
                if (!semicolon_res.has_value()) { return std::unexpected(semicolon_res.error()); }

            } else if (kw == KeywordEnum::BACKGROUND) {
                // Backgrounds get defined as:
                // background(<r, g, b>);

                auto open_brk_res = expect_symbol(input_file, '(');
                if (!open_brk_res.has_value()) { return std::unexpected(open_brk_res.error()); }

                auto bkg_color_res = parse_color(input_file, scene);
                if (!bkg_color_res.has_value()) { return std::unexpected(bkg_color_res.error()); }
                scene.background_color = bkg_color_res.value();

                auto close_res = expect_symbol(input_file, ')');
                if (!close_res.has_value()) { return std::unexpected(close_res.error()); }

                auto semicolon_res = expect_symbol(input_file, ';');
                if (!semicolon_res.has_value()) { return std::unexpected(semicolon_res.error()); }

            } else if (SHAPE_PARSERS.contains(kw)) {

                // Each shape has its dedicated parser function, which is responsible for parsing the shape's parameters and returning a Shape instance.
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