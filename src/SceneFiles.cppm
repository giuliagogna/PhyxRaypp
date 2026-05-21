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

export struct SourceLocation {
    std::string filename = "";
    int line_num = 0;
    int col_num = 0;
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
};

