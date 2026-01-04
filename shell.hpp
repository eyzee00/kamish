#ifndef __SHELL__
#define __SHELL__

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sstream>
#include <sys/wait.h>
#include "command.hpp"

class Shell {
    private:
        bool isRunning;
        std::vector<std::string> dirPath;
        char **environ;
        
        std::string trimInput(const std::string &input);
        std::vector<std::string> tokenize(const std::string &input);
        std::unique_ptr<Command> commandParser(std::string input);
    public:
        Shell(char** environPtr);
        void run();
        void executeCommand(const std::vector<std::string> &arguments);
};

#endif
