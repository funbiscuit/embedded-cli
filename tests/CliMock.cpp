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

std::vector<CliMock::Command> &CliMock::getReceivedCommands() {
    return rxQueue;
}

void CliMock::onCommand(CliCommand *command) {
    Command cmd;
    cmd.name = command->name;
    cmd.args = command->args;
    rxQueue.push_back(cmd);
}

void CliMock::writeChar(char c) {
    txQueue.push_back(c);
}
