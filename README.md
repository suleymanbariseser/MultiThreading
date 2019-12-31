# MultiThreading

Reader Part:

Reader thread reads the data from file and writes the content to an array. To give unique values to the threads create a critical section each thread will have a unique line number.After a thread take a duty, it will seek the file with a unique line and read it. While a read thread reading a line to disable the deadlock we created a new critical section. After those operation, the thread again comes to while again doing same operation and that resume until the last line readed. After the last line readed, the global variable linenumber which keeps the number of lines set equals to reader_tail which keeps last readed line and then, thread again comes to while loop and breaks the while loop.


Upper & Replace Part:

Same as the reader part we gave unique values to each upper thread. When a reader thread reads and writes to the array an upper or a replace thread will enter the critical sections in their respective methods they do their things. Upper thread turns the letter uppercase and replace thread replaces the white spaces with underscores. While loop in methods is to make a single thread to work on multiple lines.Threads will wait for their assigned lines in the method. After all lines are converted to uppercase then uppercase threads break the while loop and finish their jobs. Same for replace case.


Write Part:

After replace_tail and upper_tail will be grater than write_tail, write threads write the line to into file with specified location and those operations continue until there is no more unwrited index in the array.
