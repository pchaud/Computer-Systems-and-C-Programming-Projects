# Assignment 2 directory

This directory contains source code and other files for Assignment 2.


# Program Design 

My program is divided into 4 functions: a parsing function responsible for string parsing the received commands, a function that handles get requests, a function that handles put requests, and finally the main function which assembles all of the above plus establishes a connection to the server. 

# Bugs 

Program did not work at all. I felt like my code was okay but I should've done better with error handling and my parsing. 

# Resources 

https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/errno.h.html: This resource helped me figure out errno.h parameters and look for further description and understanding on errno functions provided in the assignment documentation (like EAGAIN/EWOULDBLOCK).

https://dev.to/namantam1/ways-to-get-the-file-size-in-c-2mag#:~:text=For%20this%2C%20we%20simply%20call,simply%20use%20the%20st_size%20attribute.: This helped me understand common approaches on getting the size of the file within my get function. I was inspired mainly by their lseek approach and I changed this approach to work for our assignment and used it as an introduction to lseek's functionality. 

https://git.ucsc.edu/cse130/cse130-winter24/resources/-/blob/main/practica/examples/regex/memory_parsing.c?ref_type=heads: The memory_parsing.c file in our resources which I used to help me write my parse function. 

https://docs.python.org/3/howto/regex.html: This resource helped me create my matches functionality within my parse function and helped to better understand regex functions and their needed formatting. 

https://www.gnu.org/software/libc/manual/html_node/Regexp-Subexpressions.html: helped me understand rm_so and rm_eo better for matches in my parse function.



