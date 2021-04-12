#include <stdlib.h>
#include <string.h>

#include "embedded_cli.h"

#define PREPARE_IMPL(t) \
  EmbeddedCliImpl* impl = (EmbeddedCliImpl*)t->_impl;

/**
 * Marks binding as candidate for autocompletion
 * This flag is updated each time getAutocompletedCommand is called
 */
#define BINDING_FLAG_AUTOCOMPLETE 1u

typedef struct EmbeddedCliImpl EmbeddedCliImpl;
typedef struct AutocompletedCommand AutocompletedCommand;
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
     * Invitation string. Is printed at the beginning of each line with user
     * input
     */
    const char *invitation;

    /**
     * We need to print initial invitation but can't do that in constructor.
     * So upon first call to process we print it and set this flag to true
     */
    bool initialInvitationPrinted;

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

    /**
     * Flags for each binding. Sizes are the same as for bindings array
     */
    uint8_t *bindingsFlags;

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

    /**
     * Current length of live autocompletion. It might be longer than cmdSize
     * so if autocompletion changes, we need to place space chars to clean up
     * previous autocompletion
     */
    uint16_t liveAutocompletionLength;

    bool wasAllocated;
};

struct AutocompletedCommand {
    /**
     * Name of autocompleted command (or first candidate for autocompletion if
     * there are multiple candidates).
     * NULL if autocomplete not possible.
     */
    const char *firstCandidate;

    /**
     * Number of characters that can be completed safely. For example, if there
     * are two possible commands "get-led" and "get-adc", then for prefix "g"
     * autocompletedLen will be 4. If there are only one candidate, this number
     * is always equal to length of the command.
     */
    size_t autocompletedLen;

    /**
     * Total number of candidates for autocompletion
     */
    uint16_t candidateCount;
};

static EmbeddedCliConfig defaultConfig;

/**
 * Number of commands that cli adds. Commands:
 * - help
 */
static const uint16_t cliInternalBindingCount = 1;

static const char *lineBreak = "\r\n";

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
 * Return autocompleted command for given prefix.
 * Prefix is compared to all known command bindings and autocompleted result
 * is returned
 * @param cli
 * @param prefix
 * @return
 */
static AutocompletedCommand getAutocompletedCommand(EmbeddedCli *cli, const char *prefix);

/**
 * Prints autocompletion result while keeping current command unchanged
 * Prints only if autocompletion is present and only one candidate exists.
 * @param cli
 */
static void printLiveAutocompletion(EmbeddedCli *cli);

/**
 * Handles autocomplete request. If autocomplete possible - fills current
 * command with autocompleted command. When multiple commands satisfy entered
 * prefix, they are printed to output.
 * @param cli
 */
static void onAutocompleteRequest(EmbeddedCli *cli);

/**
 * Removes all input from current line (replaces it with whitespaces)
 * And places cursor at the beginning of the line
 * @param cli
 */
static void clearCurrentLine(EmbeddedCli *cli);

/**
 * Write given string to cli output
 * @param cli
 * @param str
 */
static void writeToOutput(EmbeddedCli *cli, const char *str);

/**
 * Returns true if provided char is a supported control char:
 * \r, \n, \b or 0x7F (treated as \b)
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

uint16_t embeddedCliRequiredSize(EmbeddedCliConfig *config) {
    uint16_t bindingCount = config->maxBindingCount + cliInternalBindingCount;
    return sizeof(EmbeddedCli) + sizeof(EmbeddedCliImpl) +
           config->rxBufferSize * sizeof(char) +
           config->cmdBufferSize * sizeof(char) +
           bindingCount * sizeof(CliCommandBinding) +
           bindingCount * sizeof(uint8_t);
}

EmbeddedCli *embeddedCliNew(EmbeddedCliConfig *config) {
    EmbeddedCli *cli = NULL;

    uint16_t bindingCount = config->maxBindingCount + cliInternalBindingCount;

    size_t totalSize = sizeof(EmbeddedCli) + sizeof(EmbeddedCliImpl) +
                       config->rxBufferSize * sizeof(char) +
                       config->cmdBufferSize * sizeof(char) +
                       bindingCount * sizeof(CliCommandBinding) +
                       bindingCount * sizeof(uint8_t);

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
    buf = &buf[bindingCount * sizeof(CliCommandBinding)];

    impl->bindingsFlags = buf;

    impl->wasAllocated = allocated;

    impl->rxBuffer.size = config->rxBufferSize;
    impl->rxBuffer.front = 0;
    impl->rxBuffer.back = 0;
    impl->cmdMaxSize = config->cmdBufferSize;
    impl->bindingsCount = 0;
    impl->maxBindingsCount = config->maxBindingCount + cliInternalBindingCount;
    impl->lastChar = '\0';
    impl->overflow = false;
    impl->invitation = "> ";

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

    if (!impl->initialInvitationPrinted) {
        impl->initialInvitationPrinted = true;
        writeToOutput(cli, impl->invitation);
    }

    while (fifoBufAvailable(&impl->rxBuffer)) {
        char c = fifoBufPop(&impl->rxBuffer);

        if (isControlChar(c)) {
            onControlInput(cli, c);
        } else if (isDisplayableChar(c)) {
            onCharInput(cli, c);
        }

        printLiveAutocompletion(cli);

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

    // remove chars for autocompletion and live command
    clearCurrentLine(cli);

    // print provided string
    writeToOutput(cli, string);
    writeToOutput(cli, lineBreak);

    // print current command back to screen
    impl->cmdBuffer[impl->cmdSize] = '\0';
    writeToOutput(cli, impl->invitation);
    writeToOutput(cli, impl->cmdBuffer);

    printLiveAutocompletion(cli);
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
    if (tokenizedStr == NULL || pos == 0)
        return NULL;
    int i = 0;
    int tokenCount = 1;
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

uint8_t embeddedCliFindToken(const char *tokenizedStr, const char *token) {
    if (tokenizedStr == NULL || token == NULL)
        return 0;

    uint8_t size = embeddedCliGetTokenCount(tokenizedStr);
    for (int i = 0; i < size; ++i) {
        if (strcmp(embeddedCliGetToken(tokenizedStr, i + 1), token) == 0)
            return i + 1;
    }

    return 0;
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
        writeToOutput(cli, lineBreak);

        if (impl->cmdSize > 0)
            parseCommand(cli);
        impl->cmdSize = 0;

        writeToOutput(cli, impl->invitation);
    } else if ((c == '\b' || c == 0x7F) && impl->cmdSize > 0) {
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
        writeToOutput(cli, "Help is not available");
        writeToOutput(cli, lineBreak);
        return;
    }

    uint8_t tokenCount = embeddedCliGetTokenCount(tokens);
    if (tokenCount == 0) {
        for (int i = 0; i < impl->bindingsCount; ++i) {
            writeToOutput(cli, " * ");
            writeToOutput(cli, impl->bindings[i].name);
            writeToOutput(cli, lineBreak);
            if (impl->bindings[i].help != NULL) {
                cli->writeChar(cli, '\t');
                writeToOutput(cli, impl->bindings[i].help);
                writeToOutput(cli, lineBreak);
            }
        }
    } else if (tokenCount == 1) {
        // try find command
        const char *helpStr = NULL;
        const char *cmdName = embeddedCliGetToken(tokens, 1);
        bool found = false;
        for (int i = 0; i < impl->bindingsCount; ++i) {
            if (strcmp(impl->bindings[i].name, cmdName) == 0) {
                helpStr = impl->bindings[i].help;
                found = true;
                break;
            }
        }
        if (found && helpStr != NULL) {
            writeToOutput(cli, " * ");
            writeToOutput(cli, cmdName);
            writeToOutput(cli, lineBreak);
            cli->writeChar(cli, '\t');
            writeToOutput(cli, helpStr);
            writeToOutput(cli, lineBreak);
        } else if (found) {
            writeToOutput(cli, "Help is not available");
            writeToOutput(cli, lineBreak);
        } else {
            onUnknownCommand(cli, cmdName);
        }
    } else {
        writeToOutput(cli, "Command \"help\" receives one or zero arguments");
        writeToOutput(cli, lineBreak);
    }
}

static void onUnknownCommand(EmbeddedCli *cli, const char *name) {
    writeToOutput(cli, "Unknown command: \"");
    writeToOutput(cli, name);
    writeToOutput(cli, "\". Write \"help\" for a list of available commands");
    writeToOutput(cli, lineBreak);
}

static AutocompletedCommand getAutocompletedCommand(EmbeddedCli *cli, const char *prefix) {
    AutocompletedCommand cmd = {NULL, 0, 0};

    size_t prefixLen = strlen(prefix);

    PREPARE_IMPL(cli)
    if (impl->bindingsCount == 0 || prefixLen == 0)
        return cmd;


    for (int i = 0; i < impl->bindingsCount; ++i) {
        const char *name = impl->bindings[i].name;
        size_t len = strlen(name);

        // unset autocomplete flag
        impl->bindingsFlags[i] &= ~BINDING_FLAG_AUTOCOMPLETE;

        if (len < prefixLen)
            continue;

        // check if this command is candidate for autocomplete
        bool isCandidate = true;
        for (int j = 0; j < prefixLen; ++j) {
            if (prefix[j] != name[j]) {
                isCandidate = false;
                break;
            }
        }
        if (!isCandidate)
            continue;

        impl->bindingsFlags[i] |= BINDING_FLAG_AUTOCOMPLETE;

        if (cmd.candidateCount == 0 || len < cmd.autocompletedLen)
            cmd.autocompletedLen = len;

        ++cmd.candidateCount;

        if (cmd.candidateCount == 1) {
            cmd.firstCandidate = name;
            continue;
        }

        for (int j = impl->cmdSize; j < cmd.autocompletedLen; ++j) {
            if (cmd.firstCandidate[j] != name[j]) {
                cmd.autocompletedLen = j;
                break;
            }
        }
    }

    return cmd;
}

static void printLiveAutocompletion(EmbeddedCli *cli) {
    PREPARE_IMPL(cli)

    impl->cmdBuffer[impl->cmdSize] = '\0';
    AutocompletedCommand cmd = getAutocompletedCommand(cli, impl->cmdBuffer);

    if (cmd.candidateCount == 0) {
        if (impl->liveAutocompletionLength > impl->cmdSize) {
            clearCurrentLine(cli);
            writeToOutput(cli, impl->invitation);
            writeToOutput(cli, impl->cmdBuffer);
        }
        impl->liveAutocompletionLength = 0;
        return;
    }

    // print live autocompletion
    for (int i = impl->cmdSize; i < cmd.autocompletedLen; ++i) {
        cli->writeChar(cli, cmd.firstCandidate[i]);
    }
    // replace with spaces previous autocompletion
    for (int i = cmd.autocompletedLen; i < impl->liveAutocompletionLength; ++i) {
        cli->writeChar(cli, ' ');
    }
    impl->liveAutocompletionLength = cmd.autocompletedLen;
    cli->writeChar(cli, '\r');
    // print current command again so cursor is moved to initial place
    writeToOutput(cli, impl->invitation);
    writeToOutput(cli, impl->cmdBuffer);

}

static void onAutocompleteRequest(EmbeddedCli *cli) {
    PREPARE_IMPL(cli)

    impl->cmdBuffer[impl->cmdSize] = '\0';
    AutocompletedCommand cmd = getAutocompletedCommand(cli, impl->cmdBuffer);

    if (cmd.candidateCount == 0)
        return;

    if (cmd.candidateCount == 1) {
        // complete command and insert space
        for (int i = impl->cmdSize; i < cmd.autocompletedLen; ++i) {
            char c = cmd.firstCandidate[i];
            cli->writeChar(cli, c);
            impl->cmdBuffer[i] = c;
        }
        cli->writeChar(cli, ' ');
        impl->cmdBuffer[cmd.autocompletedLen] = ' ';
        impl->cmdSize = cmd.autocompletedLen + 1;
        return;
    }
    // cmd.candidateCount > 1

    // with multiple candidates we either complete to common prefix
    // or show all candidates if we already have common prefix
    if (cmd.autocompletedLen == impl->cmdSize) {
        // we need to completely clear current line since it begins with invitation
        clearCurrentLine(cli);

        for (int i = 0; i < impl->bindingsCount; ++i) {
            // autocomplete flag is set for all candidates by last call to
            // getAutocompletedCommand
            if (!(impl->bindingsFlags[i] & BINDING_FLAG_AUTOCOMPLETE))
                continue;

            const char *name = impl->bindings[i].name;

            writeToOutput(cli, name);
            writeToOutput(cli, lineBreak);
        }

        impl->cmdBuffer[impl->cmdSize] = '\0';
        writeToOutput(cli, impl->invitation);
        writeToOutput(cli, impl->cmdBuffer);
    } else {
        // complete to common prefix
        for (int i = impl->cmdSize; i < cmd.autocompletedLen; ++i) {
            char c = cmd.firstCandidate[i];
            cli->writeChar(cli, c);
            impl->cmdBuffer[i] = c;
        }
        impl->cmdSize = cmd.autocompletedLen;
    }
}

static void clearCurrentLine(EmbeddedCli *cli) {
    PREPARE_IMPL(cli)
    // how many chars are currently printed (command + live autocompletion)
    size_t len = impl->cmdSize < impl->liveAutocompletionLength ?
                 impl->liveAutocompletionLength : impl->cmdSize;
    // account for invitation string
    len += strlen(impl->invitation);

    cli->writeChar(cli, '\r');
    for (int i = 0; i < len; ++i) {
        cli->writeChar(cli, ' ');
    }
    cli->writeChar(cli, '\r');
}

static void writeToOutput(EmbeddedCli *cli, const char *str) {
    size_t len = strlen(str);

    for (int i = 0; i < len; ++i) {
        cli->writeChar(cli, str[i]);
    }
}

static bool isControlChar(char c) {
    return c == '\r' || c == '\n' || c == '\b' || c == '\t' || c == 0x7F;
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
