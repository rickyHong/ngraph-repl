//*****************************************************************************
// Copyright 2017-2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*****************************************************************************

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "gtest/gtest.h"
#include "ngraph/frontend/onnx_import/onnx.hpp"
#include "ngraph/ngraph.hpp"
#include "util/all_close.hpp"
#include "util/all_close_f.hpp"
#include "util/ndarray.hpp"
#include "util/test_case.hpp"
#include "util/test_control.hpp"
#include "util/test_tools.hpp"

using namespace ngraph;

static std::string s_manifest = "${MANIFEST}";

using Inputs = std::vector<std::vector<float>>;
using Outputs = std::vector<std::vector<float>>;

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_quantize_linear)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/quantize_linear.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{32.25f, 48.34f, 50.f, 83.f});
    inputs.emplace_back(std::vector<float>{0.5f});

    std::vector<std::vector<std::uint8_t>> expected_output{
        std::vector<std::uint8_t>{64, 97, 100, 166}};

    std::vector<std::vector<std::uint8_t>> outputs{
        execute<float, std::uint8_t>(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_quantize_linear_zero_point)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/quantize_linear_zero_point.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{0.f, 2.f, 3.f, 1000.f, -254.f, -1000.f}); // x
    inputs.emplace_back(std::vector<float>{2.0f});                                   // y_scale

    std::vector<std::vector<std::uint8_t>> int_inputs;
    int_inputs.emplace_back(std::vector<std::uint8_t>{128}); // y_zero_point

    std::vector<std::vector<std::uint8_t>> expected_output{
        std::vector<std::uint8_t>{128, 129, 130, 255, 1, 0}};

    std::vector<std::vector<std::uint8_t>> outputs{execute<float, std::uint8_t, std::uint8_t>(
        function, inputs, int_inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_quantize_linear_axis_zero)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/quantize_linear_axis_zero.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{
        0.f, 2.f, 3.f, 1000.f, 0.f, 2.f, 3.f, 1000.f, 0.f, 2.f, 3.f, 1000.f}); // x
    inputs.emplace_back(std::vector<float>{1.f, 2.f, 4.f});                    // y_scale

    std::vector<std::vector<std::uint8_t>> int_inputs;
    int_inputs.emplace_back(std::vector<std::uint8_t>{0, 0, 0}); // y_zero_point

    std::vector<std::vector<std::uint8_t>> expected_output{
        //  std::vector<std::uint8_t>{0, 2, 3, 255, 0, 1, 2, 255, 0, 1, 1, 250}}; <- bad expected output given HALF_TO_EVEN round mode
        std::vector<std::uint8_t>{0, 2, 3, 255, 0, 1, 2, 255, 0, 0, 1, 250}};

    std::vector<std::vector<std::uint8_t>> outputs{execute<float, std::uint8_t, std::uint8_t>(
        function, inputs, int_inputs, "${BACKEND_NAME}")};
    EXPECT_EQ(expected_output.front(), outputs.front());
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_quantize_linear_axis_negative)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/quantize_linear_axis_negative.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{
        0.f, 2.f, 3.f, 1000.f, 0.f, 2.f, 3.f, 1000.f, 0.f, 2.f, 3.f, 1000.f}); // x
    inputs.emplace_back(std::vector<float>{1.f, 2.f, 4.f});                    // y_scale

    std::vector<std::vector<std::uint8_t>> int_inputs;
    int_inputs.emplace_back(std::vector<std::uint8_t>{0, 0, 0}); // y_zero_point

    std::vector<std::vector<std::uint8_t>> expected_output{
        //  std::vector<std::uint8_t>{0, 2, 3, 255, 0, 1, 2, 255, 0, 1, 1, 250}}; <- bad expected output given HALF_TO_EVEN round mode
        std::vector<std::uint8_t>{0, 2, 3, 255, 0, 1, 2, 255, 0, 0, 1, 250}};

    std::vector<std::vector<std::uint8_t>> outputs{execute<float, std::uint8_t, std::uint8_t>(
        function, inputs, int_inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_dequantize_linear)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/dequant_lin.prototxt"));

    std::vector<std::vector<std::uint8_t>> inputs;
    inputs.emplace_back(std::vector<std::uint8_t>{19, 210, 21, 10});

    Outputs expected_output{std::vector<float>{76.f, 840.f, 84.f, 40.f}};

    Outputs outputs{execute<std::uint8_t, float>(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_dequantize_linear_scalar_zero_scale_uint8)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/dequantize_linear_0.prototxt"));

    auto x = std::vector<uint8_t>{0, 3, 128, 255};
    auto scale = std::vector<float>{2.0f};
    auto zero_point = std::vector<uint8_t>{128};

    auto backend = ngraph::runtime::Backend::create("${BACKEND_NAME}");

    auto params = function->get_parameters();
    std::vector<std::shared_ptr<ngraph::runtime::Tensor>> input_tensors;
    input_tensors.push_back(
        backend->create_tensor(params.at(0)->get_element_type(), params.at(0)->get_shape()));
    input_tensors.push_back(
        backend->create_tensor(params.at(1)->get_element_type(), params.at(1)->get_shape()));
    input_tensors.push_back(
        backend->create_tensor(params.at(2)->get_element_type(), params.at(2)->get_shape()));

    copy_data(input_tensors[0], x);
    copy_data(input_tensors[1], scale);
    copy_data(input_tensors[2], zero_point);

    auto results = function->get_results();
    std::vector<std::shared_ptr<ngraph::runtime::Tensor>> result_tensors;
    result_tensors.push_back(
        backend->create_tensor(results.at(0)->get_element_type(), results.at(0)->get_shape()));

    auto handle = backend->compile(function);
    handle->call_with_validate(result_tensors, input_tensors);

    std::vector<std::vector<float>> outputs;
    outputs.push_back(read_vector<float>(result_tensors[0]));

    auto expected_output = std::vector<std::vector<float>>{{-256.0f, -250.0f, 0.0f, 254.0f}};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_dequantize_linear_scalar_zero_scale_int8)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/dequantize_linear_1.prototxt"));

    auto x = std::vector<int8_t>{-30, -3, 100, 127};
    auto scale = std::vector<float>{2.0f};
    auto zero_point = std::vector<int8_t>{-10};

    auto backend = ngraph::runtime::Backend::create("${BACKEND_NAME}");

    auto params = function->get_parameters();
    std::vector<std::shared_ptr<ngraph::runtime::Tensor>> input_tensors;
    input_tensors.push_back(
        backend->create_tensor(params.at(0)->get_element_type(), params.at(0)->get_shape()));
    input_tensors.push_back(
        backend->create_tensor(params.at(1)->get_element_type(), params.at(1)->get_shape()));
    input_tensors.push_back(
        backend->create_tensor(params.at(2)->get_element_type(), params.at(2)->get_shape()));

    copy_data(input_tensors[0], x);
    copy_data(input_tensors[1], scale);
    copy_data(input_tensors[2], zero_point);

    auto results = function->get_results();
    std::vector<std::shared_ptr<ngraph::runtime::Tensor>> result_tensors;
    result_tensors.push_back(
        backend->create_tensor(results.at(0)->get_element_type(), results.at(0)->get_shape()));

    auto handle = backend->compile(function);
    handle->call_with_validate(result_tensors, input_tensors);

    std::vector<std::vector<float>> outputs;
    outputs.push_back(read_vector<float>(result_tensors[0]));

    auto expected_output = std::vector<std::vector<float>>{{-40.0f, 14.0f, 220.0f, 274.0f}};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_dequantize_linear_1d_zero_scale_uint8)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/dequantize_linear_2.prototxt"));

    auto test_case = ngraph::test::NgraphTestCase(function, "${BACKEND_NAME}");

    test_case.add_input(std::vector<uint8_t>{0, 1, 2, 3, 0, 1, 2, 3, 0, 10, 20, 30}); // x
    test_case.add_input(std::vector<float>{1.0f, 2.0f, 4.0f});                        // scale
    test_case.add_input(std::vector<uint8_t>{0, 0, 0});                               // zero_point

    test_case.add_expected_output<float>(
        {3, 4},
        std::vector<float>{
            0.0f, 1.0f, 2.0f, 3.0f, 0.0f, 2.0f, 4.0f, 6.0f, 0.0f, 40.0f, 80.0f, 120.0f});
    test_case.run();
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_dequantize_linear_1d_zero_scale_int8)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/dequantize_linear_3.prototxt"));

    auto test_case = ngraph::test::NgraphTestCase(function, "${BACKEND_NAME}");

    test_case.add_input(std::vector<int8_t>{0, 1, 2, 3, 0, 2, 4, 6, 0, 10, 20, 30}); // x
    test_case.add_input(std::vector<float>{1.0f, 2.0f, 4.0f, 8.0f});                 // scale
    test_case.add_input(std::vector<int8_t>{0, -10, -20, -30});                      // zero_point

    test_case.add_expected_output<float>(
        {3, 4},
        std::vector<float>{
            0.0f, 22.0f, 88.0f, 264.0f, 0.0f, 24.0f, 96.0f, 288.0f, 0.0f, 40.0f, 160.0f, 480.0f});
    test_case.run();
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_dequantize_linear_1d_zero_scale_int8_4d)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/dequantize_linear_4.prototxt"));

    auto test_case = ngraph::test::NgraphTestCase(function, "${BACKEND_NAME}");

    test_case.add_input(std::vector<uint8_t>{7, 9, 10, 10, 5, 8, 9, 1, 8, 6, 7, 9, 10, 0, 7, 10, 8,
                                             2, 6, 0,  5,  9, 8, 1, 2, 7, 5, 3, 2, 4,  1, 3, 8,  7,
                                             4, 8, 10, 1,  5, 5, 7, 7, 0, 2, 4, 4, 0,  5}); // x
    test_case.add_input(std::vector<float>{1.0f, 10.0f, 7.0f});                             // scale
    test_case.add_input(std::vector<uint8_t>{10, 2, 1}); // zero_point

    test_case.add_expected_output<float>(
        {2, 3, 2, 4},
        std::vector<float>{-3.0f, -1.0f, 0.0f,  0.0f,   -5.0f, -2.0f, -1.0f, -9.0f,  60.0f, 40.0f,
                           50.0f, 70.0f, 80.0f, -20.0f, 50.0f, 80.0f, 49.0f, 7.0f,   35.0f, -7.0f,
                           28.0f, 56.0f, 49.0f, 0.0f,   -8.0f, -3.0f, -5.0f, -7.0f,  -8.0f, -6.0f,
                           -9.0f, -7.0f, 60.0f, 50.0f,  20.0f, 60.0f, 80.0f, -10.0f, 30.0f, 30.0f,
                           42.0f, 42.0f, -7.0f, 7.0f,   21.0f, 21.0f, -7.0f, 28.0f});

    test_case.run();
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_dequantize_linear_1d_zero_scale_uint8_negative_axis)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/dequantize_linear_5.prototxt"));

    auto test_case = ngraph::test::NgraphTestCase(function, "${BACKEND_NAME}");

    test_case.add_input(std::vector<uint8_t>{0, 1, 2, 3, 0, 1, 2, 3, 0, 10, 20, 30}); // x
    test_case.add_input(std::vector<float>{1.0f, 2.0f, 4.0f});                        // scale
    test_case.add_input(std::vector<uint8_t>{0, 0, 0});                               // zero_point

    test_case.add_expected_output<float>(
        {3, 4},
        std::vector<float>{
            0.0f, 1.0f, 2.0f, 3.0f, 0.0f, 2.0f, 4.0f, 6.0f, 0.0f, 40.0f, 80.0f, 120.0f});
    test_case.run();
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_quant_conv_linear)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/quant_conv_lin.prototxt"));

    std::vector<std::vector<std::uint8_t>> inputs;
    inputs.emplace_back(std::vector<std::uint8_t>{
        1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
        22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
        43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81});

    std::vector<std::vector<std::int8_t>> expected_output{std::vector<std::int8_t>{
        2,  3,  3,  3,  4,  4,  4,  5,  2,  4,  6,  7,  8,  8,  9,  9,  10, 3,  8,  11, 12,
        13, 13, 14, 14, 15, 5,  11, 16, 17, 18, 18, 19, 19, 20, 7,  14, 22, 22, 23, 23, 24,
        24, 25, 8,  18, 27, 27, 28, 28, 29, 29, 30, 10, 21, 32, 32, 33, 33, 34, 34, 35, 12,
        24, 37, 37, 38, 38, 39, 40, 40, 13, 17, 26, 27, 27, 27, 28, 28, 28, 9}};

    std::vector<std::vector<std::int8_t>> outputs{
        execute<std::uint8_t, std::int8_t>(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_quant_conv_linear_2d)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/qlinear_conv_2d.prototxt"));

    auto test_case = ngraph::test::NgraphTestCase(function, "${BACKEND_NAME}");

    test_case.add_input_from_file<uint8_t>(TEST_FILES, "onnx/qlinearconv2d/x.bin");
    test_case.add_input(std::vector<float>{0.00369204697199166f}); // x_scale
    test_case.add_input(std::vector<uint8_t>{132});                // x_zero_point
    test_case.add_input(std::vector<uint8_t>{0});                  // w
    test_case.add_input(std::vector<float>{0.00172794575337321f}); // w_scale
    test_case.add_input(std::vector<uint8_t>{255});                // w_zero_point
    test_case.add_input(std::vector<float>{0.00162681262008846f}); // y_scale
    test_case.add_input(std::vector<uint8_t>{123});                // y_zero_point

    test_case.add_expected_output_from_file<uint8_t>(
        {1, 1, 7, 7}, TEST_FILES, "onnx/qlinearconv2d/y.bin");
    test_case.run();
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_quant_conv_linear_3d)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/qlinear_conv_3d.prototxt"));

    auto test_case = ngraph::test::NgraphTestCase(function, "${BACKEND_NAME}");

    test_case.add_input_from_file<uint8_t>(TEST_FILES, "onnx/qlinearconv3d/x.bin");
    test_case.add_input(std::vector<float>{0.00389225385151803f}); // x_scale
    test_case.add_input(std::vector<uint8_t>{127});                // x_zero_point
    test_case.add_input(std::vector<uint8_t>{255});                // w
    test_case.add_input(std::vector<float>{0.00128723995294422f}); // w_scale
    test_case.add_input(std::vector<uint8_t>{0});                  // w_zero_point
    test_case.add_input(std::vector<float>{0.0011764180380851f});  // y_scale
    test_case.add_input(std::vector<uint8_t>{128});                // y_zero_point

    test_case.add_expected_output_from_file<uint8_t>(
        {1, 1, 4, 4, 4}, TEST_FILES, "onnx/qlinearconv3d/y.bin");
    test_case.run();
}
