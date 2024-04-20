#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "sdata.hpp"

using namespace sdata;

// Mock struct for testing
struct MockData {
    int value;
};

// Test fixture for SData tests
class SDataTest : public ::testing::Test
{
protected:
    void SetUp() override {
        // Create a temporary file for testing
        temp_file = std::tmpnam(nullptr);
    }

    void TearDown() override {
        // Delete the temporary file
        std::remove(temp_file.c_str());
    }

    std::string temp_file;
};

TEST_F(SDataTest, MemoryMappedSuccessfully) {
    SData<MockData> sdata(temp_file, true);
    // sleep for a while to allow the memory mapping to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(sdata.isMemoryMapped());
}

// TEST_F(SDataTest, GetData) {
//     SData<MockData> sdata(temp_file, true);
//     MockData data;
//     EXPECT_TRUE(sdata.getData(&data));
//     // Add your assertions here
// }

// TEST_F(SDataTest, GetBuffer) {
//     SData<MockData> sdata(temp_file, true);
//     MockData* buffer = sdata.getBuffer();
//     // Add your assertions here
// }

// TEST_F(SDataTest, GetBufferIndex) {
//     SData<MockData> sdata(temp_file, true);
//     int bufferIndex = sdata.getBufferIndex();
//     // Add your assertions here
// }

TEST_F(SDataTest, Trigger) {
    SData<MockData> sdata(temp_file, true);

    int prev_index = sdata.getBufferIndex();
    sdata.trigger();
    int new_index = sdata.getBufferIndex();

    // Trigger should increment the buffer index by 1
    EXPECT_EQ(prev_index+1, new_index);
}

// TEST_F(SDataTest, WaitOnStateChange) {
//     SData<MockData> sdata(temp_file, true);
//     MockData data;
//     EXPECT_TRUE(sdata.waitOnStateChange(&data));
//     // Add your assertions here
// }

TEST_F(SDataTest, SetData) {
    SData<MockData> sdata_producer(temp_file, true);
    SData<MockData> sdata_consumer(temp_file, true);

    MockData producer_data;
    MockData consumer_data;

    producer_data.value = 10;

    // get data prior to setting
    sdata_consumer.getData(consumer_data);

    EXPECT_NE(consumer_data.value, producer_data.value);

    sdata_producer.setData(&producer_data);
    sdata_consumer.getData(&consumer_data);

    EXPECT_EQ(consumer_data.value, producer_data.value);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}