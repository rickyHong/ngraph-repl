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

#include "test_case.hpp"
#include "all_close.hpp"
#include "all_close_f.hpp"
#include "gtest/gtest.h"
#include "ngraph/assertion.hpp"
#include "test_tools.hpp"

void ngraph::test::NgraphTestCase::run()
{
    auto function_results = m_function->get_results();
    NGRAPH_CHECK(m_expected_outputs.size() == function_results.size(),
                 "Expected number of outputs is different from the function's number of results.");

    auto handle = m_backend->compile(m_function);
    handle->call_with_validate(m_result_tensors, m_input_tensors);

    for (int i = 0; i < m_expected_outputs.size(); ++i)
    {
        auto result_tensor = m_result_tensors.at(i);
        auto expected_result_constant = m_expected_outputs.at(i);
        auto element_type = result_tensor->get_element_type();

        auto expected_shape = expected_result_constant->get_shape();
        auto result_shape = result_tensor->get_shape();

        EXPECT_EQ(expected_shape, result_shape);

        if (element_type == ngraph::element::f32)
        {
            auto result = read_vector<float>(result_tensor);
            auto expected = expected_result_constant->get_vector<float>();
            EXPECT_TRUE(test::all_close_f(expected, result));
        }
        else if (element_type == ngraph::element::u8)
        {
            auto result = read_vector<uint8_t>(result_tensor);
            auto expected = expected_result_constant->get_vector<uint8_t>();
            EXPECT_TRUE(test::all_close(expected, result));
        }
        else
        {
            NGRAPH_FAIL() << "Please add support for " << element_type
                          << " to ngraph::test::NgraphTestCase::run().";
        }
    }
}
