#include "shell.hpp"
#include "command.hpp"

Shell::Shell(char **environPtr) : environ(environPtr){

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

        std::unique_ptr<Command> currentCommand = this->commandParser(currentLine);
        if (!this->isRunning || !currentCommand)
            continue;

        int status = currentCommand->execute(this->environ);
        std::cout << "Last process exited with status code: " << status << std::endl;
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

/*
 * trimInput - trims leading and trailing spaces from the given input
 */
std::string Shell::trimInput(const std::string &input) {
    // Use find_first_not_of() to find the first character in the string that is not a space
    size_t firstCharPos = input.find_first_not_of(' ');

    // If we couldn't find any, the input is just a series of spaces, we return an empty string to signal it
    if (firstCharPos == std::string::npos)
        return "";

    // Use find_last_not_of() to find the last character in the string that is not a space
    size_t lastCharPos = input.find_last_not_of(' ');

    // Use substr(), the index of the first character and the index of the last character to trim the leading and trailing spaces
    // substr() takes the length of the string as a second argument, naturally the length of the trimmed input is last - first + 1
    std::string trimmedInput = input.substr(firstCharPos, (lastCharPos - firstCharPos + 1));
    return trimmedInput;
}

/*
 * commandParser - The GOD function in this whole project
 * It's a recursive descent parser, divides and conquers, like all the great leaders
 * How it works: It creates an Abstract Syntax Tree from the given input
 * It starts by finding the root, the root can be any command connector, and divides the string into two
 * The left child: which is guaranteed to be a simple command
 * The right child: which is what is right of the connector, it can be a simple command, or series of connected commands
 * It recursively calls itself on both children until it hits the base case, when it does, it is guaranteed to be a lone simple command
 * Then, we build the command from its arguments
 */
std::unique_ptr<Command> Shell::commandParser(std::string input) {
    // We start by trimming the input from leading and trailing spaces
    std::string trimmedInput = this->trimInput(input);
    
    // If the trimmed command is empty, well no command to parse
    if (trimmedInput.empty())
        return nullptr;

    // If the user wants to exit, get him the fuck out
    if (trimmedInput == "exit") {
        this->isRunning = false;
        return nullptr;
    }

    // This is the interesting part, stay with me now
    // If we find "&&", this means we have a composite command on our hands
    size_t andPos = trimmedInput.find("&&");
    if (andPos != std::string::npos) {
        //We divide the input into left of the "&&" and the right of it using substr() and andPos
        std::string leftString = trimmedInput.substr(0, andPos);
        std::string rightString = trimmedInput.substr(andPos + 2);

        // We call the function recursively on the left and right children
        // This is the part where each individual command is built, each call keeps dividing the input into two
        // Until we are left with and elementary command
        std::unique_ptr<Command> leftCommand = commandParser(leftString);
        std::unique_ptr<Command> rightCommand = commandParser(rightString);

        // Allocate memory on the heap for an andCommand object, use move semantics to move ownership from the left/rightCommand to the constructor
        // The constructor in turn moves the ownership from itself to the instance private members
        andCommand *rawAndPtr = new andCommand(std::move(leftCommand), std::move(rightCommand));

        // Wrap the created object by a generic command unique pointer to delegate memory management
        // And also makes use of the abstract Command class, makes working with different types of commands extremely easier
        std::unique_ptr<Command> genericCmdPtr(rawAndPtr);
        return genericCmdPtr;
    }
    else {
        // This is our base case, each recursive call leads to this
        // We build a simple command from the tokenized input
        simpleCommand *rawSimplePtr = new simpleCommand(this->tokenize(trimmedInput));
        
        // And just like before, we wrap the simpleCommand by a generic unique pointer, consistent programming!
        std::unique_ptr<Command> genericCmdPtr(rawSimplePtr);
        return genericCmdPtr;
    }
}


