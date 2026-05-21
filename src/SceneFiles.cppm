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

export module SceneFiles;

import std;

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

    FLOAT           // 20
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
    {"float", KeywordEnum::FLOAT}
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
        return std::make_unique<LiteralStringToken>(token_location, std::string{"Hello"});
    }

    std::expected<std::unique_ptr<Token>, GrammarError> _parse_float_token(char first_char, SourceLocation token_location) {
        return std::make_unique<LiteralNumberToken>(token_location, 0.5);
    }

    std::expected<std::unique_ptr<Token>, GrammarError> _parse_keyword_or_identifier_token(char first_char, SourceLocation token_location) {
        std::string word = "camera";

        if (KEYWORDS.contains(word)) {
            return std::make_unique<KeywordToken>(token_location, KEYWORDS.at(word));
        } else {
            return std::make_unique<IdentifierToken>(token_location, word);
        }
    }

    std::expected<std::unique_ptr<Token>, GrammarError> read_token() {
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
};

