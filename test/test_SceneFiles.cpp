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
import Geometry;
import Color;
import Pigment;
import BRDF;
import Shape;
import Material;
import Camera;
import Mesh;

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

TEST_CASE("Test Lexer: unread_token()") {
    // Sequence of tokens
    std::istringstream string_stream("sphere (");
    InputStream stream(string_stream);

    // Read the first token
    auto tok1_res = stream.read_token();
    check_is_keyword(tok1_res, KeywordEnum::SPHERE);

    // Read the second token
    auto tok2_res = stream.read_token();
    check_is_symbol(tok2_res, '(');

    // Put the token back into the stream
    // Because tok2_res.value() holds a unique_ptr, we must use std::move
    // to transfer ownership of the memory back to the InputStream's saved_token.
    stream.unread_token(std::move(tok2_res.value()));

    // Read again: It must be the exact same token we just put back
    auto tok3_res = stream.read_token();
    check_is_symbol(tok3_res, '(');

    // The stream should now be completely empty
    check_is_stop(stream.read_token());
}


// =======================================================================
// PARSER TESTS
// =======================================================================

TEST_CASE("Test Parser: Expect functions") {

    SUBCASE("expect_symbol()") {
        std::istringstream string_stream("< ]");
        InputStream stream(string_stream);

        auto res1 = expect_symbol(stream, '<');
        REQUIRE(res1.has_value());

        // The next token is '[', so expecting a ',' should FAIL
        auto res2 = expect_symbol(stream, ',');
        REQUIRE_FALSE(res2.has_value());

        // Print it to the Doctest output safely
        //MESSAGE("Successfully caught error: ", res2.error().message);

        // Automate the check! Ensure the message actually mentions the wrong symbol
        CHECK(res2.error().message.find("Expected symbol ','") != std::string::npos);
    }

    SUBCASE("expect_keyword()") {
        std::istringstream string_stream("sphere(");
        InputStream stream(string_stream);

        auto res = expect_keywords(stream, {KeywordEnum::SPHERE, KeywordEnum::PLANE, KeywordEnum::CUBE});
        REQUIRE(res.has_value());
        CHECK(res.value() == KeywordEnum::SPHERE);

        std::istringstream string_stream2("pluto (");
        InputStream stream2(string_stream2);
        auto res2 = expect_keywords(stream2, {KeywordEnum::SPHERE, KeywordEnum::PLANE, KeywordEnum::CUBE});
        REQUIRE_FALSE(res2.has_value());
        //MESSAGE("Successfully caught error: ", res2.error().message);

    }

    SUBCASE("expect_identifier()") {
        std::istringstream string_stream("material sky_material(");
        InputStream stream(string_stream);

        auto res1 = expect_keywords(stream, {KeywordEnum::MATERIAL});
        REQUIRE(res1.has_value());
        auto res2 = expect_identifier(stream);
        REQUIRE(res2.has_value());
        CHECK(res2.value() == "sky_material");

        std::istringstream string_stream2("sphere");
        InputStream stream2(string_stream2);

        auto res3 = expect_identifier(stream2);
        REQUIRE_FALSE(res3.has_value());
        // Check that it caught the wrong token type
        CHECK(res3.error().message.find("Expected an Identifier") != std::string::npos);
    }

    SUBCASE("expect_string()") {
        // Using escape \ to let it read the " characters, otherwise the stream contains
        // only beautiful string and not "beautiful string" and read_token() reads it as a
        // keyword or identifier token and not LiteralString
        std::istringstream string_stream("\"beautiful string\"");
        InputStream stream(string_stream);

        auto res = expect_string(stream);
        REQUIRE(res.has_value());
        CHECK(res.value() == "beautiful string");

        std::istringstream string_stream2("unquoted_string");
        InputStream stream2(string_stream2);

        auto res2 = expect_string(stream2);
        REQUIRE_FALSE(res2.has_value());
        CHECK(res2.error().message.find("Expected an LiteralString") != std::string::npos);
    }

    SUBCASE("expect_number()") {

        Scene scene;
        scene.float_variables["my_var"] = 42.5f;

        // Literal Number
        std::istringstream string_stream1("3.14");
        InputStream stream1(string_stream1);
        auto res1 = expect_number(stream1, scene);
        REQUIRE(res1.has_value());
        REQUIRE(res1.value() == 3.14f);

        // Read a variable from the scene
        std::istringstream string_stream2("my_var");
        InputStream stream2(string_stream2);
        auto res2 = expect_number(stream2, scene);
        REQUIRE(res2.has_value());
        CHECK(res2.value() == 42.5f);

        // Unknown variable
        std::istringstream iss3("unknown_var");
        InputStream stream3(iss3);
        auto res3 = expect_number(stream3, scene);
        REQUIRE_FALSE(res3.has_value());
    }
}



TEST_CASE("Test Parser: Parse functions") {

    Scene scene;

    // parse_vec()
    SUBCASE("parse_vec()") {
        std::istringstream string_stream1("[0.1, 0.2, 0.3]");
        InputStream stream1(string_stream1);

        auto res1 = parse_vec(stream1, scene);
        REQUIRE(res1.has_value());
        CHECK(res1.value().is_close(Vec{0.1, 0.2, 0.3}));

        std::istringstream string_stream2("[0.1, 0.2]");
        InputStream stream2(string_stream2);
        auto res2 = parse_vec(stream2, scene);
        REQUIRE_FALSE(res2.has_value());
    }

    // parse_color()
    SUBCASE("parse_color()") {
        std::istringstream string_stream1("<0.1, 0.2, 0.3>");
        InputStream stream1(string_stream1);
        auto res1 = parse_color(stream1, scene);
        REQUIRE(res1.has_value());
        CHECK(res1.value().is_close(Color{0.1, 0.2, 0.3}));

        std::istringstream string_stream2("[0.1, 0.2, 0.3]");
        InputStream stream2(string_stream2);
        auto res2 = parse_color(stream2, scene);
        REQUIRE_FALSE(res2.has_value());
        //MESSAGE("Successfully caught error: ", res2.error().message);
    }

    // parse_pigment()
    SUBCASE("parse_pigment()") {
        SUBCASE("parse_pigment() - UNIFORM") {
            std::istringstream string_stream("uniform(<0.1, 0.2, 0.3>)");
            InputStream stream(string_stream);

            auto res = parse_pigment(stream, scene);
            REQUIRE(res.has_value());

            auto* uniform_pigment = dynamic_cast<UniformPigment*>(res.value().get());
            REQUIRE(uniform_pigment != nullptr);

            Color c = uniform_pigment->get_color({0,0});
            CHECK(c.is_close(Color{0.1, 0.2, 0.3}));
        }

        SUBCASE("parse_pigment() - CHECKERED") {
            std::istringstream string_stream("checkered(<0.1, 0.2, 0.3>, <0.4, 0.5, 0.6>, 4)");
            InputStream stream(string_stream);

            auto res = parse_pigment(stream, scene);
            REQUIRE(res.has_value());

            auto* checkered_pigment = dynamic_cast<CheckeredPigment*>(res.value().get());
            REQUIRE(checkered_pigment != nullptr);

            CHECK(checkered_pigment->color1.is_close(Color{0.1, 0.2, 0.3}));
            CHECK(checkered_pigment->color2.is_close(Color{0.4, 0.5, 0.6}));
            CHECK(checkered_pigment->num_steps == 4);
        }

        SUBCASE("parse_pigment() - IMAGE") {
            std::string test_filename = "dummy_test_texture.pfm";

            // Create a tiny 1x1 PFM file
            {
                std::ofstream out(test_filename, std::ios::binary);
                out << "PF\n1 1\n-1.0\n";
                float r = 1.0f, g = 0.5f, b = 0.25f;
                out.write(reinterpret_cast<const char*>(&r), sizeof(float));
                out.write(reinterpret_cast<const char*>(&g), sizeof(float));
                out.write(reinterpret_cast<const char*>(&b), sizeof(float));
            } // File closes here

            std::istringstream string_stream("image(\"" + test_filename + "\")");
            InputStream stream(string_stream);

            auto pigment_res = parse_pigment(stream, scene);
            REQUIRE(pigment_res.has_value());

            auto* img_pigment = dynamic_cast<ImagePigment*>(pigment_res.value().get());
            CHECK(img_pigment != nullptr);

            Color c = img_pigment->get_color({0.5,0.5});
            CHECK(c.is_close(Color{1.0f, 0.5f, 0.25f}));

            std::filesystem::remove(test_filename);
        }

        // =============================================
        // Negative tests - parse_pigment
        // =============================================

        SUBCASE("Invalid Pigment Keyword (Negative)") {
            // "magic" is not a valid pigment keyword
            std::istringstream string_stream("magic(<1.0, 1.0, 1.0>)");
            InputStream stream(string_stream);

            auto res = parse_pigment(stream, scene);
            REQUIRE_FALSE(res.has_value());
            CHECK(res.error().message.find("Expected a keyword, but got a different token type") != std::string::npos);
        }

        SUBCASE("Missing closing bracket in UNIFORM (Negative)") {
            // Missing the final ')'
            std::istringstream string_stream("uniform(<0.1, 0.2, 0.3>");
            InputStream stream(string_stream);

            auto res = parse_pigment(stream, scene);
            REQUIRE_FALSE(res.has_value());
            CHECK(res.error().message.find("Expected symbol ')'") != std::string::npos);
        }

        SUBCASE("Missing parameters in CHECKERED (Negative)") {
            std::istringstream string_stream("checkered(<0.1, 0.2, 0.3>)");
            // Forgetting the comma, the second color, and the steps
            InputStream stream(string_stream);

            auto res = parse_pigment(stream, scene);
            REQUIRE_FALSE(res.has_value());
            CHECK(res.error().message.find("Expected symbol ','") != std::string::npos);
        }

        SUBCASE("Non-existent image file (Negative)") {
            // Providing a file that doesn't exist on the hard drive
            std::istringstream string_stream("image(\"this_file_does_not_exist.pfm\")");
            InputStream stream(string_stream);

            auto res = parse_pigment(stream, scene);
            REQUIRE_FALSE(res.has_value());

            CHECK(res.error().message.find("Failed to load image") != std::string::npos);
        }
    }


    // parse_brdf()
    SUBCASE("parse_brdf()") {
        SUBCASE("parse_brdf() - DIFFUSE") {
            std::istringstream string_stream("diffuse(uniform(<1.0, 1.0, 1.0>))");
            InputStream stream(string_stream);

            auto res = parse_brdf(stream, scene);
            REQUIRE(res.has_value());

            auto* diff_brdf = dynamic_cast<DiffusiveBRDF*>(res.value().get());
            REQUIRE(diff_brdf != nullptr);

            auto* uniform_pigment = dynamic_cast<UniformPigment*>(diff_brdf->pigment.get());
            REQUIRE(uniform_pigment != nullptr);

            Color c = uniform_pigment->get_color({0.0,0.0});
            CHECK(c.is_close(Color{1.0, 1.0, 1.0}));

        }

        SUBCASE("parse_brdf() - SPECULAR") {
            std::istringstream string_stream("specular(uniform(<1.0, 1.0, 1.0>))");
            InputStream stream(string_stream);

            auto res = parse_brdf(stream, scene);
            REQUIRE(res.has_value());

            auto* spec_brdf = dynamic_cast<SpecularBRDF*>(res.value().get());
            REQUIRE(spec_brdf != nullptr);

            auto* uniform_pigment = dynamic_cast<UniformPigment*>(spec_brdf->pigment.get());
            REQUIRE(uniform_pigment != nullptr);

            Color c = uniform_pigment->get_color({0.0,0.0});
            CHECK(c.is_close(Color{1.0, 1.0, 1.0}));
        }

        // =============================================
        // Negative tests - parse_brdf
        // =============================================

        SUBCASE("Invalid BRDF Keyword (Negative)") {
            // Providing a keyword that isn't diffuse or specular
            std::istringstream string_stream("material(uniform(<1.0, 1.0, 1.0>))");
            InputStream stream(string_stream);

            auto res = parse_brdf(stream, scene);
            REQUIRE_FALSE(res.has_value());
            CHECK(res.error().message.find("unexpected keyword") != std::string::npos);
        }

        SUBCASE("Missing Opening Bracket (Negative)") {
            // Missing the '(' after diffuse
            std::istringstream string_stream("diffuse uniform(<1.0, 1.0, 1.0>))");
            InputStream stream(string_stream);

            auto res = parse_brdf(stream, scene);
            REQUIRE_FALSE(res.has_value());
            CHECK(res.error().message.find("Expected symbol '('") != std::string::npos);
        }

        SUBCASE("Invalid Nested Pigment (Negative)") {
            // Passing a typo inside the pigment definition ("uniforme" instead of "uniform")
            std::istringstream string_stream("diffuse(uniforme(<1.0, 1.0, 1.0>))");
            InputStream stream(string_stream);

            auto res = parse_brdf(stream, scene);
            REQUIRE_FALSE(res.has_value());

            // The error from parse_pigment should perfectly bubble up!
            CHECK(res.error().message.find("Expected a keyword") != std::string::npos);
        }

        SUBCASE("Missing Closing Bracket (Negative)") {
            // Missing the final ')'
            std::istringstream string_stream("diffuse(uniform(<1.0, 1.0, 1.0>)");
            InputStream stream(string_stream);

            auto res = parse_brdf(stream, scene);
            REQUIRE_FALSE(res.has_value());
            CHECK(res.error().message.find("Expected symbol ')'") != std::string::npos);
        }
    }


    // parse_material()
    SUBCASE("parse_material()") {
        SUBCASE("Valid Material") {
            // Material grammar: identifier( BRDF, emitted_pigment )
            std::istringstream string_stream("my_shiny_mat(specular(uniform(<1.0, 1.0, 1.0>)), uniform(<0.0, 0.0, 0.0>))");
            InputStream stream(string_stream);

            auto res = parse_material(stream, scene);
            REQUIRE(res.has_value());

            // Ensure the identifier was parsed correctly
            CHECK(res.value().first == "my_shiny_mat");

            // Extract the material
            Material mat = std::move(res.value().second);
            REQUIRE(mat.brdf != nullptr);
            REQUIRE(mat.emitted_radiance != nullptr);

            // Check that the BRDF is a specular: the correct parsing pf the BRDF has been tested above
            auto* spec_brdf = dynamic_cast<SpecularBRDF*>(mat.brdf.get());
            REQUIRE(spec_brdf != nullptr); // Proves it's specular

            // Check that the emitted radiance is a uniform pigment: the correct parsing of pigments has been tested above
            auto* emitted_pigment = dynamic_cast<UniformPigment*>(mat.emitted_radiance.get());
            REQUIRE(emitted_pigment != nullptr); // Proves the emission is uniform

        }

        SUBCASE("Missing BRDF definition (Negative)") {
            // Just putting a pigment where the BRDF should be
            std::istringstream string_stream("bad_mat(uniform(<1.0, 1.0, 1.0>), uniform(<0.0, 0.0, 0.0>))");
            InputStream stream(string_stream);

            auto res = parse_material(stream, scene);
            REQUIRE_FALSE(res.has_value());
            CHECK(res.error().message.find("unexpected keyword") != std::string::npos);
        }
    }

    // parse_transformation()
    SUBCASE("parse_transformation()") {
        SUBCASE("Valid LL(1) Lookahead Chaining") {
            // Test LL(1) Lookahead chaining: Translation * Scaling * Rotation X
            std::istringstream string_stream("translation([0.0, 10.0, 0.0]) * scaling([2.0, 2.0, 2.0]) * rot_x(1.57079632679)");
            InputStream stream(string_stream);

            auto res = parse_transformation(stream, scene);
            REQUIRE(res.has_value());

            Transformation t = res.value();

            // Verify mathematically: point (1, 1, 1) -> Rotate X 90deg (1, -1, 1) -> Scale by 2 (2, -2, 2) -> Translate Y by 10 (2, 8, 2)
            Point p{1.0f, 1.0f, 1.0f};
            Point result = t * p;

            CHECK(result.is_close(Point{2.0, 8.0, 2.0}, 1e-5));
        }

        SUBCASE("Invalid chained keyword (Negative)") {
            // "translating" is a typo (should be "translation")
            std::istringstream string_stream("scaling([2.0, 2.0, 2.0]) * translating([0.0, 1.0, 0.0])");
            InputStream stream(string_stream);

            auto res = parse_transformation(stream, scene);
            REQUIRE_FALSE(res.has_value());
            CHECK(res.error().message.find("Expected a keyword,") != std::string::npos);
        }
    }


    // parse_camera()
    SUBCASE("parse_camera()") {
        SUBCASE("parse_camera() - PERSPECTIVE") {
            std::istringstream string_stream("(perspective, 1.5, 10.0, translation([0.0, 0.0, -5.0]))");
            InputStream stream(string_stream);

            auto res = parse_camera(stream, scene);
            REQUIRE(res.has_value());

            auto* persp = dynamic_cast<PerspectiveCamera*>(res.value().get());
            REQUIRE(persp != nullptr);

            CHECK(persp->aspect_ratio == 1.5);
            CHECK(persp->d == 10.0);

            Transformation exp_tr = Trans(Vec{0.0, 0.0, -5.0});
            CHECK(persp->trans.m.is_close(exp_tr.m));
        }

        SUBCASE("parse_camera() - ORTHOGONAL") {
            std::istringstream string_stream("(orthogonal, 1.5, identity)");
            InputStream stream(string_stream);

            auto res = parse_camera(stream, scene);
            REQUIRE(res.has_value());

            auto* ortho = dynamic_cast<OrthogonalCamera*>(res.value().get());
            REQUIRE(ortho != nullptr);

            CHECK(ortho->aspect_ratio == 1.5);

            Transformation exp_tr = Transformation{};
            CHECK(ortho->trans.m.is_close(exp_tr.m));
        }

        SUBCASE("parse_camera() - Missing closing bracket (Negative)") {
            // Missing the final ')' after the transformation
            std::istringstream string_stream("(orthogonal, 1.5, identity");
            InputStream stream(string_stream);

            auto res = parse_camera(stream, scene);
            REQUIRE_FALSE(res.has_value());
            CHECK(res.error().message.find("Expected symbol ')'") != std::string::npos);
        }
    }

    // parse_shape()
    SUBCASE("parse_shape<T>()") {
        SUBCASE("Valid Sphere with Material Lookup") {
            // Pre-populate the scene dictionary with a dummy material so the lookup works
            auto dummy_brdf = std::make_shared<DiffusiveBRDF>(std::make_shared<UniformPigment>(Color{1.0f,1.0f,1.0f}));
            auto dummy_emitted = std::make_shared<UniformPigment>(Color{0.0f,0.0f,0.0f});
            scene.materials["blue_mat"] = std::make_shared<Material>(dummy_brdf, dummy_emitted);

            std::istringstream string_stream("(scaling([2.0, 2.0, 2.0]), blue_mat)");
            InputStream stream(string_stream);

            // Test Sphere building
            auto res = parse_shape<Sphere>(stream, scene);
            REQUIRE(res.has_value());

            // Confirm the material pointer was linked to the scene dictionary
            CHECK(res.value()->material == scene.materials["blue_mat"]);

            // Confirm the transformation is correct
            Transformation expected_trans = Scale(Vec{2.0f, 2.0f, 2.0f});
            CHECK(res.value()->trans.m.is_close(expected_trans.m));
        }

        SUBCASE("Unknown Material applied (Negative)") {
            // Because this is a fresh subcase, scene.materials is already empty here!
            std::istringstream string_stream("(identity, invisible_material)");
            InputStream stream(string_stream);

            auto res = parse_shape<Sphere>(stream, scene);
            REQUIRE_FALSE(res.has_value());
            CHECK(res.error().message.find("Unknown material 'invisible_material'") != std::string::npos);
        }
    }

    // parse_mesh()
    SUBCASE("parse_mesh()") {

        auto dummy_brdf = std::make_shared<DiffusiveBRDF>(std::make_shared<UniformPigment>(Color{1.0f,1.0f,1.0f}));
        scene.materials["mesh_mat"] = std::make_shared<Material>(dummy_brdf);

        std::string test_obj = "dummy_test_mesh.obj";
        {
            std::ofstream out(test_obj);
            out << "v 0 0 0\nv 1 0 0\nv 0 1 0\n";
            out << "vn 0 0 1\nvn 0 0 1\nvn 0 0 1\n";
            out << "f 1/1/1 2/2/2 3/3/3\n";
        }

        SUBCASE("Valid Mesh - 3 Arguments (Defaults)") {
            std::istringstream string_stream(std::format("(\"{}\", mesh_mat, identity)", test_obj));
            InputStream stream(string_stream);

            auto res = parse_mesh(stream, scene);
            REQUIRE(res.has_value());

            // Prove it successfully built a Mesh and linked the material
            auto* mesh = dynamic_cast<Mesh*>(res.value().get());
            REQUIRE(mesh != nullptr);
            CHECK(mesh->material == scene.materials["mesh_mat"]);
        }

        SUBCASE("Valid Mesh - 4 Arguments (Custom Bins)") {
            std::istringstream string_stream(std::format("(\"{}\", mesh_mat, identity, 25)", test_obj));
            InputStream stream(string_stream);

            auto res = parse_mesh(stream, scene);
            REQUIRE(res.has_value());
            REQUIRE(dynamic_cast<Mesh*>(res.value().get()) != nullptr);
        }

        SUBCASE("Valid Mesh - 5 Arguments (Custom Bins & Threshold)") {
            std::istringstream string_stream(std::format("(\"{}\", mesh_mat, identity, 25, 5)", test_obj));
            InputStream stream(string_stream);

            auto res = parse_mesh(stream, scene);
            REQUIRE(res.has_value());
            REQUIRE(dynamic_cast<Mesh*>(res.value().get()) != nullptr);
        }

        // ==========================================
        // Negative Tests
        // ==========================================

        SUBCASE("Negative: Missing File") {
            std::istringstream string_stream("(\"this_file_does_not_exist.obj\", mesh_mat, identity)");
            InputStream stream(string_stream);

            auto res = parse_mesh(stream, scene);
            REQUIRE_FALSE(res.has_value());
            CHECK(res.error().message.find("Cannot open mesh file") != std::string::npos);
        }

        SUBCASE("Negative: Bad Optional Syntax") {
            // User typed a comma but forgot the number
            std::istringstream string_stream(std::format("(\"{}\", mesh_mat, identity, )", test_obj));
            InputStream stream(string_stream);

            auto res = parse_mesh(stream, scene);
            REQUIRE_FALSE(res.has_value());
            // Since there is no number, expect_number will throw its standard error
            CHECK(res.error().message.find("Expected a literal number") != std::string::npos);
        }

        std::filesystem::remove("dummy_test_mesh.obj");
    }
}

// =======================================================================
// INTEGRATION TESTS (FILE-BASED)
// =======================================================================

TEST_CASE("Integration: parse_scene() from external files") {

    SUBCASE("Valid End-to-End Scene") {
        std::istringstream string_stream(R"(
            float clock = 150.0;
            float cam_dist = 2.0;

            material sky_material(
                diffuse(uniform(<0.0, 0.0, 0.0>)),
                uniform(<0.7, 0.5, 1.0>)
            );

            # Here is a comment

            material ground_material(
                diffuse(checkered(<0.3, 0.5, 0.1>, <0.1, 0.2, 0.5>, 4)),
                uniform(<0.0, 0.0, 0.0>)
            );

            material sphere_material(
                specular(uniform(<0.5, 0.5, 0.5>)),
                uniform(<0.0, 0.0, 0.0>)
            );

            plane(translation([0.0, 0.0, 100.0]) * rot_y(clock), sky_material);
            plane(identity, ground_material);

            sphere(translation([0.0, 0.0, 1.0]), sphere_material);

            camera(perspective, 1.0, cam_dist, rot_z(30.0) * translation([-4.0, 0.0, 1.0]));
        )");

        InputStream stream(string_stream);

        auto scene_res = parse_scene(stream);
        if (!scene_res.has_value()) {
            MESSAGE("Parse error: ", scene_res.error().message);
        }
        REQUIRE(scene_res.has_value());
        Scene scene = std::move(scene_res.value());

        // Check the float variables
        CHECK(scene.float_variables.size() == 2);
        CHECK(scene.float_variables.contains("clock"));
        CHECK(scene.float_variables.at("clock") == 150.0f);
        CHECK(scene.float_variables.contains("cam_dist"));
        CHECK(scene.float_variables.at("cam_dist") == 2.0f);

        // Check the materials (not in order). Parsing of the material is tested above
        CHECK(scene.materials.size() == 3);
        CHECK(scene.materials.contains("sky_material"));
        CHECK(scene.materials.contains("sphere_material"));
        CHECK(scene.materials.contains("ground_material"));

        CHECK(scene.materials.at("sky_material") != nullptr);
        CHECK(scene.materials.at("sphere_material") != nullptr);
        CHECK(scene.materials.at("ground_material") != nullptr);

        // Check the shapes in the world
        REQUIRE(scene.world.shapes.size() == 3);

        // First shape should be a Plane with the sky_material
        auto* shape1 = dynamic_cast<Plane*>(scene.world.shapes[0].get());
        REQUIRE(shape1 != nullptr);
        CHECK(shape1->material == scene.materials.at("sky_material"));

        // Second shape should be a Plane with the ground_material
        auto* shape2 = dynamic_cast<Plane*>(scene.world.shapes[1].get());
        REQUIRE(shape2 != nullptr);
        CHECK(shape2->material == scene.materials.at("ground_material"));

        // Third shape should be a Sphere with the sphere_material
        auto* shape3 = dynamic_cast<Sphere*>(scene.world.shapes[2].get());
        REQUIRE(shape3 != nullptr);
        CHECK(shape3->material == scene.materials.at("sphere_material"));

        // Check the Camera (Plumbing check)
        REQUIRE(scene.camera != nullptr);
        auto* persp_cam = dynamic_cast<PerspectiveCamera*>(scene.camera.get());
        REQUIRE(persp_cam != nullptr);

    }

    SUBCASE("Invalid scene - starting with something that is not a keyword") {
        std::istringstream string_stream(R"(
            This file starts with a comment but you forgot the '#'
            float variable_that_should_not_be_read = 5.0;
            )");

        InputStream stream(string_stream);
        auto res = parse_scene(stream);
        REQUIRE_FALSE(res.has_value());
        CHECK(res.error().message.find("Expected a keyword") != std::string::npos);
    }

    SUBCASE("Invalid scene - missing semicolon") {
        std::istringstream string_stream(R"(
            # This file starts with a comment
            float you_forgot_semicolon = 5.0
            )");

        InputStream stream(string_stream);
        auto res = parse_scene(stream);
        REQUIRE_FALSE(res.has_value());
        CHECK(res.error().message.find("Expected symbol") != std::string::npos);
    }

    SUBCASE("Invalid scene - variable redefinition") {
        std::istringstream string_stream(R"(
            # This file starts with a comment
            float define_1 = 5.0;
            float define_1 = 12.0;
            )");

        InputStream stream(string_stream);
        auto res = parse_scene(stream);
        REQUIRE_FALSE(res.has_value());
        CHECK(res.error().message.find("cannot be redefined") != std::string::npos);
    }

    SUBCASE("Invalid scene: illegal top-level keyword") {
        // 'diffuse' is a valid keyword, but it belongs inside a material, not out in the open!
        std::istringstream stream("diffuse(uniform(<1.0, 1.0, 1.0>));");
        InputStream input_stream(stream);

        auto scene_res = parse_scene(input_stream);
        REQUIRE_FALSE(scene_res.has_value());
        CHECK(scene_res.error().message.find("Keyword 'diffuse' is not allowed at the top level") != std::string::npos);
    }

    SUBCASE("Negative: Double Camera Definition") {
        std::istringstream stream(R"(
            camera(perspective, 1.0, 2.0, identity);
            camera(orthogonal, 1.0, identity);
        )");
        InputStream input_stream(stream);

        auto scene_res = parse_scene(input_stream);
        REQUIRE_FALSE(scene_res.has_value());
        CHECK(scene_res.error().message.find("more than one Camera") != std::string::npos);
    }
}