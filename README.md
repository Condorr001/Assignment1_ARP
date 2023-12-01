
# Creators 
Davide Tonelli - S6424332

Valentina Condorelli - S4945679
Group name: ConTo

# ARP: Assignment 1
## How to run
### Building dependencies
To build this project the following dependencies are needed:
+ make
+ cmake version > 3.6
+ c compiler
+ libncurses
### Command to run the program
Simply execute the run.sh script in the main folder by typing in the shell:

    ./run.sh

## How does it work
### Architecture
![architecture-image-placeholder](docs/Schema_assignment1_ARP.png?raw=true)
### Active components
The active components of this project are:
- Server
- Map
- Drone
- Input
- Watchdog (WD)
#### Server
The main role of the **server** process is to read from the shared memory areas in
order to periodically update the logfile with the drone info. For this purpose, the server is responsible of initializing the semaphores to access the shared memory areas. Moreover, by means of a *fork()*, it spawns the **map** process. 
The primitives used by the server are:
- Kill(): used to send a signal to the WD to tell that it's alive
- Sigaction(): used to initialize the signal handler to handle the signal sent by the WD
- Fork(): used to create a child process which can spawn the map
- Sem_open(), Sem_init(), Sem_wait(), Sem_post(), Sem_unlink(), Sem_close(): used to manage the semaphores
- Shm_open(), Ftruncate(), Mmap(), Shm_unlink(), munmap(): used to manage the shared memory
- Fopen(), Fclose: used to open and close a file located in a specific path (in this case, the logfile)
- Flock(): used to lock/unlock the file when multiple processes can access it
- Execvp(): used to spawn the map

#### Map
The **map** process reads the position data of the drone from the shared memory and
displays it on the screen. As a consequence, the drone can be seen moving in the map following its dynamics, with the borders as limits.
The primitives used by the map are:
- Kill(): used to send a signal to the WD to tell that it's alive
- Sigaction(): used to initialize the signal handler to handle the signal sent by the WD
- Mkfifo(): used to create a fifo (named pipe) to send both its PID and its parent PID (so the pid of the Konsole running it) to the WD
- Open(), Close(): used to open and close the file descriptor associated to the fifo
- Write(): used to write in the fifo
- Sem_open() Sem_wait(), Sem_post(), Sem_unlink(), Sem_close(): used to manage the semaphores declared in the server
- Shm_open(), Ftruncate(), Mmap(), Shm_unlink(), munmap(): used to manage the shared memory

#### Drone
The **drone** process is the one responsible for calculating:
- the current position of the drone, based on the old position;
- the current applied forces, as given by the input process;
- the repulsive forces of the walls;
- the current speed of the drone. 

The formula used to calculate the next position of the drone is the following:
```math
x = \frac{
    W_x + D_x - \frac{M}{T^2}\cdot(x(t-2) - 2\cdot x(t-1)) + \frac{K}{T}\cdot x(t-1)
}{
    \Big(\frac{M}{T^2} + \frac{K}{T}\Big)
}
```
Where:
+ $W_x$ is the x component of the repulsive force from the walls
+ $D_x$ is the x component of the force acting on the drone due to user input
+ $M$ is the mass of the drone
+ $T$ is the time interval with which the formula is calculated
+ $x$, $x(t-1)$, $x(t-2)$ are the position of the drone at different time instants
+ $K$ is the viscous coefficient

For the y coordinate the formula is the same. This describes the dynamics of the drone.

The primitives used by the drone are:
- Kill(): used to send a signal to the WD to tell that it's alive
- Sigaction(): used to initialize the signal handler to handle the signal sent by the WD
- Shm_open(), Ftruncate(), Mmap(), Shm_unlink(), munmap(): used to manage the shared memory
- Sem_open(), Sem_wait(), Sem_post(), Sem_unlink(), Sem_close(): used to manage the semaphores declared in the server

#### Input
The **input** process takes the input from the user keyboard and calculates the current
forces acting on the drone based on those inputs. The resulting forces are
then written in the shared memory in order to be read from the drone process, which uses the forces to compute its dynamics.
The input is also responsible for displaying all the parameters of the drone, such as position, velocity and forces acting on it. The displayed forces do not
take into account repulsive forces of the walls, but only the ones that the user
is applying through the input.

The keys available for the user are:
```
+-+-+---+---+
| q | w | e |
+---+---+---+
| a | s | d |
+---+---+---+
| z | x | c |
+-+-+---+---+
```
The eight external keys can be used to move the drone by adding a force in the respective direction (top, top-right, right and so on). On the other hand, the S key in used to instantly zero all the forces, in order to see the inertia on the drone.

The primitives used by the input are:
- Kill(): used to send a signal to the WD to tell that it's alive
- Sigaction(): used to initialize the signal handler to handle the signal sent by the WD
- Mkfifo(): used to create a fifo (named pipe) to send its pid to the WD
- Open(), Close(): used to open and close the file descriptor associated to the fifo
- Write(): used to write in the fifo
- Sem_open() Sem_wait(), Sem_post(), Sem_unlink(), Sem_close(): used to manage the semaphores declared in the server
- Shm_open(), Ftruncate(), Mmap(), Shm_unlink(), munmap(): used to manage the shared memory

#### Watchdog
The **watchdog** is resposible for checking the correct execution of all the other processes. For this purpose, the WD sends the SIGUSR1 signal to all the processes. Then, two checks are made. Firstly, it verifies that the processes are alive by checking if the *kill* syscall returns an error and immediately kills all the processes if this happens.
Secondly, it verifies that the processes are not frozen by waiting for a SIGUSR2 reply by each process. In case that this signal is not received, meaning
that the process is frozen, the WD kills all the processes.

## Other components, directories and files
The project is structured as follows:
- main folder
	- run.sh: script to run the project
	- CMakeList.txt: cmake file
- *src* folder
	- all the active components
	- master.c, which spawns the active components
	- CMakeList.txt: cmake file
- *headers* folder
	- constants.h: file with all the constants used in the project
	- dataStruct.h: file with all the data structures used in the project
	- *wrapFuncs* folder: contains headers and .c file that implement all the syscall used in the project with errors checking included
	- *utils* folder: contains headers and .c file for specific utility functions
- *docs* folder: contains a few documents to better understand the project
- *build* folder: destination folder of all the built file after running the script
- *bin* folder:
	- *log* folder: contains the logfile
	- *conf* folder: contains the parameter file for the drone dynamics
