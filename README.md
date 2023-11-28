# Assignment 1
## How to run
Simply execute the run.sh script

    ./run.sh

## How does it work
### Architecture
![architecture-image-placeholder](docs/Schema_assignment1_ARP.png?raw=true)
### General explanation
The main role of the **server** process is to read from the shared memory areas in
order to write into the log file. Its other purposes are to initialize the
semaphores to access the shared memory areas, and to spawn the map process. 

The **map** process reads the position data of the drone from the shared memory and
displays it on the screen.

The **drone** process is the one responsible for calculating the current position of
the drone given the old position and the current applied forces that are given
by the input process, and the repulsive forces of the walls. This process also
calculates the current speed of the drone. 

The formula used to calculate the next position of the drone is the 
following:
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

For the y coordinate the formula is the same.

The **input** process takes the input from the user and calculates the current
forces acting on the drone only produced by user input. This calculated forces are
then written in the shared memory in order to be read from the drone process.
This process is also responsible for displaying all the parameters of the drone
like: position, velocity and forces acting on it. The displayed forces do not
take into account repulsive forces of the walls, but only the ones that the user
is applying with the input.

The **watchdog** process sends the SIGUSR1 signal to all the processes and as a
first check it verifies that the processes are responding to that signal,
meaning that if the *kill* syscall returns an error then it immediately kills
the not respondent process. If the process targeted by the sending of the signal is active it
has to respond with SIGUSR2. In case that this signal is not received, meaning
that the process is frozen, then the watchdog will in the same way kill the
unresponsive process.
