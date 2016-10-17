Hardware kernel driver design (2016.10.17)
================================================================================

Rockchip has two sets of hardware kernel driver.

The first one is vcodec_service/vpu_service/mpp_service which is a high
performance stateless frame base hardware kernel driver. This driver supports
all available codecs that hardware can provide. This driver is used on Android/
Linux.

Here is the vcodec_service kernel driver framework diagram.

    +-------------+             +-------------+             +-------------+
    |  client  A  |             |  client  B  |             |  client  C  |
    +-------------+             +-------------+             +-------------+

userspace
+------------------------------------------------------------------------------+
 kernel

    +-------------+             +-------------+             +-------------+
    |  session A  |             |  session B  |             |  session C  |
    +-------------+             +-------------+             +-------------+
    |             |             |             |             |             |
 waiting         done        waiting         done        waiting         done
    |             |             |             |             |             |
+---+---+     +---+---+     +---+---+     +---+---+                   +---+---+
| task3 |     | task0 |     | task1 |     | task0 |                   | task0 |
+---+---+     +-------+     +-------+     +-------+                   +---+---+
    |                                                                     |
+---+---+                                                             +---+---+
| task2 |                                                             | task1 |
+-------+                                                             +-------+
                                 +-----------+
                                 |  service  |
                       +---------+-----------+---------+
                       |               |               |
                    waiting         running          done
                       |               |               |
                 +-----+-----+   +-----+-----+   +-----+-----+
                 |  task A2  |   |  task A1  |   |  task C0  |
                 +-----+-----+   +-----------+   +-----+-----+
                       |                               |
                 +-----+-----+                   +-----+-----+
                 |  task B1  |                   |  task C1  |
                 +-----+-----+                   +-----+-----+
                       |                               |
                 +-----+-----+                   +-----+-----+
                 |  task A3  |                   |  task A0  |
                 +-----------+                   +-----+-----+
                       |
                 +-----+-----+
                 |  task B0  |
                 +-----------+

The principle of this design is to separate user task handling and hardware
resource management and minimize the kernel serial process time between two
hardware process operation.

The driver uses session as communication channel. Each userspace client (client)
will have a kernel session. Client will commit tasks to session. Then hardware
is managed by service (vpu_service/vcodec_service). Service will provide the
ability to process tasks in sessions.

When client commits a task to kernel the task will be set to waiting status and
link to both session waiting list and service waiting list. Then service will
get task from waiting list to running list and run. When hardware finishs a task
the task will be moved to done list and put to both service done list and
session done list. Finally client will get the finished task from session.

