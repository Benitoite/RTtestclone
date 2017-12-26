#include <string>

#include "LUT.h"
#include "gtest/gtest.h"

TEST(LutTest, BasicFloatInterpolation) {
    LUTf testLut(4);

    testLut[0] = 10.0;
    testLut[1] = 11.0;
    testLut[2] = 12.0;
    testLut[3] = 13.0;

    // See comment in source about how 2.5 not mapping to 12.5 is weird.
    std::vector<float> inputs = {
            -1.0, -0.0, 0.0, 0.5, 1.0, 1.75, 2.0, 2.5, 3.0, 3.5, 1e6};
    std::vector<float> expected_vec = {
            10.0, 10.0, 10.0, 10.5, 11.0, 11.75, 12.0, 13.0, 13.0, 13.0, 13.0};
    ASSERT_EQ(inputs.size(), expected_vec.size());
    for (int i = 0; i < inputs.size(); ++i) {
        float input = inputs[i];
        float expected = expected_vec[i];
        EXPECT_EQ(testLut[input], expected) << "Mismatch for input " << input;
    }
}

#if defined( __SSE2__ ) && defined( __x86_64__ )
TEST(LutTest, VectorizedFloatInterpolation) {
    constexpr int kTestIterations = 100;

    LUTf testLut(4);

    testLut[0] = 10.0;
    testLut[1] = 11.0;
    testLut[2] = 12.0;
    testLut[3] = 13.0;

    // See comment in source about how 2.5 not mapping to 12.5 is weird.
    std::vector<float> inputs = {
            -1.0, -0.0, 0.0, 0.5, 1.0, 1.75, 2.0, 2.5, 3.0, 3.5, 1e6};
    std::vector<float> expected_vec = {
            10.0, 10.0, 10.0, 10.5, 11.0, 11.75, 12.0, 12.5, 13.0, 13.0, 13.0};
    ASSERT_EQ(inputs.size(), expected_vec.size());


    for (int iteration = 0; iteration < kTestIterations; ++iteration) {
        // Indexes for 4-element SSE vector to look up.
        std::vector<float> inputsAsFloat(4);
        std::vector<float> expectedAsFloat(4);
        for (int i = 0; i < 4; ++i) {
            size_t index = rand() % inputs.size();
            inputsAsFloat[i] = inputs[index];
            expectedAsFloat[i] = expected_vec[index];
        }
        vfloat inputsSse = LVFU(inputsAsFloat[0]);

        vfloat outputs = testLut[inputsSse];
        std::vector<float> outputsAsVector(4);
        STVFU(outputsAsVector[0], outputs);

        for (int i = 0; i < 4; ++i) {
            EXPECT_NEAR(outputsAsVector[i], expectedAsFloat[i], 1e-5)
                << "Mismatch on input value " << inputsAsFloat[i];
        }
    }
}
#endif
