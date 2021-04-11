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

    /**
     * Adds command to a list of known commands
     * @param name
     */
    void addCommandBinding(const char *name, const char *help = nullptr);

    /**
     * Return raw output of cli
     * @return
     */
    std::string getRawOutput();

    /**
     * Returns processed output as individual lines
     * \b removes character
     * \r returns to the beginning of line (without removing chars)
     * \n moves to new line
     * @param trimBack - whether to remove spaces at the end of the line
     * @param cursorColumn - output column of cursor position
     * @param cursorRow - output row of cursor position
     * @return vector of lines
     */
    std::vector<std::string>
    getLines(bool trimBack = true, size_t *cursorColumn = nullptr, size_t *cursorRow = nullptr);

    std::vector<Command> &getReceivedCommands();

    std::vector<Command> &getReceivedKnownCommands();

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

    /**
     * Queue of commands that were received by cli and are known
     * (specified in command bindings)
     */
    std::vector<Command> rxQueueBindings;

    void onCommand(CliCommand *command);

    void onBoundCommand(Command command);

    void writeChar(char c);

    static void trimStr(std::string &str);
};


#endif //CLI_MOCK_H
