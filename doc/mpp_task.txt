MPP task design (2017.4.7)
================================================================================

Mpp task is the contain component for transaction with external user in advanced
mode. The target of advanced mode is to provide flexible, multiple input/output
content for extension.

Mpp task has mpp_meta as the rich content carrier. Mpp meta uses KEY and value
pair for extension. One task can carries multiple data into or out of mpp.
The typical case is encoder with OSD and motion detection. One task may contain
OSD buffer, motion detection buffer, frame buffer and stream buffer as input and
output stream buffer and motion detection buffer with data. And this case can
also be used on decoder if decoder wants to output some side information.


Mpp task transaction
================================================================================

1. Mpp task queue
Mpp task queue is the manager of tasks. Due to user may incorrectly use the task
we choose the design that hold all task inside mpp. Task queue will create and
release task. But task queue will not interact with user directly. We use port
and task status to control the transaction.

2. Mpp port
Mpp port is the transaction interface of task queue. External user and internal
worker thread will use mpp_port_poll / mpp_port_dequeue / mpp_port_enqueue
interface to poll / dequeue / enqueue the task task queue. Mpp advanced mode is
using port to connect external user, interface storage and internal process
thread. Each task queue has two port: input port and output port. And from a
global view the task will always flow from input port to output port.

3. Mpp task status
There are four status for one task. Mpp use list_head to represent the status.

INPUT_PORT : Initial status for input port user to dequeue. Or when output port
             successfully enqueue a task then the task is on this status.

INPUT_HOLD : When input port user successfully dequeue a task then the task is
             on this status.

OUTPUT_PORT: When input port user successfully enqueue a task then the task is
             on OUTPUT_PORT status. And this task is ready for dequeue from
             output port.

OUTPUT_HOLD: When output port user successfully dequeue a task then the task is
             on this status.

4. Mpp task / port transaction
There are three transaction functions on a port: poll / dequeue / enqueue.
When port user call the transaction function task will be transfer from one
status to next status. The status transform flow is unidirectional from input
port to output port.

The overall relationship graph of task / port / status is shown below.

                               1. task queue
                       +------------------------------+
                       |                              |
                  +----+----+  +--------------+  +----+----+
                  |    4    |  |       3      |  |   2.1   |
         +--------+ dequeue <--+    status    <--+ enqueue <---------+
         |        |         |  |  INPUT_PORT  |  |         |         |
         |        +---------+  |              |  +---------+         |
  +------v-----+  |         |  +--------------+  |         |  +------+------+
  |      3     |  |    2    |                    |    2    |  |      3      |
  |   status   |  |  input  |                    |  output |  |   status    |
  | INPUT_HOLD |  |   port  |                    |   port  |  | OUTPUT_HOLD |
  |            |  |         |                    |         |  |             |
  +------+-----+  |         |  +--------------+  |         |  +------^------+
         |        +---------+  |       3      |  +---------+         |
         |        |    4    |  |    status    |  |   2.1   |         |
         +--------> enqueue +-->  INPUT_PORT  +--> dequeue +---------+
                  |         |  |              |  |         |
                  +----+----+  +--------------+  -----+----+
                       |                              |
                       +------------------------------+


Mpp task transaction of a complete work flow
================================================================================

On advanced mode mpp uses two task queue: input task queue and output task
queue.
Input task queue connects input side external user to internal worker thread.
Output task queue connects internal worker thread to output side external user.
Then there will be three threads to parallelize internal process and external
transation. This will maximize mpp efficiency.

The work flow is demonstrated as below graph.

            +-------------------+           +-------------------+
            |  input side user  |           | output side user  |
            +----^---------+----+           +----^---------+----+
                 |         |                     |         |
            +----+----+----v----+           +----+----+----v----+
            | dequeue | enqueue |           | dequeue | enqueue |
            +----^----+----+----+           +----^----+----+----+
                 |         |                     |         |
            +----+---------+----+    MPP    +----+---------v----+
        +---+     input port    +-----------+    output port    +---+
        |   +-------------------+           +-------------------+   |
        |   |  input task queue |           | output task queue |   |
        |   +-------------------+           +-------------------+   |
        |   |    output port    |           |     input port    |   |
        |   +----+---------^----+           +----+---------^----+   |
        |        |         |                     |         |        |
        |   +----v----+----+----+           +----v----+----+----+   |
        |   | dequeue | enqueue |           | dequeue | enqueue |   |
        |   +----+----+----^----+           +----+----+----^----+   |
        |        |         |                     |         |        |
        |   +----v---------+---------------------v---------+----+   |
        |   |              internal work thread                 |   |
        |   +---------------------------------------------------+   |
        |                                                           |
        +-----------------------------------------------------------+

