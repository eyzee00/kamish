#include "command.hpp"


/*----------------Abstract Command Class-------------------------------*/
/*
 * getAbsolutePath - Gets the full path of a given executable if found
 * If not found it returns the given executable name
 */
std::string Command::getAbsolutePath(const std::string &executableName) {
    // If the given argument is already an absolute path, or relative path, just return it
    if (executableName.find('/') != std::string::npos)
        return executableName;

    // Use getenv() to get the value of "PATH" environment variable
    char *pathEnviron = std::getenv("PATH");
    // If for some reason was not set, nothing to do, return the executableName
    if (!pathEnviron)
        return executableName;
    
    // Else make a C++ style string variable out of the PATH, and turn it into a stream to tokenize it later
    std::string path = pathEnviron;
    std::string fullPath, currentDir;
    std::stringstream streamedPath(path);

    // Tokenize the stream using getline, and set the delimiter to ':' to treat each directory independently
    while (std::getline(streamedPath, currentDir, ':')) {
        // Build the full path using the read PATH directory, and the executable name
        fullPath = currentDir + "/" + executableName;

        // Use the C access function to check if the file exists in the built path
        // If so return the full path, else, keep looking
        if (!access(fullPath.c_str(), X_OK))
            return fullPath;
    }
    // If we reach this statement, the executable doesn't exist in none of the default PATH directories
    return executableName;
}


/*----------------simpleCommand Class-------------------------------*/

simpleCommand::simpleCommand(const std::vector<std::string> &arguments) : argumentList(arguments) {

}

int simpleCommand::execute(char **environ, bool shouldFork) {
    // Get the full path for the executable if possible
    this->argumentList[0] = getAbsolutePath(this->argumentList[0]);
    // Create a C style vector since execve() doesn't understand C++ style strings
    std::vector<char *> cStyleArgs;
    
    // Loop through the argument list
    for (const auto &argument : this->argumentList) {
        // This line seems complicated, but what it does is convert the C++ arguments(string) into C style string (char *)
        // then store them in the C style vector.
        // c_str() effectively turns them back to (const char *), and const_cast<char *> removes the read only restriction
        cStyleArgs.push_back(const_cast<char *>(argument.c_str()));
    }
    // After this line and the previous loop are done, we effectively have a vector of C style pointers to characters (char *)
    // Which is almost exactly what execve() expects (char *argv[])
    cStyleArgs.push_back(nullptr);

    if (shouldFork) {
        // Start the child process by calling fork()
        pid_t pid = fork();

        // If fork() failed, print an error and return -1 to the shell
        if (pid == -1) {
            perror("Fork failed");
            return -1;
        }
        
        // fork() returns 0 to the child process
        if (!pid) {
            // Call execve(), cStyleArgs.data() turns vector<char *> argv to char *argv[]
            execve(cStyleArgs[0], cStyleArgs.data(), environ);
            
            // execve() never returns, if we reach this line, execve() must've failed
            // This process must be killed to prevent to shells from running at the same time
            perror("Execve Failed");
            exit(EXIT_FAILURE);
        }

        // fork() returns the child's PID to the parent
        else {
            int status;
            // Pause the current process until the child process finishes to avoid having them both run at the same time
            waitpid(pid, &status, 0);
            // If the child process exited naturally, return the status code to the shell
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            // If not, return -1 to signify it to the shell
            else {
                return -1;
            }
        }
    }
    else {
        execve(cStyleArgs[0], cStyleArgs.data(), environ);

        perror("Execve Failed");
        exit(EXIT_FAILURE);
    }
    return -1;
}

/*----------------andCommand Class-------------------------------*/
andCommand::andCommand(std::unique_ptr<Command> leftCommand, std::unique_ptr<Command> rightCommand) 
    : leftChild(std::move(leftCommand)), rightChild(std::move(rightCommand)) {

}
/*
 * andCommand execute function
 * Makes use of the power of both unique_ptr and the polymorphic execute function
 * execute can trigger twice or more depending on the children, what matters is that the execute function is smart enough to tell
 */
int andCommand::execute(char **environPtr, bool shouldFork) {
    // Store the status of the first child execution
    int status = this->leftChild->execute(environPtr, true);

    // Execute the right child only if the left has succeded
    if (status == 0)
        status = this->rightChild->execute(environPtr, true);
    
    // Will return the second child's status if both have executed, if the first child failed, it will return its status instead
    return status;
}

/*----------------andCommand Class-------------------------------*/
pipeCommand::pipeCommand(std::unique_ptr<Command> leftCommand, std::unique_ptr<Command> rightCommand)
    : leftChild(std::move(leftCommand)), rightChild(std::move(rightCommand)) {

}


/*
 * pipeCommand execute function
 * The harderst one to implement, we have to redirect the standard output of the left child to the standard input of the right child
 * we will make use of pipe() to create a pipe, and dup2() to redirect input and output
 * We have to be careful since we will be forking twice, and we have to keep track of the status of each child to avoid zombies or pipe leaks
 */
int pipeCommand::execute(char **environPtr, bool shouldFork) {
    // Create a new pipe that we will use to redirect output from the right child to the left one
    int newPipe[2];

    // Use pipe() to create a new pipe, newPipe[0] is where the right child will read from, newPipe[1] is where the left child will write to
    if (pipe(newPipe) == -1) {
        perror("Pipe Creation Failed");
        return -1;
    }

    // Start by forking and creating the left child process
    pid_t leftProc = fork();

    if (leftProc == -1) {
        perror("Fork Failure");
        return -1;
    }

    // If this is the left child
    if (leftProc == 0) {

        // dup2() basically renames/overwrites the standard output of te left child to use the pipe we created
        // But because we have two ends to write to (the overwritten STDOUT, and newPipe[1]) we close newPipe[1]
        // It's not an arbitrary choice, the left child knows only to write to STDOUT, so we trick it by overwriting STDOUT
        dup2(newPipe[1], STDOUT_FILENO);
        close(newPipe[0]);
        close(newPipe[1]);
        
        
        // When we call execute on the right child, and if its a simple command for example
        // Its execute function will fork again, the resulting child will never return if successful
        // But this child will return, because it's the parent process relative to the execute call()
        exit(this->leftChild->execute(environPtr, false));
    }
    
    // Back to the parent, we fork again to create the right child process
    pid_t rightProc = fork();

    if (rightProc == -1) {
        perror("Fork Failure");
        return -1;
    }
    
    // If this is the right child
    if (rightProc == 0) {

        // dup2() basically renames/overwrites the standard input of te right child to use the pipe we created
        // But because we have two ends to read from (the overwritten STDIN, and newPipe[0]) we close newPipe[0]
        // It's not an arbitrary choice, the right child knows only to write to STDIN, so we trick it by overwriting STDIN
        dup2(newPipe[0], STDIN_FILENO);
        close(newPipe[0]);
        close(newPipe[1]);
        
        exit(this->rightChild->execute(environPtr, false));
    }

    // Back to the parent child again
    int leftStatus, rightStatus;

    // Crucial, we must close the pipes here or the right child will be waiting forever for input coming from the left child process
    close(newPipe[0]);
    close(newPipe[1]);

    // Wait for both children to finish to avoid creating two shells / or three
    waitpid(leftProc, &leftStatus, 0);
    waitpid(rightProc, &rightStatus, 0);
    
    // If the right status has exited succesfully, return the status, which will be 0 after conversion
    if (WIFEXITED(rightStatus)) {
        return WEXITSTATUS(rightStatus);
    }
    // Else, return -1
    else {
        return -1;
    }
}

/*----------------redirectCommand Class-------------------------------*/

redirectCommand::redirectCommand(std::unique_ptr<Command> givenCommand, const std::string &givenFileName, const std::string &redirectType) 
    : command(std::move(givenCommand)), fileName(givenFileName), type(redirectType) {

}
/*
 * Another tough function to implement
 * Handles the redirect command type, makes use syscalls like open() and dup2() to overwrite either the standard input or output
 * This execute version and the simple command's execute are optimized to only fork if needed:
 * Meaning, that if this function was called by the pipeCommand execute function which already forks, this function catches on
 * That way, we avoid forking twice for the same command, although it is not that serious, depends on what command we execute
 */
int redirectCommand::execute(char **environPtr, bool shouldFork) {
    // Create a direction flag that tells us if we replace STDIN or STDOUT
    int fileDescriptor = -1;
    int direction = -1;

    // Open the file in the corresponding mode depending on the type of the redirect command
    if (this->type == "trunc") {
        fileDescriptor = open(this->fileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        direction = 1;
    }
    else if(this->type == "append") {
        fileDescriptor = open(this->fileName.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        direction = 1;
    }
    else if (this->type == "read") {
        fileDescriptor = open(this->fileName.c_str(), O_RDONLY, 0644);
        direction = 0;
    }
    else {
        return -1;
    }

    // If the file descriptor is still -1, this means opening the file failed
    if (fileDescriptor == -1) {
        perror("Error opening the file");
        return -1;
    }

    // This is where we use the trick, hang on
    pid_t childPID;
    // If shouldFork is true, which is the default, we fork as usual
    // For example; if this execute function was called in AND execute chain, shouldFork would be true
    if (shouldFork) {
        childPID = fork();
        if (childPID == -1) {
            perror("Failed to fork");
            return -1;

        }
    }
    // If shouldFork is false, this means we forked elsewhere, and until now, we only for in the simpleCommand and the pipeCommand
    // This function can't be called by the simpleCommand's execute, since it's a simpleCommand, go figure...
    // Since pipeCommand's execute() forks for each child
    // we account for that, and masquerade as the childProcess, since, this is a child process if shouldFork is false
    else {
        childPID = 0;
    }

    // If this is the child...
    if (!childPID) {
        // If we need to read from the file, we replace STDIN by the given file
        if (!direction)
            dup2(fileDescriptor, STDIN_FILENO);
        // If we need to write to the file, we replace STDOUT by the given file
        else
            dup2(fileDescriptor, STDOUT_FILENO);

        // Close the file since it was already duplicated
        close(fileDescriptor);

        // Execute the command, and exit with its status
        exit(this->command->execute(environPtr, false));
    }
    
    // Back to the parent, we close the file, so the child doesn't hang, if its reading
    close(fileDescriptor);
    int status = 0;
    // Only wait if we have forked, if we didn't, another process is waiting, so keep it moving
    if (shouldFork)
        waitpid(childPID, &status, 0);

    // Return the exit status if it was successful
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else
        return -1;
}
