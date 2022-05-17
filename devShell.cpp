#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
using namespace std;

#define NC "\e[0m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\033[33m"

#define MAXSIZE 50
#define READ 0
#define WRITE 1
char* args[MAXSIZE + 1];

// function to get input from console
void takeInput(string& input) {
  cout << YEL << endl;

  if (fork() == 0) {
    execlp("pwd", "pwd", NULL);
  } else {
    wait(NULL);
  }
  cout << GRN << "> " << NC;

  getline(cin, input);
}

// simple tokenizer
void tokenize(string input) {
  char* token = strtok((char*)(input.c_str()), " ");

  int i = 0;
  while (token != NULL) {
    args[i++] = token;
    token = strtok(NULL, " ");
  }
  args[i] = NULL;
}

// function to check for type of redirection and get file name
string getRedirectFile(string& input, int& type) {
  int fd;
  string fileName;
  int read = input.find("<");
  int write = input.find(">");

  if (read != -1) {
    fileName = input.substr(read + 1);
    input = input.substr(0, read);
  } else if (write != -1) {
    fileName = input.substr(write + 1);
    input = input.substr(0, write);
  }

  if (read != -1)
    type = READ;
  else if (write != -1)
    type = WRITE;

  return fileName;
}

// get 1 token (left side of pipe if it exists)
string getToken(string& input, string& token, int& type) {
  int isPipe = input.find("|");

  // if pipe exists then give me the left side and truncate the input
  if (isPipe != -1) {
    token = input.substr(0, isPipe);
    input = input.substr(isPipe + 1);
  }

  // else there is no pipe (there's only one command) so give me the whole input
  else {
    token = input;
    input = "";
  }
  string _file;

  // check for any redirection, get redirect file
  if (token.find(">") || token.find("<")) {
    _file = getRedirectFile(token, type);
  }
  return _file;
}

void executeCommand(string input) {
  string token;
  int file, type;
  char* filename;
  string _filename;

  int readBackup = 0;

  int fd[2];

  while (true) {
    type = -1;

    // pipe
    if (pipe(fd) == -1) {
      cout << "Pipe failed" << endl;
    };

    // get 1 token (while check if there is redirection)
    _filename = getToken(input, token, type);
    filename = (char*)_filename.c_str();
    // if token is empty (no command to execute) break
    if (!token.length()) {
      break;
    }

    int pid = fork();

    if (pid == 0) {
      // tokenize the command to execute
      tokenize(token);

      // there is some redirection

      // write
      if (type == WRITE) {
        file = open(filename, O_WRONLY | O_CREAT, 0777);
        dup2(readBackup, READ);
        dup2(file, WRITE);
        close(file);
      }

      // read
      else if (type == READ) {
        file = open(filename, O_RDONLY, 0777);
        dup2(file, READ);
        close(file);

        if (input.length()) {
          dup2(fd[WRITE], WRITE);
        }
      }

      // if no redirection
      else {
        // read 1st time from stdin then read from backup (read end of pipe)
        dup2(readBackup, READ);

        // if there are commands left to execute, keep writing to pipe
        if (input.length()) {
          dup2(fd[WRITE], WRITE);
        }
      }

      close(fd[READ]);
      close(fd[WRITE]);

      // execute
      if (execvp(args[0], args) == -1) {
        cout << RED "Command not found!" << NC << endl;
      }

    }

    // parent
    else if (pid > 0) {
      wait(NULL);
      close(fd[WRITE]);

      // assign backup for next read
      readBackup = fd[READ];
    } else {
      // fork failed
      cout << RED "Fork failed" << NC << endl;
    }
  }

  // close pipes if left
  close(fd[WRITE]);
  close(fd[READ]);
}

// function to execute cd command
bool checkForCD(string input) {
  int isCD = input.find("cd");
  if (isCD != -1) {
    tokenize(input);

    // cd case
    if (args[1] == NULL) {
      chdir("/");
    }

    // ~ case
    else if (strcmp(args[1], "~") == 0) {
      chdir(getenv("HOME"));
    }

    // else
    else if (chdir(args[1]) == -1) {
      cout << RED "Invalid Directory!" << NC << endl;
    }

    return true;
  }
  return false;
}

int main() {
  string input;

  cout << "\nWelcome to " << GRN << "devShell!\n" << NC;
  cout << " Enter your command or enter exit to "
          "exit.\n"
       << endl;

  while (true) {
    takeInput(input);
    if (input == "exit") {
      break;
    } else if (checkForCD(input)) {
      continue;
    }

    executeCommand(input);
  }

  return 0;
}