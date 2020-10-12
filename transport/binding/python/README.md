In some case, the common shared library in python and the system
is not using the same library.

In this case, the library linked in the generated so will confuse
to the library used by the python. Then it will happed undefine reference problem.

To solve this problem, use the system library to replace the python using library
