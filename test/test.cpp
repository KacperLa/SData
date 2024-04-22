#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../include/sdata.hpp"
#include <thread>
#include <chrono>

using namespace sdata;
using namespace std::chrono;

// Mock struct for testing
struct MockData {
    int value {0};
    std::uint64_t timestamp;
};

// Test fixture for SData tests
class SDataTest : public ::testing::Test
{
protected:
    void SetUp() override {
        // Create a temporary file for testing
        temp_file = "/tmp/sdata_test";
    }

    void TearDown() override {
        // Delete the temporary file
        std::remove(temp_file.c_str());
    }

    std::uint64_t get_time_nano()
    {
        return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    }

    std::string temp_file;
};

TEST_F(SDataTest, MemoryMappedSuccessfully) {
    SData<MockData> sdata(temp_file, 100000000, true);
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
//     SData<MockData> sdata(temp_file, 100000000, true);
//     MockData* buffer = sdata.getBuffer();
//     // Add your assertions here
// }

TEST_F(SDataTest, GetBufferIndex) {
    SData<MockData> sdata(temp_file, 100000000, true);
    int bufferIndex = sdata.getBufferIndex();

    // At statup the buffer index should be 1
    EXPECT_EQ(bufferIndex, 1);
}

// TEST_F(SDataTest, WaitOnStateChange) {
//     SData<MockData> sdata(temp_file, true);
//     MockData data;
//     EXPECT_TRUE(sdata.waitOnStateChange(&data));
//     // Add your assertions here
// }

TEST_F(SDataTest, setData_check_BufferIndex) {
    SData<MockData> sdata_producer(temp_file, 100000000, true);

    MockData data;

    // Upon Start up the index of the buffer should be 1
    EXPECT_EQ(sdata_producer.getBufferIndex(), 1);

    // Using setData should increment the buffer index by 1
    sdata_producer.setData(&data);

    EXPECT_EQ(sdata_producer.getBufferIndex(), 2);
;
}

TEST_F(SDataTest, setData_check_BufferIndex_rollover) {
    SData<MockData> sdata_producer(temp_file, 100000000, true);

    // SData relies on a triple buffer system, so the buffer index should rollover
    // 1 -> 2 -> 0 -> 1 ...

    MockData data;

    // Upon Start up the index of the buffer should be 1
    EXPECT_EQ(sdata_producer.getBufferIndex(), 1);

    // Using setData should increment the buffer index by 1
    sdata_producer.setData(&data);

    EXPECT_EQ(sdata_producer.getBufferIndex(), 2);

    // Using setData should increment the buffer index by 1
    sdata_producer.setData(&data);

    EXPECT_EQ(sdata_producer.getBufferIndex(), 0);

    // Using setData should increment the buffer index by 1
    sdata_producer.setData(&data);

    EXPECT_EQ(sdata_producer.getBufferIndex(), 1);
}

TEST_F(SDataTest, Trigger_check_BufferIndex) {
    SData<MockData> sdata_producer(temp_file, 100000000, true);

    MockData data;

    // Upon Start up the index of the buffer should be 1
    EXPECT_EQ(sdata_producer.getBufferIndex(), 1);

    // Triggering the producer should increment the buffer index by 1
    sdata_producer.trigger();

    EXPECT_EQ(sdata_producer.getBufferIndex(), 2);
}

TEST_F(SDataTest, Trigger_check_BufferIndex_rollover) {
    SData<MockData> sdata_producer(temp_file, 100000000, true);

    // SData relies on a triple buffer system, so the buffer index should rollover
    // 1 -> 2 -> 0 -> 1 ...

    // Upon Start up the index of the buffer should be 1
    EXPECT_EQ(sdata_producer.getBufferIndex(), 1);

    // Triggering the producer should increment the buffer index by 1
    sdata_producer.trigger();
    EXPECT_EQ(sdata_producer.getBufferIndex(), 2);

    // Triggering the producer should increment the buffer index by 1
    sdata_producer.trigger();
    EXPECT_EQ(sdata_producer.getBufferIndex(), 0);

    // Triggering the producer should increment the buffer index by 1
    sdata_producer.trigger();
    EXPECT_EQ(sdata_producer.getBufferIndex(), 1);
}

TEST_F(SDataTest, SetData) {
    SData<MockData> sdata_producer(temp_file, 100000000, true);
    SData<MockData> sdata_consumer(temp_file, 100000000, true);

    MockData producer_data;
    MockData consumer_data;

    producer_data.value = 10;

    // get data prior to setting
    sdata_consumer.getData(&consumer_data);

    EXPECT_NE(consumer_data.value, producer_data.value);

    sdata_producer.setData(&producer_data);
    sdata_consumer.getData(&consumer_data);

    EXPECT_EQ(consumer_data.value, producer_data.value);
}

// TEST_F(SDataTest, speedTest) {
//     SData<MockData> sdata_producer(temp_file, 100000000, true); // 100ms timeout
//     SData<MockData> sdata_consumer(temp_file, 100000000, true); // 100ms timeout

//     MockData producer_data;
//     MockData consumer_data;

//     int cycles_to_run = 100;

//     // Counter to count the number of times the consumer has read data
//     int counter = 0;

//     // statistics
//     std::uint64_t averate_time = 0;
//     std::uint64_t max_time = 0;
//     std::uint64_t min_time = 0;

//     // array to store the time taken to read data
//     uint64_t time_taken[cycles_to_run];

//     // create a thread to read data from the producer
//     std::thread consumer_thread([&](){
//         // Get the data from the reader
//         std::cout << "Consumer data: " << std::to_string(consumer_data.value) << "\n";
//         while (consumer_data.value < cycles_to_run-1)
//         {
//             if (sdata_consumer.waitOnStateChange(&consumer_data) == 1)
//             {
//                 break;
//             }
//             time_taken[consumer_data.value] = get_time_nano() - consumer_data.timestamp;
//             counter++;
//         }
//         std::cout << "Consumer data: " << std::to_string(consumer_data.value) << "\n";
//         std::cout << "Consumer thread end \n";
//     });

//     // Sleep for a while to allow the consumer thread to start
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));

//     for (int i = 0; i < cycles_to_run; i++) {
//         producer_data.value = i;
//         producer_data.timestamp = get_time_nano();
//         sdata_producer.setData(&producer_data);
//         std::this_thread::sleep_for(std::chrono::milliseconds(1));
//     }

//     // Wait for the producer thread to finish
//     consumer_thread.join();

//     EXPECT_EQ(counter, cycles_to_run);

//     // Calculate the average time taken to read data
//     std::uint64_t total_time = 0;
//     for (int i = 0; i < cycles_to_run; i++) {
//         if (time_taken[i] > max_time) {
//             max_time = time_taken[i];
//         }
//         if (time_taken[i] < min_time) {
//             min_time = time_taken[i];
//         }
//         total_time += time_taken[i];
//     }
//     averate_time = total_time / cycles_to_run;

//     std::cout << "Average Hz: " << 1000000000/averate_time << "\n";
//     std::cout << "Max time taken to read data: " << max_time << "\n";
//     std::cout << "Min time taken to read data: " << min_time << "\n";
// }


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}