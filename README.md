# C-Project-2: Simple UNIX Shell in C

This project implements a simple command-line shell in C, designed for educational purposes to 
simulate core behaviors of UNIX/Linux shells. It was developed as part of the CSE321 course's Term Projects.
The shell allows users to execute basic Linux commands, handle I/O redirection, process piping,
multiple command execution, and also provides history and signal handling functionalities.

Features:

- Display prompt and execute basic Linux commands
- Built-in commands: cd, history
- Redirection support: '<', '>', '>>'
- Command piping support: 'cmd1 | cmd2 | cmd3'
- Multiple commands separated by semicolon (;)
- Conditional command execution using '&&'
- Command history tracking
- Signal handling: Ignores 'Ctrl+C' at shell level and terminates only active child processes and terminates 'Ctrl+D'


To Run the code:

-go to the terminal of linux
-create: "touch code.c"
-write/edit: "gedit code.c"
-compile: "gcc -o c code.c" " ./c "
