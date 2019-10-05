/**
 * Name: Liyao Jiang
 * ID:
 */
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <algorithm>
#include <fcntl.h>
#include <signal.h>
using namespace std;

string path_str = "/bin/:/usr/bin/";
pid_t child_pid = (pid_t)-1;
pid_t bg_pid = (pid_t)-1;

/**
 * @brief Tokenize a string 
 * 
 * @param str - The string to tokenize
 * @param delim - The string containing delimiter character(s)
 * @return std::vector<std::string> - The list of tokenized strings. Can be empty
 */
std::vector<std::string> tokenize(const std::string &str, const char *delim)
{
  char *cstr = new char[str.size() + 1];
  std::strcpy(cstr, str.c_str());

  char *tokenized_string = strtok(cstr, delim);

  std::vector<std::string> tokens;
  while (tokenized_string != NULL)
  {
    tokens.push_back(std::string(tokenized_string));
    tokenized_string = strtok(NULL, delim);
  }
  delete[] cstr;

  return tokens;
}

/**
 * @brief Handles cd change directory built in command
 * 
 * @param vector<string> token_str_vector - the parsed command tokens
 */
void cd(vector<string> token_str_vector)
{
  if (token_str_vector.size() == 2)
  {
    if (chdir(token_str_vector[1].c_str()) == -1)
    {
      cout << "dragonshell: No such file or directory" << endl;
    }
  }
  else if (token_str_vector.size() < 2)
  {
    cout << "dragonshell: expected argument to \"cd\"" << endl;
  }
  else
  {
    cout << "dragonshell: too many arguments to \"cd\"" << endl;
  }
}

/**
 * @brief Handles pwd print working directory built in command
 */
void pwd()
{
  char s[PATH_MAX];
  cout << getcwd(s, PATH_MAX) << endl;
}

/**
 * @brief Handles $PATH built in command
 * prints the current path
 */
void path()
{
  cout << "Current PATH: " << path_str << endl;
}

/**
 * @brief Handles a2path add to path built in command
 * 
 * @param vector<string> token_str_vector - the parsed command tokens
 */
void a2path(vector<string> token_str_vector)
{
  if (token_str_vector.size() < 2)
  {
    // overwriting with empty string
    path_str = "";
  }
  else if (token_str_vector.size() == 2)
  {
    if (token_str_vector[1].substr(0, 6).compare("$PATH:") == 0)
    {
      // appending
      if (token_str_vector[1].substr(6).compare("") != 0)
      {
        if (path_str.compare("") != 0)
        {
          path_str = path_str + ":";
        }
        path_str = path_str + token_str_vector[1].substr(6);
      }
    }
    else
    {
      // overwriting
      path_str = token_str_vector[1].substr(0);
    }
  }
}

/**
 * @brief Handles exit and ctrl+d built in command
 * gracefully closes child processes and exits the dragonshell
 */
void graceful_exit()
{
  cout << "Exiting" << endl;
  // terminate processes running in the background if any
  if (bg_pid != -1)
  {
    kill(bg_pid, SIGINT);
  }
  while (wait(NULL) > 0)
    ;
  fflush(stdout);
  _exit(0);
}

/**
 * @brief Handles signal callbacks
 * @param int signum - the signal identifier
 */
void signal_callback_handler(int signum)
{
  // handle ctrl+c and ctrl+z to send signal to child, parent won't be stopped
  if ((signum == SIGINT) | (signum == SIGTSTP))
  {
    if ((getpid() != child_pid) & (child_pid != -1))
    {
      kill(child_pid, signum);
      child_pid = -1;
    }
  }

  // bg process told parent it finshed, get ready for next
  if (signum == SIGCHLD)
  {
    if ((getpid() != bg_pid) & (bg_pid != -1))
    {
      kill(bg_pid, SIGINT);
      bg_pid = -1;
    }
  }
}

/**
 * @brief Handles regular external command executing
 * @param string filename - the file to be executed
 * @param vector<string> token_str_vector - the parsed command tokens
 */
void run_program(string filename, vector<string> token_str_vector)
{
  int pid;
  char *argv[token_str_vector.size() + 1];
  for (size_t i = 0; i < token_str_vector.size(); i++)
  {
    argv[i] = const_cast<char *>(token_str_vector[i].c_str());
  }
  argv[token_str_vector.size()] = '\0';

  if ((pid = fork()) == -1)
  {
    perror("fork error");
  }
  else if (pid == 0)
  {
    if (execv(filename.c_str(), argv) == -1)
    {
      // perror("execve");
      cout << "dragonshell: Command not found" << endl;
    }
    _exit(0);
  }
  else if (pid > 0)
  {
    child_pid = pid;
    if (wait(NULL) == -1)
    {
      child_pid = -1;
      // perror("wait");
      return;
    }
    child_pid = -1;
  }
}

/**
 * @brief Handles background external command executing
 * @param string filename - the file to be executed
 * @param vector<string> token_str_vector - the parsed command tokens
 */
void run_program_bg(string filename, vector<string> token_str_vector)
{
  int pid, ppid;
  char *argv[token_str_vector.size() + 1];
  for (size_t i = 0; i < token_str_vector.size(); i++)
  {
    argv[i] = const_cast<char *>(token_str_vector[i].c_str());
  }
  argv[token_str_vector.size()] = '\0';

  ppid = getpid();
  if ((pid = fork()) == -1)
  {
    perror("fork error");
  }
  else if (pid == 0)
  {
    int fd = open("/dev/null", O_WRONLY);
    dup2(fd, STDOUT_FILENO);
    close(fd);
    if (execv(filename.c_str(), argv) == -1)
    {
      // perror("execve");
      cout << "dragonshell: Command not found" << endl;
    }
    kill(ppid, SIGCHLD); // this will not be reached, but implicitly, child will send SIGCHLD to parent
    _exit(0);
  }
  else if (pid > 0)
  {
    bg_pid = pid;
    cout << "PID " << bg_pid << " is running in the background" << endl;
    return;
  }
}

/**
 * @brief Helps to find the given command in the path variables
 * @param vector<string> token_str_vector - the parsed command tokens
 * @param vector<string> path_str_vector - the parsed path variable tokens
 */
string find_filename(vector<string> token_str_vector, vector<string> path_str_vector)
{
  string filename = token_str_vector[0]; //assume it is absolute path
  string command = token_str_vector[0];

  // unless find any maybe_filename = path + command, that is valid
  for (size_t i = 0; i < path_str_vector.size(); i++)
  {
    string path = path_str_vector[i];
    string maybe_filename = path + command;
    // validate maybe_filename exists;
    if (access(maybe_filename.c_str(), F_OK) == 0)
    {
      filename = maybe_filename;
      break;
    }
  }
  return filename;
}

/**
 * @brief Restores the stdin and stdout back
 * @param int saved_stdout - stored STDOUT_FILENO file descriptor
 * @param int saved_stdin - stored STDIN_FILENO file descriptor
 */
void restore_std(int saved_stdout, int saved_stdin)
{
  // set output back to stdout
  dup2(saved_stdout, STDOUT_FILENO);
  dup2(saved_stdin, STDIN_FILENO);
}

/**
 * @brief Handles the output redirection feature
 * @param vector<string> token_str_vector - the parsed command tokens
 */
vector<string> redirection(vector<string> token_str_vector)
{
  bool found_redirection = false;
  size_t index_redirection;
  for (size_t i = 0; i < token_str_vector.size(); i++)
    if (token_str_vector[i] == ">")
    {
      found_redirection = true;
      index_redirection = i;
      break;
    }
  if (!found_redirection)
  {
    return token_str_vector;
  }
  else
  {
    // make sure there it follows [process] > [file]
    if ((index_redirection < token_str_vector.size() - 1) & (index_redirection != 0))
    {
      // set output to file
      string output_filename = token_str_vector[index_redirection + 1];
      int fd1;
      if ((fd1 = open(output_filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
      {
        perror("open");
        _exit(EXIT_FAILURE);
      }
      dup2(fd1, STDOUT_FILENO);
      close(fd1);
      // get then return the proper token_str_vector without ">" and " [file]"
      token_str_vector.erase(token_str_vector.begin() + index_redirection,
                             token_str_vector.begin() + (index_redirection + 2));
      return token_str_vector;
    }
    else
    {
      // case no outputfile given
      token_str_vector[0] = "";
      return token_str_vector;
    }
  }
}

/**
 * @brief Handles each command from user input
 * checks builtin commands first, otherwise run as external command
 * checks for running in the background, and call the run_comman_bg handler
 * @param vector<string> token_str_vector - the parsed command tokens
 * @param vector<string> path_str_vector - the parsed path variable tokens
 */
void run_command(vector<string> token_str_vector, vector<string> path_str_vector)
{
  if (token_str_vector[0].compare("") == 0)
  {
    cout << "dragonshell: expected \"[process] > [file]\"" << endl;
  }
  else if (token_str_vector[0].compare("cd") == 0)
  {
    cd(token_str_vector);
  }
  else if (token_str_vector[0].compare("pwd") == 0)
  {
    pwd();
  }
  else if (token_str_vector[0].compare("$PATH") == 0)
  {
    path();
  }
  else if (token_str_vector[0].compare("a2path") == 0)
  {
    a2path(token_str_vector);
  }
  else if (token_str_vector[0].compare("exit") == 0)
  {
    graceful_exit();
  }
  else // run external program
  {
    string filename = find_filename(token_str_vector, path_str_vector);
    if ((token_str_vector[token_str_vector.size() - 1].compare("&") == 0) & (token_str_vector.size() != 1))
    {
      // put job in background
      token_str_vector.erase(token_str_vector.end());
      if (bg_pid == -1)
      {
        // when there is no bg process running
        run_program_bg(filename, token_str_vector);
      }
      else
      {
        // exceeded the max of 1 bg process
        cout << "dragonshell: maximum of one bg process!" << endl;
      }
    }
    else
    {
      run_program(filename, token_str_vector);
    }
  }
}

/**
 * @brief Detects for piping, and handles piping
 * @param vector<string> token_str_vector - the parsed command tokens
 * @param vector<string> path_str_vector - the parsed path variable tokens
 */
bool find_pipe(vector<string> token_str_vector, vector<string> path_str_vector)
{
  bool found_pipe = false;
  size_t index_pipe;
  for (size_t i = 0; i < token_str_vector.size(); i++)
    if (token_str_vector[i] == "|")
    {
      found_pipe = true;
      index_pipe = i;
      break;
    }
  if (!found_pipe)
  {
    return false;
  }
  else
  {
    // make sure there exists [process_1] and [process_2] arguments
    if ((index_pipe < token_str_vector.size() - 1) & (index_pipe != 0))
    {
      // First, split to token_str_vector1 and token_str_vector2
      vector<string> token_str_vector1 = vector<string>(token_str_vector.begin(),
                                                        token_str_vector.begin() + index_pipe);
      vector<string> token_str_vector2 = vector<string>(token_str_vector.begin() + index_pipe + 1,
                                                        token_str_vector.end());

      // redirect output from process_1 to the input of process_2
      int fd[2];
      pid_t pid;
      if (pipe(fd) < 0)
        perror("pipe error!");
      if ((pid = fork()) < 0)
        perror("fork error!");
      if (pid == 0)
      {
        // child write output to pipe write end
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        run_command(token_str_vector1, path_str_vector);
        _exit(0);
      }
      else
      {
        // parent read child's output as input from pipe read end
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        run_command(token_str_vector2, path_str_vector);
        wait(NULL);
      }
    }
    else
    {
      cout << "dragonshell: expected \"[process] > [file]\"" << endl;
    }
    return true;
  }
}

/**
 * @brief print the string prompt without a newline, before beginning to read
 * tokenize the input, run the command(s), and print the result do this in a loop
 */
int main(int argc, char **argv)
{
  // signal handling
  struct sigaction sa;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = signal_callback_handler;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTSTP, &sa, NULL);
  sigaction(SIGCHLD, &sa, NULL);

  vector<string> path_str_vector;
  cout << "Welcome to Dragon Shell!" << endl;

  int saved_stdout = dup(STDOUT_FILENO);
  int saved_stdin = dup(STDIN_FILENO);

  while (!cin.eof())
  {
    restore_std(saved_stdout, saved_stdin);
    path_str_vector = tokenize(path_str, ":");
    cout << "dragonshell > ";

    vector<string> command_str_vector;
    string input_str;

    getline(cin, input_str);
    command_str_vector = tokenize(input_str, ";");

    for (size_t i = 0; i < command_str_vector.size(); ++i)
    {
      restore_std(saved_stdout, saved_stdin);
      vector<string> token_str_vector;
      token_str_vector = tokenize(command_str_vector[i], " ");

      if (find_pipe(token_str_vector, path_str_vector))
      {
        continue;
      }
      else
      {
        // Handle ouput redirection
        token_str_vector = redirection(token_str_vector);
        // run regular command without pipe
        run_command(token_str_vector, path_str_vector);
      }
    }
  }
  // exit gracefully (same behaviour as "exit") on ctrl+d
  graceful_exit();
  return 0;
}