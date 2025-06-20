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

#ifndef NBDKIT_UTILS_H
#define NBDKIT_UTILS_H

#include <stdbool.h>

extern void shell_quote (const char *str, FILE *fp);
extern void uri_quote (const char *str, FILE *fp);
extern void c_string_quote (const char *str, FILE *fp);
extern bool is_shell_variable (const char *name);
extern int exit_status_to_nbd_error (int status, const char *cmd);
extern int set_cloexec (int fd);
extern int set_nonblock (int fd);
extern char **copy_environ (char **env, ...) __attribute__ ((__sentinel__));
extern char *make_temporary_directory (void);
extern ssize_t full_pread (int fd, void *buf, size_t count, off_t offset);
extern ssize_t full_pwrite (int fd, const void *buf, size_t count,
                            off_t offset);

#ifndef WIN32
#include <stdint.h>
#include <sys/stat.h>
extern int64_t device_size (int fd, const struct stat *statbuf);
#endif

#endif /* NBDKIT_UTILS_H */
