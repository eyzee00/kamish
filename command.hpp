#ifndef __COMMAND__
#define __COMMAND__

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <memory>
#include <sstream>
#include <fcntl.h>

/*
 * Abstract Command class, the contract that each type of command should adhere to
 */
class Command{
    public:
        // This insures that the children's destructor runs instead
        // this is an abstract class, implementing a destructor here doesn't make any sense.
        virtual ~Command() = default;
        
        // The start of a great inheritence chain, by making this function virtual, we force all the children to implement their own version
        // Which gives us the opportunity to implement different types of commands, e.g. simple commands, logically connected commands...
        virtual int execute(char **environPtr, bool shouldFork = true) = 0;
    
    // Give all types of commands to resolve the absolute path of a given executable
    protected:
        std::string getAbsolutePath(const std::string &executableName);
};

/*
 * simpleCommand - the simplest type of command, not connected by any type of logical or pipping operation
 */
class simpleCommand : public Command {
    // Each command has its list of arguments, private and can only be set by the constructor
    // This way we don't need to pass the arguments to the execute function, it can access them directly
    private:
        std::vector<std::string> argumentList;

    // Adhere to the abstract class: Construct the command from its tokenized arguments
    // Define the custom execute function, again, adhering to the contract
    public:
        simpleCommand(const std::vector<std::string> &arguments);
        int execute(char **environPtr, bool shouldFork) override;
        
};

/*
 * andCommand - logically connected commands with the "&&" operator
 */
class andCommand : public Command {
    // Each andCommand can have any type of command to its left and to its right
    // Unique pointers make this significantly easier, now we can delegate the execution to the proper execute() function
    // Meaning, if the left command is just a simple command, it will run its execute() function
    private:
        std::unique_ptr<Command> leftChild;
        std::unique_ptr<Command> rightChild;
    
    // The parser will handle building the commands, and as usual we override the virtual function to adhere to the contract
    public:
        andCommand(std::unique_ptr<Command> leftCommand, std::unique_ptr<Command> rightCommand);
        int execute(char **environPtr, bool shouldFork) override;
};

/*
 * pipeCommand - for commands connected with a pipe '|'
 */

class pipeCommand : public Command {
    // Each pipeCommand can have any type of command to its left and to its right
    // Unique pointers make this significantly easier, now we can delegate the execution to the proper execute() function
    // Meaning, if the left command is an AND command, it will run its execute() function
    private:
        std::unique_ptr<Command> leftChild;
        std::unique_ptr<Command> rightChild;

    // The parser will handle building the commands, and as usual we override the virtual function to adhere to the contract
    public:
        pipeCommand(std::unique_ptr<Command> leftCommand, std::unique_ptr<Command> rightCommand);
        int execute(char **environPtr, bool shouldFork) override;
};

/*
 * redirectCommand - for commands connected with a redirect ">>", ">" or "<"
 */
class redirectCommand : public Command {
    private:
        std::unique_ptr<Command> command;
        std::string fileName;
        std::string type;

    public:
        redirectCommand(std::unique_ptr<Command> givenCommand, const std::string &givenFileName, const std::string &redirectType);
        int execute(char **environPtr, bool shouldFork) override;
};

/*
 * sequenceCommand - for commands chained by ";"
 */
class sequenceCommand : public Command {
    private:
        std::unique_ptr<Command> leftChild;
        std::unique_ptr<Command> rightChild;
    
    public:
        sequenceCommand(std::unique_ptr<Command> leftCommand, std::unique_ptr<Command> rightCommand);
        int execute(char **environPtr, bool shouldFork) override;
};

/*
 * orCommand - for commands connected by the OR symbol "||"
 */
class orCommand : public Command {
    private:
        std::unique_ptr<Command> leftChild;
        std::unique_ptr<Command> rightChild;

    public:
        orCommand(std::unique_ptr<Command> leftCommand, std::unique_ptr<Command> rightCommand);
        int execute(char **environPtr, bool shouldFork) override;
};

#endif
