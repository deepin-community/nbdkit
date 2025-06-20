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
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

#include <nbdkit-filter.h>

struct delay { unsigned sec, nsec; };

static struct delay delay_read_;    /* read delay */
static struct delay delay_write_;   /* write delay */
static struct delay delay_zero_;    /* zero delay */
static struct delay delay_trim_;    /* trim delay */
static struct delay delay_extents_; /* extents delay */
static struct delay delay_cache_;   /* cache delay */
static struct delay delay_open_;    /* open delay */
static struct delay delay_close_;   /* close delay */

static int delay_fast_zero = 1; /* whether delaying zero includes fast zero */

static int
parse_delay (const char *key, const char *value, struct delay *r)
{
  return nbdkit_parse_delay (key, value, &r->sec, &r->nsec);
}

static int
delay (struct delay delay, int *err)
{
  if (nbdkit_nanosleep (delay.sec, delay.nsec) == -1) {
    *err = errno;
    return -1;
  }
  return 0;
}

static int
read_delay (int *err)
{
  return delay (delay_read_, err);
}

static int
write_delay (int *err)
{
  return delay (delay_write_, err);
}

static int
zero_delay (int *err)
{
  return delay (delay_zero_, err);
}

static int
trim_delay (int *err)
{
  return delay (delay_trim_, err);
}

static int
extents_delay (int *err)
{
  return delay (delay_extents_, err);
}

static int
cache_delay (int *err)
{
  return delay (delay_cache_, err);
}

static int
open_delay (int *err)
{
  return delay (delay_open_, err);
}

/* Called for each key=value passed on the command line. */
static int
delay_config (nbdkit_next_config *next, nbdkit_backend *nxdata,
              const char *key, const char *value)
{
  if (strcmp (key, "rdelay") == 0 ||
      strcmp (key, "delay-read") == 0 ||
      strcmp (key, "delay-reads") == 0) {
    if (parse_delay (key, value, &delay_read_) == -1)
      return -1;
    return 0;
  }
  else if (strcmp (key, "wdelay") == 0) {
    if (parse_delay (key, value, &delay_write_) == -1)
      return -1;
    /* Historically wdelay set all write-related delays. */
    delay_zero_ = delay_trim_ = delay_write_;
    return 0;
  }
  else if (strcmp (key, "delay-write") == 0 ||
           strcmp (key, "delay-writes") == 0) {
    if (parse_delay (key, value, &delay_write_) == -1)
      return -1;
    return 0;
  }
  else if (strcmp (key, "delay-zero") == 0 ||
           strcmp (key, "delay-zeroes") == 0) {
    if (parse_delay (key, value, &delay_zero_) == -1)
      return -1;
    return 0;
  }
  else if (strcmp (key, "delay-trim") == 0 ||
           strcmp (key, "delay-trims") == 0 ||
           strcmp (key, "delay-discard") == 0 ||
           strcmp (key, "delay-discards") == 0) {
    if (parse_delay (key, value, &delay_trim_) == -1)
      return -1;
    return 0;
  }
  else if (strcmp (key, "delay-extent") == 0 ||
           strcmp (key, "delay-extents") == 0) {
    if (parse_delay (key, value, &delay_extents_) == -1)
      return -1;
    return 0;
  }
  else if (strcmp (key, "delay-cache") == 0) {
    if (parse_delay (key, value, &delay_cache_) == -1)
      return -1;
    return 0;
  }
  else if (strcmp (key, "delay-fast-zero") == 0) {
    delay_fast_zero = nbdkit_parse_bool (value);
    if (delay_fast_zero < 0)
      return -1;
    return 0;
  }
  else if (strcmp (key, "delay-open") == 0) {
    if (parse_delay (key, value, &delay_open_) == -1)
      return -1;
    return 0;
  }
  else if (strcmp (key, "delay-close") == 0) {
    if (parse_delay (key, value, &delay_close_) == -1)
      return -1;
    return 0;
  }
  else
    return next (nxdata, key, value);
}

#define delay_config_help \
  "rdelay=<NN>[ms]                Read delay in seconds/milliseconds.\n" \
  "delay-read=<NN>[ms]            Read delay in seconds/milliseconds.\n" \
  "delay-write=<NN>[ms]           Write delay in seconds/milliseconds.\n" \
  "delay-zero=<NN>[ms]            Zero delay in seconds/milliseconds.\n" \
  "delay-trim=<NN>[ms]            Trim delay in seconds/milliseconds.\n" \
  "delay-extents=<NN>[ms]         Extents delay in seconds/milliseconds.\n" \
  "delay-cache=<NN>[ms]           Cache delay in seconds/milliseconds.\n" \
  "wdelay=<NN>[ms]                Write, zero and trim delay in secs/msecs.\n" \
  "delay-fast-zero=<BOOL>         Delay fast zero requests (default true).\n" \
  "delay-open=<NN>[ms]            Open delay in seconds/milliseconds.\n" \
  "delay-close=<NN>[ms]           Close delay in seconds/milliseconds."

/* Override the plugin's .can_fast_zero if needed */
static int
delay_can_fast_zero (nbdkit_next *next,
                     void *handle)
{
  /* Advertise if we are handling fast zero requests locally */
  if ((delay_zero_.sec != 0 || delay_zero_.nsec != 0) && !delay_fast_zero)
    return 1;
  return next->can_fast_zero (next);
}

/* Open connection. */
static void *
delay_open (nbdkit_next_open *next, nbdkit_context *nxdata,
            int readonly, const char *exportname, int is_tls)
{
  int err;

  if (open_delay (&err) == -1) {
    errno = err;
    nbdkit_error ("delay: %m");
    return NULL;
  }

  if (next (nxdata, readonly, exportname) == -1)
    return NULL;

  return NBDKIT_HANDLE_NOT_NEEDED;
}

/* Close connection.
 *
 * We cannot call nbdkit_nanosleep here because the socket may have
 * been closed and that function will abort and return immediately.
 * However we want to force a sleep (even if the server is shutting
 * down) so use regular nanosleep instead.
 *
 * We cannot use the .close callback because that happens after the
 * socket has closed, thus not delaying the client.  By using
 * .finalize we can delay well-behaved clients (those that use
 * NBD_CMD_DISC).  We cannot delay clients that drop the connection.
 */
static int
delay_finalize (nbdkit_next *next, void *handle)
{
  if (delay_close_.sec > 0 || delay_close_.nsec > 0) {
    struct timespec ts;

    ts.tv_sec = delay_close_.sec;
    ts.tv_nsec = delay_close_.nsec;
    /* If nanosleep fails we don't really want to interrupt the chain
     * of finalize calls through the other filters, so ignore any
     * error here.
     */
    nanosleep (&ts, NULL);
  }

  return next->finalize (next);
}

/* Read data. */
static int
delay_pread (nbdkit_next *next,
             void *handle, void *buf, uint32_t count, uint64_t offset,
             uint32_t flags, int *err)
{
  if (read_delay (err) == -1)
    return -1;
  return next->pread (next, buf, count, offset, flags, err);
}

/* Write data. */
static int
delay_pwrite (nbdkit_next *next,
              void *handle,
              const void *buf, uint32_t count, uint64_t offset, uint32_t flags,
              int *err)
{
  if (write_delay (err) == -1)
    return -1;
  return next->pwrite (next, buf, count, offset, flags, err);
}

/* Zero data. */
static int
delay_zero (nbdkit_next *next,
            void *handle, uint32_t count, uint64_t offset, uint32_t flags,
            int *err)
{
  if ((flags & NBDKIT_FLAG_FAST_ZERO) &&
      (delay_zero_.sec > 0 || delay_zero_.nsec > 0) &&
      !delay_fast_zero) {
    *err = ENOTSUP;
    return -1;
  }
  if (zero_delay (err) == -1)
    return -1;
  return next->zero (next, count, offset, flags, err);
}

/* Trim data. */
static int
delay_trim (nbdkit_next *next,
            void *handle, uint32_t count, uint64_t offset,
            uint32_t flags, int *err)
{
  if (trim_delay (err) == -1)
    return -1;
  return next->trim (next, count, offset, flags, err);
}

/* Extents. */
static int
delay_extents (nbdkit_next *next,
               void *handle, uint32_t count, uint64_t offset, uint32_t flags,
               struct nbdkit_extents *extents, int *err)
{
  if (extents_delay (err) == -1)
    return -1;
  return next->extents (next, count, offset, flags, extents, err);
}

/* Cache. */
static int
delay_cache (nbdkit_next *next,
             void *handle, uint32_t count, uint64_t offset, uint32_t flags,
             int *err)
{
  if (cache_delay (err) == -1)
    return -1;
  return next->cache (next, count, offset, flags, err);
}

static struct nbdkit_filter filter = {
  .name              = "delay",
  .longname          = "nbdkit delay filter",
  .config            = delay_config,
  .config_help       = delay_config_help,
  .can_fast_zero     = delay_can_fast_zero,
  .open              = delay_open,
  .finalize          = delay_finalize,
  .pread             = delay_pread,
  .pwrite            = delay_pwrite,
  .zero              = delay_zero,
  .trim              = delay_trim,
  .extents           = delay_extents,
  .cache             = delay_cache,
};

NBDKIT_REGISTER_FILTER (filter)
