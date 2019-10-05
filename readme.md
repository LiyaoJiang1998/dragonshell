# Dragonshell

**
CMPUT 379 - Operating Systems Concepts 2019FALL
Assignment #1
Name: Liyao Jiang  
ID:**
- - -

## Design choices

- - -

The program is written in C++, the main function has a while loop that keeps prompting user with a new line, read for command after the last command is finished. Parses the String into ";" separated multiple commands and parses each command into tokens. Passes the token vector to `run_command` helper function to run the corresponding command. String parsing is done by calling the provided `tokenize` function from the helper code.

- ### Create a New Process for Each Command

    Before running each command, a new process is created using `fork()` system call

- ### Built-in Commands

  - **cd** Command

    Using the `chdir()` system call to change to the given directory in the input argument. Prompts user when no argument is provided, or when the provided file or directory is not valid.
  
  - **pwd** Command
  
    Using the `getcwd()` system call to get the working directory, then print to stdout.
  
  - Showing the path and **a2path** Command
  
    Using a variable path_str to keep the ":" separated paths, path is initialized to be "/bin/:/usr/bin/",implementing built in command `$PATH` to print out the current value of path variable path_str.

    Using a2path to append to the path_str, if the user input start with "$PATH:", then append to path_str, otherwise it will overwrite the current path_str with the user input.
  
  - **exit** Command

    To gracefully exit the dragonshell program, either by pressing "Ctrl + D" or input the "exit" command. The "Ctrl + D" is handled by watching for std::cin.EOF since Ctrl + D sends a EOF to the stdin. When "exit" input is run, the same function is called as when "Ctrl + D" is received.

    On exit, gracefully close things, using `kill()` system call to send SIGINT signal to the background process if any, and use `wait()` system call to hold until child process finishes, lastly use `_exit()` system call to exit the parent process.

- ### Run External Programs

    When the input command doesn't match any built-in command, I assume it to be a external command. Using the helper function `find_filename` try to match the input command with the stored path variables, using system call `access()` to test if the file exists, if the file exists, set as the filename for `execv()` system call to run, otherwise if every path variable is not matched to valid file, assume the command given is absolute path and use 'execv()' system call to run. If `execv()` system file return value says that it could not find the file, then dragonshell prompts the user the command is invalid.

    Before using 'execv()' system call to run the external program, a new child process is created by calling `fork()` system call, and the parent process use `wait()` system call to wait for the child process to finish. Then give the control back to user, ready for the next command.

    **Note:** Pipe and Putting Jobs in Background implementation will be discussed later in their own sections.

- ### Output Redirection

    My function `Redirection()` will check the input command tokens vector to find ">" output redirection symbol and the next argument which is output filename. First, if found, it will remove the ">" from the tokens, and calls `open()` system call to open a new file with the output filename. Then, use `dup2()` system call to replace the STDOUT_FILENO with the new file.Use `close()` system call to close the unused file descriptor. Then runs the command as usual, and the output is redirected to the output file.

    The STDOUT_FILENO and STDIN_FILENO as variables and restored when needed using helper function 'restore_std()', which uses `dup2()` system call to replace the saved STDOUT_FILENO and STDIN_FILENO back after the output redirection command finishes.

- ### Pipe

    Before running the command as regular commands, my `find_pipe()` function is called to check is "|" pipe symbol is found in the commands. If not found, it returns false to say that no pipe is found, and run the command as regular. If it finds the "|", then it parses the two commands separated by "|" into two sector of token vectors.

    It uses `pipe()` system call to create a pipe in the parent process and then calls "fork()" system call to create a child process. Child write output to pipe write end, and parent read child's output as input from pipe read end. The unused ends are closed using 'close()' system call.

    Using `dup2()` system call, the child's output is replaced with pipe's write end. And using the same `dup2()` system call, the parent's input is replaced with pipe's read end. After child process finishes, `_exit()` system call terminates the child process, and parent process calls `wait()` process to wait for child to finish, before giving back the control to user. Then `find_pipe()` returns true saying that it already handled the piped two commands, and ready for the next command from user, no need to execute any command regularly without pipe.

- ### Running Multiple Commands

    The program first parses the input line string from `getline()` function by delimiter ";". So each command is separately stored in the vector<string\>. Then each command parsed to tokens and handled sequentially，after all commands are finished, return control back to user.

- ### Handling Signals

    System call `sigaction()` is used to assign handler to the SIGINT (a.k.a CTRL+C) and SIGTSTP(a.k.a CTRL+V). In the handler, if the two signals are received in dragonshell parent process and no child process is running, then both signals will be ignored by taking no action. If the signals are received, and there is child process running, then the signals are send as is to child process using 'kill()' system call. So that SIGINT (a.k.a CTRL+C) and SIGTSTP(a.k.a CTRL+V) won't stop or interrupt the dragonshell.

- ### Putting Jobs in Background

    When parsing the command to tokens, if & is found at the end, the program run in the background using `fork()` system call to create a child and 'execV()' to run the external command. In the parent process, it **does not** `wait()` for the children to finish. Instead it keeps going, prints out the prompt of "child's PID running in the background" and returns the control back to user.

## Testing

- - -

For each feature I implement, I designed them incrementally, each time I finish one feature and test them by compile and run the program as a user, and perform the implemented feature with some test commands see if the behavior is correct, before working on the next feature. So each feature is working, and I test the required features are working when integrating the features together.

## Acknoledgement

- - -

The dragonshell.cc code skeleton is provided by the start code on eclass. The Makefile I wrote references the Makefile example in LAB 1.
