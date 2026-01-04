#include "shell.hpp"
#include "command.hpp"
#include <limits.h> // For PATH_MAX

Shell::Shell(char **environPtr) : environ(environPtr){

}


std::string Shell::getPrompt() {
    char cwd[PATH_MAX];
    
    // 1. Get current working directory

    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::string path(cwd);
        
        // 2. Shorten Home Directory to "~"
        // (Optional polish: makes the prompt look cleaner)
        const char* home = getenv("HOME");
        if (home) {
            std::string homeDir(home);
            if (path.find(homeDir) == 0) {
                path.replace(0, homeDir.length(), "~");
            }

        }

        // 3. Build the fancy string
        // Format: [BOLD Path] [RED Signature]$ 
        return std::string(PROMPT_RED) + path + " " + 

               std::string(PROMPT_RESET) + "$: ";
    }

    
    // Fallback if getcwd fails
    return "kamish$ ";
}

void Shell::run() {
    this->isRunning = true;

    // The shell's main loop, will run until ctrl + D is pressed or if the user types the built-in "exit"
    while (this->isRunning) {
        std::string prompt = this->getPrompt();
        char *cInput = readline(prompt.c_str());

        if (!cInput) {
            std::cout << "Terminated" << std::endl;
            break;
        }

        std::string input(cInput);

        if (!input.empty()) {
            add_history(input.c_str());
        }

        free(cInput);
        std::unique_ptr<Command> currentCommand = this->commandParser(input);
        if (!this->isRunning || !currentCommand)
            continue;

        int status = currentCommand->execute(this->environ);
    }

    clear_history();
    #if defined(HAVE_READLINE) || defined (__linux__)
    rl_clear_history();
    rl_free_undo_list();
    #endif
}
std::vector<std::string> Shell::tokenize(const std::string &input) {
    std::vector<std::string> tokens;
    std::string currentToken;
    
    bool inQuotes = false;
    char quoteChar = 0;


    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];


        if (inQuotes) {
            // STATE: INSIDE QUOTES

            if (c == quoteChar) {
                // Found the closing quote. Switch back to Normal.
                inQuotes = false;
                // Note: We do NOT add 'c' to currentToken. This effectively strips the quote!
            } else {
                // Inside quotes, spaces are just text.
                currentToken += c;
            }
        } 
        else {
            // STATE: NORMAL (OUTSIDE QUOTES)
            if (c == '"' || c == '\'') {
                // Found an opening quote. Switch to Quote Mode.
                inQuotes = true;
                quoteChar = c;
            } 
            else if (std::isspace(c)) {
                // Found a space outside quotes. This ends the token.
                if (!currentToken.empty()) {
                    tokens.push_back(currentToken);
                    currentToken.clear();
                }
            } 
            else {
                // Just a normal letter/number
                currentToken += c;
            }
        }
    }

    // Push the final token (if the string didn't end with a space)
    if (!currentToken.empty()) {
        tokens.push_back(currentToken);
    }

    return tokens;
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
    // If we find ";", this means we have a composite command on our hands
    size_t colonPos = trimmedInput.find(";");
    if (colonPos != std::string::npos) {
        // We divide the input into left of the ";" and the right of it using substr() and andPos
        std::string leftString = trimmedInput.substr(0, colonPos);
        std::string rightString = trimmedInput.substr(colonPos + 1);

        // We call the function recursively on the left and right children
        // This is the part where each individual command is built, each call keeps dividing the input into two
        // Until we are left with and elementary command
        std::unique_ptr<Command> leftCommand = commandParser(leftString);
        std::unique_ptr<Command> rightCommand = commandParser(rightString);

        // Allocate memory on the heap for an sequenceCommand object, use move semantics to move ownership from the left/rightCommand to the constructor
        // The constructor in turn moves the ownership from itself to the instance private members
        sequenceCommand *rawSequencePtr = new sequenceCommand(std::move(leftCommand), std::move(rightCommand));
        std::unique_ptr<Command> genericCmdPtr(rawSequencePtr);

        return genericCmdPtr;
    }

    // If we find "&&", this means we have an AND command on our hands
    size_t andPos = trimmedInput.find("&&");
    if (andPos != std::string::npos) {
        // We divide the input into left of the "&&" and the right of it using substr() and andPos
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
    
    // If we find "||", this means we have an OR command on our hands
    size_t orPos = trimmedInput.find("||");
    if (orPos != std::string::npos) {
        // We divide the input into left of the "||" and the right of it using substr() and andPos
        std::string leftString = trimmedInput.substr(0, orPos);
        std::string rightString = trimmedInput.substr(orPos + 2);
        
        // We call the function recursively on the left and right children
        // This is the part where each individual command is built, each call keeps dividing the input into two
        // Until we are left with and elementary command
        std::unique_ptr<Command> leftCommand = commandParser(leftString);
        std::unique_ptr<Command> rightCommand = commandParser(rightString);

        // Allocate memory on the heap for an orCommand object, use move semantics to move ownership from the left/rightCommand to the constructor
        // The constructor in turn moves the ownership from itself to the instance private members
        orCommand *rawOrPtr = new orCommand(std::move(leftCommand), std::move(rightCommand));

        // Wrap the created object by a generic command unique pointer to delegate memory management
        // And also makes use of the abstract Command class, makes working with different types of commands extremely easier
        std::unique_ptr<Command> genericCmdPtr(rawOrPtr);
        return genericCmdPtr;
    }

    // If we find "|", this means we have a pipe command on our hands
    size_t pipePos = trimmedInput.find("|");
    if (pipePos != std::string::npos) {
        // We divide the input into left of the "|" and the right of it using substr() and andPos
        std::string leftString = trimmedInput.substr(0, pipePos);
        std::string rightString = trimmedInput.substr(pipePos + 1);

        // We call the function recursively on the left and right children
        // This is the part where each individual command is built, each call keeps dividing the input into two
        // Until we are left with and elementary command
        std::unique_ptr<Command> leftCommand = commandParser(leftString);
        std::unique_ptr<Command> rightCommand = commandParser(rightString);

        // Allocate memory on the heap for a pipeCommand object, use move semantics to move ownership from the left/rightCommand to the constructor
        // The constructor in turn moves the ownership from itself to the instance private members
        pipeCommand *rawPipePtr = new pipeCommand(std::move(leftCommand), std::move(rightCommand));

        // Wrap the created object by a generic command unique pointer to delegate memory management
        // And also makes use of the abstract Command class, makes working with different types of commands extremely easier
        std::unique_ptr<Command> genericCmdPtr(rawPipePtr);

        return genericCmdPtr;
    }

    // If we find ">>", this means we have a redirect and append command on our hands
    size_t appendPos = trimmedInput.find(">>");
    if (appendPos != std::string::npos) {
        // We divide the input into left of the "|" and the right of it using substr() and andPos
        std::string leftString = trimmedInput.substr(0, appendPos);
        std::string fileName = trimmedInput.substr(appendPos + 2);

        std::unique_ptr<Command> resultingCommand = commandParser(leftString);
        fileName = this->trimInput(fileName);

        // Allocate memory on the heap for a redirectCommand object, use move semantics to move ownership from the left/rightCommand to the constructor
        // The constructor in turn moves the ownership from itself to the instance private members
        redirectCommand *rawRedirectPtr = new redirectCommand(std::move(resultingCommand), fileName, "append");

        // Wrap the created object by a generic command unique pointer to delegate memory management
        // And also makes use of the abstract Command class, makes working with different types of commands extremely easier
        std::unique_ptr<Command> genericCmdPtr(rawRedirectPtr);
        return genericCmdPtr;
    }
    

    // If we find ">", this means we have a redirect and truncate command on our hands
    size_t truncPos = trimmedInput.find(">");
    if (truncPos != std::string::npos) {
        // We divide the input into left of the ">" and the right of it using substr() and andPos
        std::string leftString = trimmedInput.substr(0, truncPos);
        std::string fileName = trimmedInput.substr(truncPos + 1);

        fileName = this->trimInput(fileName);
        std::unique_ptr<Command> resultingCommand = commandParser(leftString);

        // Allocate memory on the heap for a redirectCommand object, use move semantics to move ownership from the left/rightCommand to the constructor
        // The constructor in turn moves the ownership from itself to the instance private members
        redirectCommand *rawRedirectPtr = new redirectCommand(std::move(resultingCommand), fileName, "trunc");

        // Wrap the created object by a generic command unique pointer to delegate memory management
        // And also makes use of the abstract Command class, makes working with different types of commands extremely easier
        std::unique_ptr<Command> genericCmdPtr(rawRedirectPtr);
        return genericCmdPtr;
    }
    
    // If we find "<", this means we have a redirect and read command on our hands
    size_t readPos = trimmedInput.find("<");
    if (readPos != std::string::npos) {
        // We divide the input into left of the "<" and the right of it using substr() and andPos
        std::string leftString = trimmedInput.substr(0, readPos);
        std::string fileName = trimmedInput.substr(readPos + 1);

        fileName = this->trimInput(fileName);
        std::unique_ptr<Command> resultingCommand = commandParser(leftString);

        // Allocate memory on the heap for a redirectCommand object, use move semantics to move ownership from the left/rightCommand to the constructor
        // The constructor in turn moves the ownership from itself to the instance private members
        redirectCommand *rawRedirectPtr = new redirectCommand(std::move(resultingCommand), fileName, "read");

        // Wrap the created object by a generic command unique pointer to delegate memory management
        // And also makes use of the abstract Command class, makes working with different types of commands extremely easier
        std::unique_ptr<Command> genericCmdPtr(rawRedirectPtr);
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


