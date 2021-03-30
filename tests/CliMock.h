#ifndef CLI_MOCK_H
#define CLI_MOCK_H

#include <string>
#include <vector>

#include "embedded_cli.h"

/**
 * Simple class to mock CLI
 */
class CliMock {
public:
    struct Command {
        std::string name;
        std::string args;
    };

    explicit CliMock(EmbeddedCli *cli);

    /**
     * Send single line to cli, finish it with "\r\n"
     * @param line
     */
    void sendLine(const std::string &line);

    /**
     * Send single line to cli, without line ending
     * @param line
     */
    void sendStr(const std::string &line);

    std::vector<Command> &getReceivedCommands();

private:
    EmbeddedCli *cli;

    std::string lineEnding = "\r\n";

    /**
     * Queue of characters that were sent from cli
     */
    std::vector<char> txQueue;

    /**
     * Queue of commands that were received by cli
     */
    std::vector<Command> rxQueue;

    void onCommand(CliCommand *command);

    void writeChar(char c);
};


#endif //CLI_MOCK_H
