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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import std;
import SceneFiles;

TEST_CASE("Test InputStream") {

    std::istringstream string_stream("abc   \nd\nef");
    InputStream stream(string_stream);

    CHECK(stream.location.line_num == 1);
    CHECK(stream.location.col_num == 1);

    auto ch = stream.read_char();
    REQUIRE(ch.has_value());
    CHECK(ch.value() == 'a');
    CHECK(stream.location.line_num == 1);
    CHECK(stream.location.col_num == 2);

    // Unread the character, but pass it capital "A" to prove that the
    // reading comes from the internal saved_char variable and not the
    // actual stream
    stream.unread_char('A');
    CHECK(stream.location.line_num == 1);
    CHECK(stream.location.col_num == 1);

    // Read again
    ch = stream.read_char();
    REQUIRE(ch.has_value());
    CHECK(ch.value() == 'A');
    CHECK(stream.location.line_num == 1);
    CHECK(stream.location.col_num == 2);

    // Go on with the stream
    ch = stream.read_char();
    REQUIRE(ch.has_value());
    CHECK(ch.value() == 'b');
    CHECK(stream.location.line_num == 1);
    CHECK(stream.location.col_num == 3);

    ch = stream.read_char();
    REQUIRE(ch.has_value());
    CHECK(ch.value() == 'c');
    CHECK(stream.location.line_num == 1);
    CHECK(stream.location.col_num == 4);

    stream.skip_whitespaces_and_comments();

    ch = stream.read_char();
    REQUIRE(ch.has_value());
    CHECK(ch.value() == 'd');
    CHECK(stream.location.line_num == 2);
    CHECK(stream.location.col_num == 2);

    ch = stream.read_char();
    REQUIRE(ch.has_value());
    CHECK(ch.value() == '\n');
    CHECK(stream.location.line_num == 3);
    CHECK(stream.location.col_num == 1);

    ch = stream.read_char();
    REQUIRE(ch.has_value());
    CHECK(ch.value() == 'e');
    CHECK(stream.location.line_num == 3);
    CHECK(stream.location.col_num == 2);

    ch = stream.read_char();
    REQUIRE(ch.has_value());
    CHECK(ch.value() == 'f');
    CHECK(stream.location.line_num == 3);
    CHECK(stream.location.col_num == 3);

    ch = stream.read_char();
    CHECK(!ch.has_value());
}


// Helper functions to use in the lexer tests
void check_is_keyword(const std::expected<std::unique_ptr<Token>, GrammarError>& res, KeywordEnum expected_kw) {
    REQUIRE(res.has_value()); // Ensure no grammar error occurred

    // Check if the return type is actually what expected: if it is KeywordToken it returns
    // the pointer, otherwise it returns nullptr
    auto* token = dynamic_cast<KeywordToken*>(res.value().get());
    REQUIRE(token != nullptr); // Fails if it's not a KeywordToken

    // Check the value
    CHECK(token->keyword == expected_kw);
}

void check_is_identifier(const std::expected<std::unique_ptr<Token>, GrammarError>& res, const std::string& expected_id) {
    REQUIRE(res.has_value());
    auto* token = dynamic_cast<IdentifierToken*>(res.value().get());
    REQUIRE(token != nullptr);
    CHECK(token->identifier == expected_id);
}

void check_is_symbol(const std::expected<std::unique_ptr<Token>, GrammarError>& res, const char& expected_symbol) {
    REQUIRE(res.has_value());
    auto* token = dynamic_cast<SymbolToken*>(res.value().get());
    REQUIRE(token != nullptr);
    CHECK(token->symbol == expected_symbol);
}

void check_is_number(const std::expected<std::unique_ptr<Token>, GrammarError>& res, const float& expected_number) {
    REQUIRE(res.has_value());
    auto* token = dynamic_cast<LiteralNumberToken*>(res.value().get());
    REQUIRE(token != nullptr);
    CHECK(token->number == expected_number);
}

void check_is_string(const std::expected<std::unique_ptr<Token>, GrammarError>& res, const std::string& expected_string) {
    REQUIRE(res.has_value());
    auto* token = dynamic_cast<LiteralStringToken*>(res.value().get());
    REQUIRE(token != nullptr);
    CHECK(token->string == expected_string);
}

void check_is_stop(const std::expected<std::unique_ptr<Token>, GrammarError>& res) {
    REQUIRE(res.has_value());
    auto* token = dynamic_cast<StopToken*>(res.value().get());
    REQUIRE(token != nullptr);
}

TEST_CASE("Test Lexer: read_token()") {
    std::istringstream string_stream(R"(
        # This is a comment
        # This is another comment
        new material sky_material(
            diffuse(image("my file.pfm")),
            <5.0, 500.0, 300.0>
        ) # Comment at the end of the line
    )");

    InputStream stream(string_stream);

    check_is_keyword(stream.read_token(), KeywordEnum::NEW);
    check_is_keyword(stream.read_token(), KeywordEnum::MATERIAL);
    check_is_identifier(stream.read_token(), "sky_material");
    check_is_symbol(stream.read_token(), '(');
    check_is_keyword(stream.read_token(), KeywordEnum::DIFFUSE);
    check_is_symbol(stream.read_token(), '(');
    check_is_keyword(stream.read_token(), KeywordEnum::IMAGE);
    check_is_symbol(stream.read_token(), '(');
    check_is_string(stream.read_token(), "my file.pfm");
    check_is_symbol(stream.read_token(), ')');
    check_is_symbol(stream.read_token(), ')');
    check_is_symbol(stream.read_token(), ',');
    check_is_symbol(stream.read_token(), '<');
    check_is_number(stream.read_token(), 5.0);
    check_is_symbol(stream.read_token(), ',');
    check_is_number(stream.read_token(), 500.0);
    check_is_symbol(stream.read_token(), ',');
    check_is_number(stream.read_token(), 300.0);
    check_is_symbol(stream.read_token(), '>');
    check_is_symbol(stream.read_token(), ')');

    // Check the reader has come to the end of the stream
    check_is_stop(stream.read_token());

}
