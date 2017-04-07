Mpp buffer design (2016.10.12)
================================================================================

Mpp buffer is the warpper of the buffer used by hardware. Hardware usually can
not use the buffer malloc by cpu. Then we design MppBuffer for different memory
allocator on different platform. Currently it is designed for ion buffer on
Android and drm buffer on Linux. Later may support vb2_buffer in v4l2 devices.

In order to manage buffer usage in different user mpp buffer module introduces
MppBufferGroup as bufffer manager. All MppBuffer will connect to its manager.
MppBufferGroup provides allocator service and buffer reuse ability.

Different MppBufferGroup has different allocator. And besides normal malloc/free
function the allocator can also accept buffer from external file descriptor.

MppBufferGroup has two lists, unused buffer list and used buffer list. When
buffer is free buffer will not be released immediately. Buffer will be moved to
unused list for later reuse. There is a good reason for doing so. When video
resolution comes to 4K the buffer size will be above 12M. It will take a long
time to allocate buffer and generate map table for it. So reusing the buffer
will save the allocate/free time.

Here is the diagram of Mpp buffer status transaction.

            +----------+     +---------+
            |  create  |     |  commit |
            +-----+----+     +----+----+
                  |               |
                  |               |
                  |          +----v----+
                  +----------+ unused  <-----------+
                  |          +---------+           |
                  |          |         |           |
                  |          |         |           |
            +-----v----+     |         |     +-----+----+
            |  malloc  |     |         |     |   free   |
            +----------+     |         |     +----------+
            | inc_ref  |     +---------+     |  dec_ref |
            +-----+----+                     +-----^----+
                  |                                |
                  |                                |
                  |          +---------+           |
                  +---------->  used   +-----------+
                             +---------+
                             |         |
                             |         |
                             |         |
                             |         |
                             |         |
                             +----^----+
                                  |
                                  |
                             +----+----+
                             | import  |
                             +---------+


