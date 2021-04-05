Functionalities
- Execute commands using execvp system call and shell specific commands like cd, help, exit, history.
- Input/Output redirection (>, <).
- Pipes (|).
- Handling interrupts for jobs.
- Running jobs in the foreground and background using fg and bg command.
- Command line editing with TAB completion.
- history command - Uparrow and Downarrow key for command history navigation.

To install readline,
sudo apt-get install -y libreadline-dev

Compile using
cc shell.c -lreadline -o shell
