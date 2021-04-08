#include <stdlib.h>
#include <string.h>

#include "embedded_cli.h"

#define PREPARE_IMPL(t) \
  EmbeddedCliImpl* impl = (EmbeddedCliImpl*)t->_impl;


typedef struct EmbeddedCliImpl EmbeddedCliImpl;
typedef struct FifoBuf FifoBuf;

struct FifoBuf {
    char *buf;
    /**
     * Position of first element in buffer. From this position elements are taken
     */
    uint16_t front;
    /**
     * Position after last element. At this position new elements are inserted
     */
    uint16_t back;
    /**
     * Size of buffer
     */
    uint16_t size;
};

struct EmbeddedCliImpl {
    /**
     * Buffer for storing received chars.
     * Chars are stored in FIFO mode.
     */
    FifoBuf rxBuffer;

    /**
     * Buffer for current command
     */
    char *cmdBuffer;

    /**
     * Size of current command
     */
    uint16_t cmdSize;

    /**
     * Total size of command buffer
     */
    uint16_t cmdMaxSize;

    CliCommandBinding *bindings;

    uint16_t bindingsCount;

    uint16_t maxBindingsCount;

    /**
     * Stores last character that was processed.
     */
    char lastChar;

    /**
     * Indicates that rx buffer overflow happened. In such case last command
     * that wasn't finished (no \r or \n were received) will be discarded
     */
    bool overflow;

    bool wasAllocated;
};

static EmbeddedCliConfig defaultConfig;

/**
 * Number of commands that cli adds. Commands:
 * - help
 */
static const uint16_t cliInternalBindingCount = 1;

/**
 * Process input character. Character is valid displayable char and should be
 * added to current command string and displayed to client.
 * @param cli
 * @param c
 */
static void onCharInput(EmbeddedCli *cli, char c);

/**
 * Process control character (like \r or \n) possibly altering state of current
 * command or executing onCommand callback.
 * @param cli
 * @param c
 */
static void onControlInput(EmbeddedCli *cli, char c);

/**
 * Parse command in buffer and execute callback
 * @param cli
 */
static void parseCommand(EmbeddedCli *cli);

/**
 * Setup bindings for internal commands, like help
 * @param cli
 */
static void initInternalBindings(EmbeddedCli *cli);

/**
 * Show help for given tokens (or default help if no tokens)
 * @param cli
 * @param tokens
 * @param context - not used
 */
static void onHelp(EmbeddedCli *cli, char *tokens, void *context);

/**
 * Show error about unknown command
 * @param cli
 * @param name
 */
static void onUnknownCommand(EmbeddedCli *cli, const char *name);

/**
 * Handles autocomplete request. If autocomplete possible - fills current
 * command with autocompleted command. When multiple commands satisfy entered
 * prefix, they are printed to output.
 * @param cli
 */
static void onAutocompleteRequest(EmbeddedCli *cli);

/**
 * Write given string to cli output
 * @param cli
 * @param str
 */
static void writeToOutput(EmbeddedCli *cli, const char *str);

/**
 * Returns true if provided char is a supported control char:
 * \r, \n or \b
 * @param c
 * @return
 */
static bool isControlChar(char c);

/**
 * Returns true if provided char is a valid displayable character:
 * a-z, A-Z, 0-9, whitespace, punctuation, etc.
 * Currently only ASCII is supported
 * @param c
 * @return
 */
static bool isDisplayableChar(char c);

/**
 * How many elements are currently available in buffer
 * @param buffer
 * @return number of elements
 */
static uint16_t fifoBufAvailable(FifoBuf *buffer);

/**
 * Return first character from buffer and remove it from buffer
 * Buffer must be non-empty, otherwise 0 is returned
 * @param buffer
 * @return
 */
static char fifoBufPop(FifoBuf *buffer);

/**
 * Push character into fifo buffer. If there is no space left, character is
 * discarded and false is returned
 * @param buffer
 * @param a - character to add
 * @return true if char was added to buffer, false otherwise
 */
static bool fifoBufPush(FifoBuf *buffer, char a);

EmbeddedCliConfig *embeddedCliDefaultConfig(void) {
    defaultConfig.rxBufferSize = 64;
    defaultConfig.cmdBufferSize = 64;
    defaultConfig.cliBuffer = NULL;
    defaultConfig.cliBufferSize = 0;
    defaultConfig.maxBindingCount = 8;
    return &defaultConfig;
}

EmbeddedCli *embeddedCliNew(EmbeddedCliConfig *config) {
    EmbeddedCli *cli = NULL;

    size_t totalSize = sizeof(EmbeddedCli) + sizeof(EmbeddedCliImpl) +
                       config->rxBufferSize * sizeof(char) +
                       config->cmdBufferSize * sizeof(char) +
                       config->maxBindingCount * sizeof(CliCommandBinding) +
                       cliInternalBindingCount * sizeof(CliCommandBinding);

    bool allocated = false;
    if (config->cliBuffer == NULL) {
        config->cliBuffer = malloc(totalSize);
        if (config->cliBuffer == NULL)
            return NULL;
        allocated = true;
    } else if (config->cliBufferSize < totalSize) {
        return NULL;
    }

    uint8_t *buf = config->cliBuffer;

    memset(buf, 0, totalSize);

    cli = (EmbeddedCli *) buf;
    buf = &buf[sizeof(EmbeddedCli)];

    cli->_impl = (EmbeddedCliImpl *) buf;
    buf = &buf[sizeof(EmbeddedCliImpl)];

    PREPARE_IMPL(cli)
    impl->rxBuffer.buf = (char *) buf;
    buf = &buf[config->rxBufferSize * sizeof(char)];

    impl->cmdBuffer = (char *) buf;
    buf = &buf[config->cmdBufferSize * sizeof(char)];

    impl->bindings = (CliCommandBinding *) buf;

    impl->wasAllocated = allocated;

    impl->rxBuffer.size = config->rxBufferSize;
    impl->rxBuffer.front = 0;
    impl->rxBuffer.back = 0;
    impl->cmdMaxSize = config->cmdBufferSize;
    impl->bindingsCount = 0;
    impl->maxBindingsCount = config->maxBindingCount + cliInternalBindingCount;
    impl->lastChar = '\0';
    impl->overflow = false;

    initInternalBindings(cli);

    return cli;
}

EmbeddedCli *embeddedCliNewDefault(void) {
    return embeddedCliNew(embeddedCliDefaultConfig());
}

void embeddedCliReceiveChar(EmbeddedCli *cli, char c) {
    PREPARE_IMPL(cli)

    if (!fifoBufPush(&impl->rxBuffer, c)) {
        impl->overflow = true;
    }
}

void embeddedCliProcess(EmbeddedCli *cli) {
    PREPARE_IMPL(cli)

    while (fifoBufAvailable(&impl->rxBuffer)) {
        char c = fifoBufPop(&impl->rxBuffer);

        if (isControlChar(c)) {
            onControlInput(cli, c);
        } else if (isDisplayableChar(c)) {
            onCharInput(cli, c);
        }

        impl->lastChar = c;
    }

    // discard unfinished command if overflow happened
    if (impl->overflow) {
        impl->cmdSize = 0;
        impl->overflow = false;
    }
}

bool embeddedCliAddBinding(EmbeddedCli *cli, CliCommandBinding binding) {
    PREPARE_IMPL(cli)
    if (impl->bindingsCount == impl->maxBindingsCount)
        return false;

    impl->bindings[impl->bindingsCount] = binding;

    ++impl->bindingsCount;
    return true;
}

void embeddedCliPrint(EmbeddedCli *cli, const char *string) {
    PREPARE_IMPL(cli)

    size_t len = strlen(string);
    size_t padLen = len >= impl->cmdSize ? 0 : impl->cmdSize - len;

    // remove all chars for current command from screen
    for (int i = 0; i < impl->cmdSize; ++i) {
        if (i < padLen) {
            cli->writeChar(cli, '\b');
            cli->writeChar(cli, ' ');
        }
        cli->writeChar(cli, '\b');
    }

    // print provided string
    writeToOutput(cli, string);
    writeToOutput(cli, "\r\n");

    // print current command back to screen
    impl->cmdBuffer[impl->cmdSize] = '\0';
    writeToOutput(cli, impl->cmdBuffer);
}

void embeddedCliFree(EmbeddedCli *cli) {
    PREPARE_IMPL(cli)
    if (impl->wasAllocated) {
        // allocation is done in single call to malloc, so need only single free
        free(cli);
    }
}

void embeddedCliTokenizeArgs(char *args) {
    if (args == NULL)
        return;

    // for now only space, but can add more later
    const char *separators = " ";
    size_t len = strlen(args);
    // place extra null char to indicate end of tokens
    args[len + 1] = '\0';

    if (len == 0)
        return;

    // replace all separators with \0 char
    for (int i = 0; i < len; ++i) {
        if (strchr(separators, args[i]) != NULL) {
            args[i] = '\0';
        }
    }

    // compress all sequential null-chars to single ones, starting from end

    size_t nextTokenStartIndex = 0;
    size_t i = len;
    while (i > 0) {
        --i;
        bool isSeparator = strchr(separators, args[i]) != NULL;

        if (!isSeparator && args[i + 1] == '\0') {
            // found end of token, move tokens on the right side of this one
            if (nextTokenStartIndex != 0 && nextTokenStartIndex - i > 2) {
                // will copy all tokens to the right and two null-chars
                memmove(&args[i + 2], &args[nextTokenStartIndex], len - nextTokenStartIndex + 1);
            }
        } else if (isSeparator && args[i + 1] != '\0') {
            nextTokenStartIndex = i + 1;
        }
    }

    // remove null chars from the beginning
    if (args[0] == '\0' && nextTokenStartIndex > 0) {
        memmove(args, &args[nextTokenStartIndex], len - nextTokenStartIndex + 1);
    }
}

const char *embeddedCliGetToken(const char *tokenizedStr, uint8_t pos) {
    if (tokenizedStr == NULL)
        return NULL;
    int i = 0;
    int tokenCount = 0;
    while (true) {
        if (tokenCount == pos)
            break;

        if (tokenizedStr[i] == '\0') {
            ++tokenCount;
            if (tokenizedStr[i + 1] == '\0')
                break;
        }

        ++i;
    }

    if (tokenizedStr[i] != '\0')
        return &tokenizedStr[i];
    else
        return NULL;
}

uint8_t embeddedCliGetTokenCount(const char *tokenizedStr) {
    if (tokenizedStr == NULL || tokenizedStr[0] == '\0')
        return 0;

    int i = 0;
    int tokenCount = 1;
    while (true) {
        if (tokenizedStr[i] == '\0') {
            if (tokenizedStr[i + 1] == '\0')
                break;
            ++tokenCount;
        }
        ++i;
    }

    return tokenCount;
}

static void onCharInput(EmbeddedCli *cli, char c) {
    PREPARE_IMPL(cli)

    // have to reserve two extra chars for command ending (used in tokenization)
    if (impl->cmdSize + 2 >= impl->cmdMaxSize)
        return;

    impl->cmdBuffer[impl->cmdSize] = c;
    ++impl->cmdSize;

    if (cli->writeChar != NULL)
        cli->writeChar(cli, c);
}

static void onControlInput(EmbeddedCli *cli, char c) {
    PREPARE_IMPL(cli)

    // process \r\n and \n\r as single \r\n command
    if ((impl->lastChar == '\r' && c == '\n') ||
        (impl->lastChar == '\n' && c == '\r'))
        return;

    if (c == '\r' || c == '\n') {
        cli->writeChar(cli, '\r');
        cli->writeChar(cli, '\n');

        if (impl->cmdSize > 0)
            parseCommand(cli);
        impl->cmdSize = 0;
    } else if (c == '\b' && impl->cmdSize > 0) {
        // remove char from screen
        cli->writeChar(cli, '\b');
        cli->writeChar(cli, ' ');
        cli->writeChar(cli, '\b');
        // and from buffer
        --impl->cmdSize;
    } else if (c == '\t') {
        onAutocompleteRequest(cli);
    }

}

static void parseCommand(EmbeddedCli *cli) {
    PREPARE_IMPL(cli)

    char *cmdName = NULL;
    char *cmdArgs = NULL;
    bool nameFinished = false;

    // find command name and command args inside command buffer
    for (int i = 0; i < impl->cmdSize; ++i) {
        char c = impl->cmdBuffer[i];

        if (c == ' ') {
            // all spaces between name and args are filled with zeros
            // so name is a correct null-terminated string
            if (cmdArgs == NULL)
                impl->cmdBuffer[i] = '\0';
            if (cmdName != NULL)
                nameFinished = true;

        } else if (cmdName == NULL) {
            cmdName = &impl->cmdBuffer[i];
        } else if (cmdArgs == NULL && nameFinished) {
            cmdArgs = &impl->cmdBuffer[i];
        }
    }

    // we keep two last bytes in cmd buffer reserved so cmdSize is always by 2
    // less than cmdMaxSize
    impl->cmdBuffer[impl->cmdSize] = '\0';
    impl->cmdBuffer[impl->cmdSize + 1] = '\0';

    if (cmdName == NULL)
        return;

    // try to find command in bindings
    for (int i = 0; i < impl->bindingsCount; ++i) {
        if (strcmp(cmdName, impl->bindings[i].name) == 0) {
            if (impl->bindings[i].binding == NULL)
                break;

            if (impl->bindings[i].tokenizeArgs)
                embeddedCliTokenizeArgs(cmdArgs);
            impl->bindings[i].binding(cli, cmdArgs, impl->bindings[i].context);
            return;
        }
    }

    // command not found in bindings or binding was null
    // try to call default callback
    if (cli->onCommand != NULL) {
        CliCommand command;
        command.name = cmdName;
        command.args = cmdArgs;

        cli->onCommand(cli, &command);
    } else {
        onUnknownCommand(cli, cmdName);
    }
}

static void initInternalBindings(EmbeddedCli *cli) {
    PREPARE_IMPL(cli)

    CliCommandBinding b = {
            "help",
            "Print list of commands",
            true,
            NULL,
            onHelp
    };
    embeddedCliAddBinding(cli, b);
}

static void onHelp(EmbeddedCli *cli, char *tokens, void *context) {
    PREPARE_IMPL(cli)

    if (impl->bindingsCount == 0) {
        writeToOutput(cli, "Help is not available\r\n");
        return;
    }

    uint8_t tokenCount = embeddedCliGetTokenCount(tokens);
    if (tokenCount == 0) {
        writeToOutput(cli, "Available commands:\r\n");
        for (int i = 0; i < impl->bindingsCount; ++i) {
            writeToOutput(cli, impl->bindings[i].name);
            if (impl->bindings[i].help != NULL) {
                writeToOutput(cli, ": ");
                writeToOutput(cli, impl->bindings[i].help);
            }
            writeToOutput(cli, "\r\n");
        }
    } else if (tokenCount == 1) {
        // try find command
        const char *helpStr = NULL;
        const char *cmdName = embeddedCliGetToken(tokens, 0);
        bool found = false;
        for (int i = 0; i < impl->bindingsCount; ++i) {
            if (strcmp(impl->bindings[i].name, cmdName) == 0) {
                helpStr = impl->bindings[i].help;
                found = true;
                break;
            }
        }
        if (found && helpStr != NULL) {
            writeToOutput(cli, cmdName);
            writeToOutput(cli, ": ");
            writeToOutput(cli, helpStr);
            writeToOutput(cli, "\r\n");
        } else if (found) {
            writeToOutput(cli, "No help is available for command \"");
            writeToOutput(cli, cmdName);
            writeToOutput(cli, "\"\r\n");
        } else {
            onUnknownCommand(cli, cmdName);
        }
    } else {
        writeToOutput(cli, "Command \"help\" receives one or zero arguments\r\n");
    }
}

static void onUnknownCommand(EmbeddedCli *cli, const char *name) {
    writeToOutput(cli, "Unknown command: \"");
    writeToOutput(cli, name);
    writeToOutput(cli, "\". Write \"help\" for a list of available commands\r\n");
}

static void onAutocompleteRequest(EmbeddedCli *cli) {
    PREPARE_IMPL(cli)

    if (impl->bindingsCount == 0 || impl->cmdSize == 0)
        return;


    const char *autocomplete = NULL;
    uint16_t candidateCount = 0;
    // how many chars we can complete
    size_t autocompleteLen = 0;


    for (int i = 0; i < impl->bindingsCount; ++i) {
        const char *cmd = impl->bindings[i].name;

        size_t len = strlen(cmd);

        if (len < impl->cmdSize)
            continue;


        // check if this command is candidate for autocomplete
        bool isCandidate = true;
        for (int j = 0; j < impl->cmdSize; ++j) {
            if (impl->cmdBuffer[j] != cmd[j]) {
                isCandidate = false;
                break;
            }
        }
        if (!isCandidate)
            continue;

        if (candidateCount == 0 || len < autocompleteLen)
            autocompleteLen = len;

        ++candidateCount;

        if (candidateCount == 1) {
            autocomplete = cmd;
            continue;
        }

        for (int j = impl->cmdSize; j < autocompleteLen; ++j) {
            if (autocomplete[j] != cmd[j]) {
                autocompleteLen = j;
                break;
            }
        }
    }

    if (candidateCount == 1) {
        // complete command and insert space
        for (int i = impl->cmdSize; i < autocompleteLen; ++i) {
            char c = autocomplete[i];
            cli->writeChar(cli, c);
            impl->cmdBuffer[i] = c;
        }
        cli->writeChar(cli, ' ');
        impl->cmdBuffer[autocompleteLen] = ' ';
        impl->cmdSize = autocompleteLen + 1;
    } else if (candidateCount > 1) {
        // with multiple candidates we either complete to common prefix
        // or show all candidates if we already have common prefix
        if (autocompleteLen == impl->cmdSize) {
            bool firstPrinted = false;

            for (int i = 0; i < impl->bindingsCount; ++i) {
                const char *cmd = impl->bindings[i].name;
                size_t len = strlen(cmd);
                if (len < impl->cmdSize)
                    continue;

                // check if this command is candidate for autocomplete
                bool isCandidate = true;
                for (int j = 0; j < impl->cmdSize; ++j) {
                    if (impl->cmdBuffer[j] != cmd[j]) {
                        isCandidate = false;
                        break;
                    }
                }
                if (!isCandidate)
                    continue;

                if (firstPrinted) {
                    writeToOutput(cli, cmd);
                } else {
                    firstPrinted = true;
                    // cmd begins with current cmdBuffer, so just print remaining
                    writeToOutput(cli, &cmd[impl->cmdSize]);
                }

                writeToOutput(cli, "\r\n");
            }

            impl->cmdBuffer[impl->cmdSize] = '\0';
            writeToOutput(cli, impl->cmdBuffer);
        } else {
            // complete to common prefix
            for (int i = impl->cmdSize; i < autocompleteLen; ++i) {
                char c = autocomplete[i];
                cli->writeChar(cli, c);
                impl->cmdBuffer[i] = c;
            }
            impl->cmdSize = autocompleteLen;
        }
    }
}

static void writeToOutput(EmbeddedCli *cli, const char *str) {
    size_t len = strlen(str);

    for (int i = 0; i < len; ++i) {
        cli->writeChar(cli, str[i]);
    }
}

static bool isControlChar(char c) {
    return c == '\r' || c == '\n' || c == '\b' || c == '\t';
}

static bool isDisplayableChar(char c) {
    return (c >= 32 && c <= 126);
}

static uint16_t fifoBufAvailable(FifoBuf *buffer) {
    if (buffer->back >= buffer->front)
        return buffer->back - buffer->front;
    else
        return buffer->size - buffer->front + buffer->back;
}

static char fifoBufPop(FifoBuf *buffer) {
    char a = '\0';
    if (buffer->front != buffer->back) {
        a = buffer->buf[buffer->front];
        buffer->front = (buffer->front + 1) % buffer->size;
    }
    return a;
}

static bool fifoBufPush(FifoBuf *buffer, char a) {
    uint32_t newBack = (buffer->back + 1) % buffer->size;
    if (newBack != buffer->front) {
        buffer->buf[buffer->back] = a;
        buffer->back = newBack;
        return true;
    }
    return false;
}
