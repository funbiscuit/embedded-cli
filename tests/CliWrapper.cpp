
#include "CliWrapper.h"

#include <stdexcept>

static const std::string lineEnding = "\r\n";

CliWrapper::CliWrapper(EmbeddedCli *cli, std::optional<std::unique_ptr<CLI_UINT>> buffer) {
    this->buffer = std::move(buffer);
    cli->appContext = this;
    cli->onCommand = [](EmbeddedCli *embeddedCli, CliCommand *command) {
        auto *wrapper = (CliWrapper *) embeddedCli->appContext;
        Command cmd;
        cmd.name = command->name != nullptr ? command->name : "";
        if (command->args != nullptr) {
            cmd.args.emplace_back(command->args);
        }
        wrapper->receivedCommands.push_back(cmd);
    };
    cli->writeChar = [](EmbeddedCli *embeddedCli, char c) {
        auto *wrapper = (CliWrapper *) embeddedCli->appContext;
        wrapper->txQueue.push_back(c);
    };
    this->cli = cli;
}

CliWrapper::~CliWrapper() {
    embeddedCliFree(cli);
}

void CliWrapper::addBinding(const std::string &name,
                            const std::optional<std::string> &help,
                            bool tokenizeArgs) {
    auto command = std::make_unique<BoundCommand>();
    command->name = name;
    command->help = help;
    command->tokenizeArgs = tokenizeArgs;

    CliCommandBinding binding = {
            .name = command->name.c_str(),
            .help = help.has_value() ? command->help.value().c_str() : nullptr,
            .tokenizeArgs = tokenizeArgs,
            .context = command.get(),
            .binding = [](EmbeddedCli *c, char *args, void *context) {
                auto *wrapper = (CliWrapper *) c->appContext;
                Command cmd;
                auto *boundCommand = (BoundCommand *) context;
                cmd.name = boundCommand->name;
                if (boundCommand->tokenizeArgs) {
                    // convert tokens vector of args
                    for (int i = 1; i <= embeddedCliGetTokenCount(args); ++i) {
                        auto token = embeddedCliGetToken(args, i);
                        if (token == nullptr) {
                            throw std::runtime_error("Token must not be null");
                        }
                        cmd.args.emplace_back(token);
                    }
                } else if (args != nullptr) {
                    cmd.args.emplace_back(args);
                }
                wrapper->onBoundCommand(cmd);
            },
    };

    if (embeddedCliAddBinding(cli, binding)) {
        addedBindings.push_back(std::move(command));
    }
}

std::vector<CliWrapper::Command> &CliWrapper::getCalledBindings() {
    return calledBindings;
}

CliWrapper::Display CliWrapper::getDisplay() {
    std::vector<std::string> output;

    std::string line;
    size_t cursorPosition = 0;

    for (auto c: txQueue) {
        if (c == '\b') {
            if (cursorPosition > 0 && (cursorPosition - 1) < line.size()) {
                line.erase(cursorPosition - 1, 1);
                --cursorPosition;
            }
        } else if (c == '\r') {
            cursorPosition = 0;
        } else if (c == '\n') {
            cursorPosition = 0;
            output.push_back(line);
            line.clear();
        } else {
            if (line.size() > cursorPosition) {
                line.erase(cursorPosition, 1);
            }
            line.insert(cursorPosition, 1, c);
            ++cursorPosition;
        }
    }
    output.push_back(line);

    for (auto &l: output) {
        trimStr(l);
    }

    return Display{
            .lines = output,
            .cursorColumn = cursorPosition,
    };
}

std::string CliWrapper::getRawOutput() {
    txQueue.push_back('\0');
    std::string output = txQueue.data();
    txQueue.pop_back();
    return output;
}

std::vector<CliWrapper::Command> &CliWrapper::getReceivedCommands() {
    return receivedCommands;
}

void CliWrapper::process() {
    embeddedCliProcess(cli);
}

EmbeddedCli *CliWrapper::raw() {
    return cli;
}

void CliWrapper::send(const std::string &chars) {
    for (char c: chars) {
        embeddedCliReceiveChar(cli, c);
    }
}

void CliWrapper::sendLine(const std::string &line) {
    send(line);
    send(lineEnding);
}

void CliWrapper::trimStr(std::string &str) {
    while (!str.empty() && str.back() == ' ') {
        str.pop_back();
    }
}

void CliWrapper::onBoundCommand(Command command) {
    calledBindings.push_back(std::move(command));
}

void CliWrapper::print(const std::string &text) {
    embeddedCliPrint(cli, text.c_str());
}
