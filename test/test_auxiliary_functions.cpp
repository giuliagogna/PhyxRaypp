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
import auxiliary_functions;


// =========================================================================
// TEST 1: Testing if two float numbers are close (are_close)
// =========================================================================

TEST_CASE("Similarity between two float numbers (are_close)") {

    SUBCASE("The two numbers are actually close within the tolerance") {
        CHECK(aux::are_close(8.999999, 9, 1e-5) == true);
        CHECK(aux::are_close(9.45, 9.45, 1e-5) == true);
        CHECK(aux::are_close(10.01, 10.02, 1e-1) == true);
    }

    SUBCASE("The two numbers are not close within the tolerance") {
        CHECK(aux::are_close(8.9999, 9, 1e-5) == false);
        CHECK(aux::are_close(9.45, 9.44, 1e-5) == false);
    }

}


// =========================================================================
// TEST 2: Testing input file opening (open_input_file)
// =========================================================================

TEST_CASE("Opening files securely (open_input_file)") {

    SUBCASE("Opening an existing file") {
        // Create a temporary dummy file to test the success case
        std::string dummy_filename = "test_dummy_file_for_doctest.tmp";

        {
            // Create the file and close it immediately.
            // The scope block {} ensures the ofstream is destroyed and the file is released.
            std::ofstream out(dummy_filename);
            out << "Just some dummy content to make the file exist.";
        }

        // Try opening it with our auxiliary function
        auto result = aux::open_input_file(dummy_filename);

        // Check if the operation succeeded
        REQUIRE(result.has_value() == true);

        // Check if the stream is actually open and ready
        CHECK(result.value().is_open() == true);

        // Cleanup: close the stream manually (good practice before deleting):
        // needed here because if I try to remove a file that is still opened the OS (Windows in particular)
        // does not let me do it
        // Remove the file from the disk
        result.value().close();
        std::filesystem::remove(dummy_filename);
    }

    SUBCASE("Opening a non-existent file") {
        // Try opening a file that definitely does not exist
        std::string fake_filename = "this_file_absolutely_does_not_exist_12345.xyz";
        auto result = aux::open_input_file(fake_filename);

        // Check if it correctly failed and returned an unexpected value
        REQUIRE(result.has_value() == false);

        // Verify that the error message perfectly matches the expected format
        CHECK(result.error() == std::format("Error in opening input file '{}'", fake_filename));
    }
}


// =========================================================================
// TEST 3: Testing output file opening (open_output_file)
// =========================================================================

TEST_CASE("Opening output files securely (open_output_file)") {

    SUBCASE("Opening a valid output file") {
        std::string dummy_filename = "test_dummy_output_file.tmp";

        // Try creating/opening the file with the auxiliary function
        auto result = aux::open_output_file(dummy_filename);

        // Check if the operation succeeded
        REQUIRE(result.has_value() == true);

        // Check if the stream is actually open and ready for writing
        CHECK(result.value().is_open() == true);

        // Cleanup: close the stream manually (good practice before deleting):
        // needed here because if I try to remove a file that is still opened the OS (Windows in particular)
        // does not let me do it
        // Remove the file from the disk
        result.value().close();
        std::filesystem::remove(dummy_filename);
    }

    SUBCASE("Opening an invalid output file (invalid path)") {
        // Try to create a file inside a directory that does not exist.
        // The OS will deny this operation, triggering our error handler.
        std::string fake_filename = "this_directory_does_not_exist/impossible_file.xyz";
        auto result = aux::open_output_file(fake_filename);

        // Check if it correctly failed and returned an unexpected value
        REQUIRE(result.has_value() == false);

        // Verify that the error message perfectly matches the expected format
        CHECK(result.error() == std::format("Error in opening output file '{}'", fake_filename));
    }
}