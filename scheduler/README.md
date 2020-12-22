## (Deprecated) 

The scheduler is aims to provide a on-node level scheduling for different task.
The main idea is using unix socket to maintain all the task.
All task should be created based on the `mission.h`. The main scheduler will do the scheduling and control the mission through unix socket.
It allow realtime mission and repeating mission.

The main drawback is that the scheduler have insufficient control to the mission. The granularity of this method is 10ms level.

If wanna make the scheduling more efficient, need a more tight couple way to do it.
