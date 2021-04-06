#include "CliMock.h"

CliMock::CliMock(EmbeddedCli *cli) : cli(cli) {
    cli->appContext = this;
    cli->onCommand = [](EmbeddedCli *embeddedCli, CliCommand *command) {
        auto *mock = (CliMock *) embeddedCli->appContext;
        mock->onCommand(command);
    };
    cli->writeChar = [](EmbeddedCli *embeddedCli, char c) {
        auto *mock = (CliMock *) embeddedCli->appContext;
        mock->writeChar(c);
    };
}

void CliMock::sendLine(const std::string &line) {
    sendStr(line);
    sendStr(lineEnding);
}

void CliMock::sendStr(const std::string &str) {
    for (char c : str) {
        embeddedCliReceiveChar(cli, c);
    }
}

void CliMock::addCommandBinding(const char *name, const char *help) {
    CliCommandBinding binding;
    binding.name = name;
    binding.help = help;
    binding.tokenizeArgs = false;
    binding.context = (void *) name;
    binding.binding = [](EmbeddedCli *c, char *args, void *context) {
        auto *mock = (CliMock *) c->appContext;
        Command cmd;
        cmd.name = (const char *) context;
        cmd.args = args != nullptr ? args : "";
        mock->onBoundCommand(cmd);
    };
    bindings.push_back(binding);

    embeddedCliSetBindings(cli, bindings.data(), bindings.size());
}

std::string CliMock::getRawOutput() {
    txQueue.push_back('\0');
    std::string output = txQueue.data();
    txQueue.pop_back();
    return output;
}

std::string CliMock::getOutput() {
    std::vector<char> output;

    for (auto c : txQueue) {
        if (c == '\b') {
            if (!output.empty())
                output.pop_back();
        } else {
            output.push_back(c);
        }
    }
    output.push_back('\0');
    return output.data();
}

std::vector<CliMock::Command> &CliMock::getReceivedCommands() {
    return rxQueue;
}

std::vector<CliMock::Command> &CliMock::getReceivedKnownCommands() {
    return rxQueueBindings;
}

void CliMock::onCommand(CliCommand *command) {
    Command cmd;
    cmd.name = command->name != nullptr ? command->name : "";
    cmd.args = command->args != nullptr ? command->args : "";
    rxQueue.push_back(cmd);
}

void CliMock::onBoundCommand(Command command) {
    rxQueueBindings.push_back(std::move(command));
}

void CliMock::writeChar(char c) {
    txQueue.push_back(c);
}
