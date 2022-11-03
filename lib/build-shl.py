#!/usr/bin/python3
import datetime

SHL_TEMPLATE = """\
/**
 * This header was automatically built using
 * embedded_cli.h and embedded_cli.c
 * @date {date}
 *
{license}
 */
{api}

#ifdef EMBEDDED_CLI_IMPL
#ifndef EMBEDDED_CLI_IMPL_GUARD
#define EMBEDDED_CLI_IMPL_GUARD
#ifdef __cplusplus
extern "C" {{
#endif
{impl}
#ifdef __cplusplus
}}
#endif
#endif // EMBEDDED_CLI_IMPL_GUARD
#endif // EMBEDDED_CLI_IMPL
"""

with open('include/embedded_cli.h', 'r') as header_file, \
        open('src/embedded_cli.c', 'r') as source_file, \
        open('../LICENSE.txt', 'r') as license_file, \
        open('shl/embedded_cli.h', 'w') as output:
    build_date = "{:%Y-%m-%d}".format(datetime.date.today())

    lic = '\n'.join(map(lambda line: (" * " + line).rstrip(), license_file.read().splitlines()))
    impl = '\n'.join(filter(lambda x: not x.__contains__("embedded_cli.h"), source_file.read().splitlines()))

    output.write(SHL_TEMPLATE.format(api=header_file.read(),
                                     impl=impl,
                                     license=lic,
                                     date=build_date))
    output.close()
