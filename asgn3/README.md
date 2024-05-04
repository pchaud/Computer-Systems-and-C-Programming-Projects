# Assignment 3 directory

This directory contains source code and other files for Assignment 3.

# Program Design 

This program is divided into two main files- queue.c and rwlock.c. One involving the constructor, destructor, enqeue/push, and dequeue/pop functions through the use of the pthread library. The other file involves basic RW locking and unlocking implementation through the use of the pthread mutex functions. There is one composite provided Makefile that compiles both files. 

# Bugs 

Currently, my tests all pass and there's no bugs that I've detected so far. 

# Design Implementations on Vague Instruction 

For queue.c, I impelmented a shared variable mutex, utilized condition variables, and a buffer and the pthread library to simulate a mutex and it's respective operations. I learned with the textbook pseudocode it's much easier to implement lock and unlock functions in C++ since it automatically completes the constructor and destructor functions and has innate lock and unlock functions. Therefore, the pthread library helped remedy this with my queue functions. Beforehand, I started using the semaphore.h library but I realized I started facing starvation errors so switching to the pthread library heavily optimized my code. 

For rwlock.c I also reused the pthread library and its operations on mutexs' to handle my locking and unlocking functions. The way Pratik helped me understand the logic for the vague instructions on prioritization was to split each priority group and it's respective handling with three seperate if-else statements. This specific structure which is repeated in my rwlock.c file helped with the broadness of what's needed in the prioritzation conditions. Additionally, organizing my struct to keep track of active and waiting readers and writers as well as keeping track of the reads since the last write helped me pass throughput. Without having the readers_since_the_last_write variable, I was facing a coherency issue because I was never comparing the readers since the last write with the capacity (n) in my N_WAY prioritization. Additionally, within my writer_unlock I was facing issues with passing throughput because I simplified my structure too much. I solely had one if else statement signaling and broadcasting, but I didn't realize I needed to do this for the same tree branch of all the priorities in my unlock function as well to successfully unlock my mutex. 

# Resources 

https://canvas.ucsc.edu/courses/69163/files/folder/Lecture%20Slides?preview=8561314 : I used lecture 14 slide 31 of the mutex Lock and push and pop functions to help impelment my own queue.c functions. 

https://www.kea.nu/files/textbooks/ospp/osppv2.pdf (Tom Anderson and Mike Dahlin’s Operating Systems: Principle and Practice, Volume 2) : I used pages 115-116 from to implement my rwlock.c functions which greatly helped with implementing rwlock.h's functions without the use of a semaphore library but instead with locks and condition variables. 

https://pubs.opengroup.org/onlinepubs/009604499/functions/pthread_mutex_lock.html 
https://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_broadcast.html : 
online MAN pages describing the pthread library which helped me to understand how to use the pthread library better for both files. 

I attended my normal discussion on 2/28 with Pratik and he helped me understand how rwlock.c needed to be implmeneted, starvation and deadlocking issues that could've occured with the code I brought in with the semaphore library, and he helped me understand the very basic outline of the rwlock file.