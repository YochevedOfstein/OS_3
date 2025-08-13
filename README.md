
# Operating Systems: Assignment 3


Names: Lior Telman, Yocheved Ofstein  

------------

## The submission includes:
   - Folders: part_1 through part_10 
   - Individual makefiles for each question  
   - This README.md file

## Folder Map
   - [part_1](part_1/) — `Point.hpp`, basic Convex Hull + polygon area
   - [part_2](part_2/) — Same algo with 3 containers (*vector* / *deque* / *list*) + `gprof` profiling
   - [part_3](part_3/) — CLI (stdin) with commands: `Newgraph`, `Newpoint`, `Removepoint`, `CH`
   - [part_4](part_4/) — **Single-thread, multi-client** server using `select()`
   - [part_5](part_5/) — Library: `libreactor.a` (reactor API for I/O readiness)
   - [part_6](part_6/) — Server using the **reactor** (callbacks `onAccept` / `onClientRead`)
   - [part_7](part_7/) — **Thread-per-client** server (blocking I/O; `std::mutex` guards `Graph`)
   - [part_8](part_8/) — **Proactor** implementation (accept thread + per-client worker threads)
   - [part_9](part_9/) — Part-7 server rewritten to **use the proactor** (`startProactor`, same handler)
   - [part_10](part_10/) — Prints when CH area crosses **100** up or down