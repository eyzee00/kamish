#ifndef __COMMAND__
#define __COMMAND__

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>

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
        virtual int execute(char **environPtr) = 0;
    
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
        int execute(char **environPtr) override;
        
};

#endif
