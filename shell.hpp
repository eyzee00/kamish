#ifndef __SHELL__
#define __SHELL__
// Standard ANSI colors
#define ANSI_COLOR_RED     "\033[1;31m"
#define ANSI_COLOR_RESET   "\033[0m"
#define ANSI_BOLD          "\033[1m"

// Readline specific markers (Tells readline "Don't count these chars for length")
#define RL_START "\001"
#define RL_END   "\002"

// The final Macros to use in your code
#define PROMPT_RED   RL_START ANSI_COLOR_RED RL_END
#define PROMPT_BOLD  RL_START ANSI_BOLD RL_END
#define PROMPT_RESET RL_START ANSI_COLOR_RESET RL_END

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sstream>
#include <sys/wait.h>
#include "command.hpp"
#include <readline/readline.h>
#include <readline/history.h>

class Shell {
    private:
        bool isRunning;
        std::vector<std::string> dirPath;
        char **environ;
        
        std::string trimInput(const std::string &input);
        std::vector<std::string> tokenize(const std::string &input);
        std::unique_ptr<Command> commandParser(std::string input);
        std::string getPrompt();
    public:
        Shell(char** environPtr);
        void run();
        void executeCommand(const std::vector<std::string> &arguments);
};

#endif
