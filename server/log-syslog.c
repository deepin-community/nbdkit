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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "internal.h"
#include "open_memstream.h"
#include "syslog.h"

/* Tempted to use LOG_FTP instead of LOG_DAEMON! */
static const int PRIORITY = LOG_DAEMON|LOG_ERR;

void
log_syslog_verror (int orig_errno, const char *fs, va_list args)
{
  const char *name = threadlocal_get_name ();
  size_t instance_num = threadlocal_get_instance_num ();
  CLEANUP_FREE char *msg = NULL;
  size_t len = 0;
  FILE *fp = NULL;

  fp = open_memstream (&msg, &len);
  if (fp == NULL) {
    /* Fallback to logging using fs, args directly. */
    errno = orig_errno;      /* must restore in case fs contains %m */
    vsyslog (PRIORITY, fs, args);
    return;
  }

  if (name) {
    fprintf (fp, "%s", name);
    if (instance_num > 0)
      fprintf (fp, "[%zu]", instance_num);
    fprintf (fp, ": ");
  }

  errno = orig_errno;        /* must restore in case fs contains %m */
  vfprintf (fp, fs, args);
  close_memstream (fp);

  syslog (PRIORITY, "%s", msg);
}
