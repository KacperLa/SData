# SData
One-to-Many Lock-Free IPC Using Mapped Memory 

The goal of SData is to enable the distribution of info between different processes. This solution was tailored specificity for high-performance robotic systems. 

## Things to keep in mind when deciding if this library is right for your project:
- This library is designed for use on Linux, the library relies on linux system calls for mapping memory and waiting/waking on futexes. 
- This library does not guarantee that every "reader" will process every update, that depends on your kernel, so ABA problems may occur. Please do your own bench marks to see what is the upper limit of throughput on your system.
- This this library is most suitable for applications where you are okay with missing a message once in a while.
Example: If you are communicating a button state, rather than publishing one or a zero, increment a counter every time a button is pressed. 
- The included Unit Test contain a test for timing 1000 updates between a reader and a writer, play with that to get an idea of what is possible.
- This library relies on a three element circular buffer with does not provide protection against reading/writing to the same index, this rarely happens, and each read function provides a return value that can be checked for thi. 

## Run the tests
``` bash
mkdir build
cd build
cmake ..
make
./sdata_test
```

## How to use SData
Include the .hpp file
``` c++
#include "lib/SData/include/sdata.hpp"
```

Define of what should be kept in memory
``` c++
struct example_t
{
    int some_int;
    bool some_bool;
}
```

Create an instance of SData writer
``` c++
    std::string temp_file = '/tmp/sdata_example';
    uint64_t timeout = 100000000;
    bool is_producer = true;

    SData<example_t> sdata_writer(temp_file, timeout, is_producer);
```

Create an instance of SData reader
``` c++
    std::string temp_file = '/tmp/sdata_example';
    uint64_t timeout = 100000000;
    bool is_producer = false;

    SData<example_t> sdata_reader(temp_file, timeout, is_producer);
```

preform a write
``` c++
    example_t example;
    example.some_int = 10;
    example.some_bool = true;

    sdata_writer.setData(&example);
```

perform a read
Doing a simple read will return the const current avalible buffer. 
``` c++
    example_t example;

    sdata_reader.getData(&example);
```
or you may ask the reader to wait for the next data update (useful when syncing data processing with a data source)
``` c++
    example_t example;

    int error = sdata_reader.waitOnStateChange(&example);
    // 0 no error
    // 1 timeout
    // 2 potential buffer corruption
```

### Directly write to buffer
If you want to be fancy and minimize the number of times data is copied
you can do the following. 

``` c++
    example_t * example;

    // get pointer to the current write buffer, after every trigger as this pointer will be stale.
    example = sdata_writer.getBuffer();

    example->some_int = 10;
    example->some_bool = true;

    sdata_writer.trigger();
```

This library is sill in development so expect feature improvements and growth, PR's are welcome.
