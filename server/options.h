/* nbdkit
 * Copyright Red Hat
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * * Neither the name of Red Hat nor the names of its contributors may be
 * used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY RED HAT AND CONTRIBUTORS ''AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RED HAT OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef NBDKIT_OPTIONS_H
#define NBDKIT_OPTIONS_H

#include <stdbool.h>
#include <getopt.h>
#include <limits.h>
#include <string.h>

enum {
  HELP_OPTION = CHAR_MAX + 1,
  DUMP_CONFIG_OPTION,
  DUMP_PLUGIN_OPTION,
  EXIT_WITH_PARENT_OPTION,
  FILTER_OPTION,
  KEEPALIVE_OPTION,
  LOG_OPTION,
  LONG_OPTIONS_OPTION,
  MASK_HANDSHAKE_OPTION,
  NO_MC_OPTION,
  NO_SR_OPTION,
  PRINT_URI,
  RUN_OPTION,
  SELINUX_LABEL_OPTION,
  SHORT_OPTIONS_OPTION,
  SWAP_OPTION,
  TIMEOUT_OPTION,
  TLS_OPTION,
  TLS_CERTIFICATES_OPTION,
  TLS_PSK_OPTION,
  TLS_VERIFY_PEER_OPTION,
  VSOCK_OPTION,
};

static const char *short_options = "46D:e:fg:i:nop:P:rst:u:U:vV";
static const struct option long_options[] = {
  { "ipv4-only",        no_argument,       NULL, '4' },
  { "ipv6-only",        no_argument,       NULL, '6' },
  { "debug",            required_argument, NULL, 'D' },
  { "dump-config",      no_argument,       NULL, DUMP_CONFIG_OPTION },
  { "dump-plugin",      no_argument,       NULL, DUMP_PLUGIN_OPTION },
  { "exit-with-parent", no_argument,       NULL, EXIT_WITH_PARENT_OPTION },
  { "export",           required_argument, NULL, 'e' },
  { "export-name",      required_argument, NULL, 'e' },
  { "exportname",       required_argument, NULL, 'e' },
  { "filter",           required_argument, NULL, FILTER_OPTION },
  { "foreground",       no_argument,       NULL, 'f' },
  { "no-fork",          no_argument,       NULL, 'f' },
  { "group",            required_argument, NULL, 'g' },
  { "help",             no_argument,       NULL, HELP_OPTION },
  { "ip-addr",          required_argument, NULL, 'i' },
  { "ipaddr",           required_argument, NULL, 'i' },
  { "keepalive",        no_argument,       NULL, KEEPALIVE_OPTION },
  { "keep-alive",       no_argument,       NULL, KEEPALIVE_OPTION },
  { "log",              required_argument, NULL, LOG_OPTION },
  { "long-options",     no_argument,       NULL, LONG_OPTIONS_OPTION },
  { "mask-handshake",   required_argument, NULL, MASK_HANDSHAKE_OPTION },
  { "new-style",        no_argument,       NULL, 'n' },
  { "newstyle",         no_argument,       NULL, 'n' },
  { "no-mc",            no_argument,       NULL, NO_MC_OPTION },
  { "no-meta-contexts", no_argument,       NULL, NO_MC_OPTION },
  { "no-sr",            no_argument,       NULL, NO_SR_OPTION },
  { "no-structured-replies", no_argument,  NULL, NO_SR_OPTION },
  { "old-style",        no_argument,       NULL, 'o' },
  { "oldstyle",         no_argument,       NULL, 'o' },
  { "pid-file",         required_argument, NULL, 'P' },
  { "pidfile",          required_argument, NULL, 'P' },
  { "print-uri",        no_argument,       NULL, PRINT_URI },
  { "print-url",        no_argument,       NULL, PRINT_URI },
  { "port",             required_argument, NULL, 'p' },
  { "read-only",        no_argument,       NULL, 'r' },
  { "readonly",         no_argument,       NULL, 'r' },
  { "run",              required_argument, NULL, RUN_OPTION },
  { "selinux-label",    required_argument, NULL, SELINUX_LABEL_OPTION },
  { "short-options",    no_argument,       NULL, SHORT_OPTIONS_OPTION },
  { "show-uri",         no_argument,       NULL, PRINT_URI },
  { "show-url",         no_argument,       NULL, PRINT_URI },
  { "single",           no_argument,       NULL, 's' },
  { "stdin",            no_argument,       NULL, 's' },
  { "swap",             no_argument,       NULL, SWAP_OPTION },
  { "threads",          required_argument, NULL, 't' },
  { "timeout",          required_argument, NULL, TIMEOUT_OPTION },
  { "time-out",         required_argument, NULL, TIMEOUT_OPTION },
  { "tls",              required_argument, NULL, TLS_OPTION },
  { "tls-certificates", required_argument, NULL, TLS_CERTIFICATES_OPTION },
  { "tls-psk",          required_argument, NULL, TLS_PSK_OPTION },
  { "tls-verify-peer",  no_argument,       NULL, TLS_VERIFY_PEER_OPTION },
  { "unix",             required_argument, NULL, 'U' },
  { "user",             required_argument, NULL, 'u' },
  { "verbose",          no_argument,       NULL, 'v' },
  { "version",          no_argument,       NULL, 'V' },
  { "vsock",            no_argument,       NULL, VSOCK_OPTION },
  { NULL },
};

/* Is it a plugin or filter name relative to the plugindir/filterdir? */
static inline bool
is_short_name (const char *filename)
{
  const size_t n = strlen (filename);
  size_t i;

  for (i = 0; i < n; ++i) {
    switch (filename[i]) {
    case '/': case '\\':        /* directory separators */
    case ':': case ';':         /* path separators */
    case ' ':
    case '.':
    case ',':
    case '=':                   /* nbdkit parameter separator */
      return false;

      /* non-printable ASCII */
    case 127:
      return false;
    default:
      if ((unsigned)filename[i] <= 31)
        return false;
    }
  }

  return strstr (filename, DIR_SEPARATOR_STR) == NULL;
}

#endif /* NBDKIT_OPTIONS_H */
