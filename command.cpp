#include "command.hpp"
#include <cstdlib>

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

simpleCommand::simpleCommand(const std::vector<std::string> &arguments) : argumentList(arguments) {

}

int simpleCommand::execute(char **environ) {
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
