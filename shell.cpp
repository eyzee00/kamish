#include "shell.hpp"

Shell::Shell(char **environPtr) : environ(environPtr), isRunning(false){

}

void Shell::run() {
    std::string currentLine;
    std::vector<std::string> tokenizedLine;
    this->isRunning = true;

    // The shell's main loop, will run until ctrl + D is pressed or if the user types the built-in "exit"
    while (this->isRunning) {
        // If the input is coming from stdin, then display a prompt
        if (isatty(STDIN_FILENO))
            std::cout << "$ ";

        // Read a line from the standard input, if ctrl + D is pressed, break the loop
        if (!std::getline(std::cin, currentLine))
            break;

        // Tokenize the read line into seperate tokens, ideally the first token is the name of the executable
        // If not, it's either the full path to it, or a non existant command/executable
        tokenizedLine = tokenize(currentLine);
        
        if (tokenizedLine.empty())
            continue;
        // Handle the built-in "exit" and stop the shell from running, we can just break here but what the hell
        if (!tokenizedLine.empty() && tokenizedLine[0] == "exit") {
            this->isRunning = false;
            continue;
        }
        // Relegate executing commands to the execute command function using the tokenized list of arguments
        executeCommand(tokenizedLine);
    }
}

std::vector<std::string> Shell::tokenize(const std::string &input) {
    std::vector<std::string> tokenList;

    // Create a string stream from the given input, now we have a custom cin to read from
    std::stringstream inputStream(input);
    std::string currentWord;

    // This is the interesting part, C++ does some magic behind the scenes and returns true if reading succeeds, false otherwise
    // Apparently 'operator bool()' handles how the stringstream object behaves inside a while of if statement
    // Which keeps this code simpler, we can just read, put the result in a vector, and stop when it's time to stop
    while (inputStream >> currentWord) {
        tokenList.push_back(currentWord);
    }

    return tokenList;
} 
