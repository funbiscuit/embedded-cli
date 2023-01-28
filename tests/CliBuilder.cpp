
#include "CliBuilder.h"

#include <stdexcept>

CliBuilder::CliBuilder() {
    this->config = embeddedCliDefaultConfig();
}

CliBuilder &CliBuilder::autocomplete(bool enabled) {
    this->config->enableAutoComplete = enabled;
    return *this;
}

CliWrapper CliBuilder::build() {
    std::optional<std::unique_ptr<CLI_UINT>> buffer = std::nullopt;

    if (useStatic) {
        auto minSize = embeddedCliRequiredSize(this->config);
        auto *buf = new CLI_UINT[BYTES_TO_CLI_UINTS(minSize)];
        buffer = std::optional<std::unique_ptr<CLI_UINT>>{buf};
        this->config->cliBuffer = buf;
        this->config->cliBufferSize = minSize;
    }

    EmbeddedCli *cli = embeddedCliNew(this->config);
    if (cli == nullptr) {
        throw std::runtime_error("Expected non-null cli pointer");
    }
    return {cli, std::move(buffer)};
}

CliBuilder &CliBuilder::invitation(const char *text) {
    this->config->invitation = text;
    return *this;
}

CliBuilder &CliBuilder::staticAllocation() {
    this->useStatic = true;
    return *this;
}
