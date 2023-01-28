#ifndef EMBEDDED_CLI_CLIWRAPPER_H
#define EMBEDDED_CLI_CLIWRAPPER_H

#include <string>
#include <vector>
#include <optional>
#include <memory>

#include "embedded_cli.h"

class CliWrapper {
public:
    struct Command {
        std::string name;
        /**
         * Will contain only one element if tokenization was disabled
         */
        std::vector<std::string> args;
    };
    struct Display {
        std::vector<std::string> lines;
        size_t cursorColumn;
    };

    CliWrapper(const CliWrapper &) = delete;

    CliWrapper &operator=(const CliWrapper &) = delete;

    /**
     * Wraps given cli and takes ownership of it.
     * Will call embeddedCliFree in destructor
     * @param cli
     * @param buffer that was allocated for cli
     */
    CliWrapper(EmbeddedCli *cli, std::optional<std::unique_ptr<CLI_UINT>> buffer);

    ~CliWrapper();

    void addBinding(const std::string &name,
                    const std::optional<std::string> &help = std::nullopt,
                    bool tokenizeArgs = true);

    /**
     * Vector of all called bindings
     * @return
     */
    std::vector<Command> &getCalledBindings();

    /**
     * Returns processed output as individual lines when it is treated
     * by terminal.
     * \b removes last character
     * \r returns to the beginning of line (without removing chars)
     * \n moves to new line
     * Spaces at the end of the line are removed
     * @return displayed lines
     */
    Display getDisplay();

    /**
     * @return raw output of cli without any processing
     */
    std::string getRawOutput();

    /**
     * Vector of all received commands (from onCommand callback)
     * without called bindings
     * @return
     */
    std::vector<Command> &getReceivedCommands();

    /**
     * Prints given text via cli
     * @param text
     */
    void print(const std::string &text);

    /**
     * Let cli process received chars and call commands
     */
    void process();

    /**
     * @return actual cli, that is used inside.
     * Changes to cli must be made with caution since it might
     * disable wrapper features
     */
    EmbeddedCli *raw();

    /**
     * Send chars to cli
     * @param chars
     */
    void send(const std::string &chars);

    /**
     * Send single line to cli and finish it with CRLF
     * @param line
     */
    void sendLine(const std::string &line);

private:
    struct BoundCommand {
        std::string name;
        std::optional<std::string> help;
        bool tokenizeArgs = false;
    };

    EmbeddedCli *cli = nullptr;

    /**
     * Buffer used for CLI
     * Will be freed in destructor (if present)
     */
    std::optional<std::unique_ptr<CLI_UINT>> buffer;

    /**
     * Commands that were received by cli (from onCommand)
     * excluding binding calls
     */
    std::vector<Command> receivedCommands;

    std::vector<Command> calledBindings;

    /**
     * Queue of characters that were sent from cli and should
     * be displayed to user
     */
    std::vector<char> txQueue;

    /**
     * All bindings that are registered in cli
     */
    std::vector<std::unique_ptr<BoundCommand>> addedBindings;

    void onBoundCommand(Command command);

    static void trimStr(std::string &str);
};


#endif //EMBEDDED_CLI_CLIWRAPPER_H
