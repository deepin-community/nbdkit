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
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <assert.h>

#include "cleanup.h"
#include "internal.h"
#include "realpath.h"
#include "strndup.h"

#ifdef HAVE_GNUTLS

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

/* See comment in configure.ac */
#ifdef HAVE_GNUTLS_SOCKET_H
#include <gnutls/socket.h>
#endif

#ifdef HAVE_GNUTLS_TRANSPORT_IS_KTLS_ENABLED
#define TRY_KTLS 1
#else
#define TRY_KTLS 0
#endif

static int crypto_auth;
#define CRYPTO_AUTH_CERTIFICATES 1
#define CRYPTO_AUTH_PSK 2

static gnutls_certificate_credentials_t x509_creds;
static gnutls_psk_server_credentials_t psk_creds;

static void print_gnutls_error (int err, const char *fs, ...)
  ATTRIBUTE_FORMAT_PRINTF (2, 3);

static void
print_gnutls_error (int err, const char *fs, ...)
{
  va_list args;

  fprintf (stderr, "%s: GnuTLS error: ", program_name);

  va_start (args, fs);
  vfprintf (stderr, fs, args);
  va_end (args);

  fprintf (stderr, ": %s\n", gnutls_strerror (err));
}

/* Try to load certificates from 'path'.  Returns true if successful.
 * If it's not a certicate directory it returns false.  Exits on
 * other errors.
 */
static int
load_certificates (const char *path)
{
  CLEANUP_FREE char *ca_cert_filename = NULL;
  CLEANUP_FREE char *server_cert_filename = NULL;
  CLEANUP_FREE char *server_key_filename = NULL;
  CLEANUP_FREE char *ca_crl_filename = NULL;
  int err;

  if (asprintf (&ca_cert_filename, "%s/ca-cert.pem", path) == -1) {
    perror ("asprintf");
    exit (EXIT_FAILURE);
  }
  if (asprintf (&server_cert_filename, "%s/server-cert.pem", path) == -1) {
    perror ("asprintf");
    exit (EXIT_FAILURE);
  }
  if (asprintf (&server_key_filename, "%s/server-key.pem", path) == -1) {
    perror ("asprintf");
    exit (EXIT_FAILURE);
  }
  if (asprintf (&ca_crl_filename, "%s/ca-crl.pem", path) == -1) {
    perror ("asprintf");
    exit (EXIT_FAILURE);
  }

  /* Our test for a certificate directory is that ca-cert.pem,
   * server-cert.pem and server-key.pem must all exist in the path.
   */
  if (access (ca_cert_filename, R_OK) == -1)
    return 0;
  if (access (server_cert_filename, R_OK) == -1)
    return 0;
  if (access (server_key_filename, R_OK) == -1)
    return 0;

  /* Any problem past here is a hard error. */

  err = gnutls_certificate_allocate_credentials (&x509_creds);
  if (err < 0) {
    print_gnutls_error (err, "allocating credentials");
    exit (EXIT_FAILURE);
  }
  err = gnutls_certificate_set_x509_trust_file (x509_creds, ca_cert_filename,
                                                GNUTLS_X509_FMT_PEM);
  if (err < 0) {
    print_gnutls_error (err, "loading %s", ca_cert_filename);
    exit (EXIT_FAILURE);
  }

  if (access (ca_crl_filename, R_OK) == 0) {
    err = gnutls_certificate_set_x509_crl_file (x509_creds, ca_crl_filename,
                                                GNUTLS_X509_FMT_PEM);
    if (err < 0) {
      print_gnutls_error (err, "loading %s", ca_crl_filename);
      exit (EXIT_FAILURE);
    }
  }

  err = gnutls_certificate_set_x509_key_file (x509_creds,
                                              server_cert_filename,
                                              server_key_filename,
                                              GNUTLS_X509_FMT_PEM);
  if (err < 0) {
    print_gnutls_error (err, "loading server certificate and key (%s, %s)",
                        server_cert_filename, server_key_filename);
    exit (EXIT_FAILURE);
  }

  debug ("successfully loaded TLS certificates from %s", path);
  return 1;
}

static int
start_certificates (void)
{
  /* Try to locate the certificates directory and load them. */
  if (tls_certificates_dir == NULL) {
    const char *home;
    CLEANUP_FREE char *path = NULL;

#ifndef WIN32
#define RUNNING_AS_NON_ROOT_FOR_CERTIFICATES_DIR (geteuid () != 0)
#else
#define RUNNING_AS_NON_ROOT_FOR_CERTIFICATES_DIR 0
#endif
    if (RUNNING_AS_NON_ROOT_FOR_CERTIFICATES_DIR) {
      home = getenv ("HOME");
      if (home) {
        if (asprintf (&path, "%s/.pki/%s", home, PACKAGE_NAME) == -1) {
          perror ("asprintf");
          exit (EXIT_FAILURE);
        }
        if (load_certificates (path))
          goto found_certificates;
        free (path);
        if (asprintf (&path, "%s/.config/pki/%s", home, PACKAGE_NAME) == -1) {
          perror ("asprintf");
          exit (EXIT_FAILURE);
        }
        if (load_certificates (path))
          goto found_certificates;
      }
    }
    else { /* geteuid () == 0 */
      if (load_certificates (root_tls_certificates_dir))
        goto found_certificates;
    }
  }
  else {
    if (load_certificates (tls_certificates_dir))
      goto found_certificates;
  }
  return -1;

 found_certificates:
#ifdef HAVE_GNUTLS_CERTIFICATE_SET_KNOWN_DH_PARAMS
  gnutls_certificate_set_known_dh_params (x509_creds, GNUTLS_SEC_PARAM_MEDIUM);
#endif
  return 0;
}

static int
start_psk (void)
{
  int err;
  CLEANUP_FREE char *abs_psk_file = NULL;

  /* Make sure the path to the PSK file is absolute. */
  abs_psk_file = realpath (tls_psk, NULL);
  if (abs_psk_file == NULL) {
    perror (tls_psk);
    exit (EXIT_FAILURE);
  }

  err = gnutls_psk_allocate_server_credentials (&psk_creds);
  if (err < 0) {
    print_gnutls_error (err, "allocating PSK credentials");
    exit (EXIT_FAILURE);
  }

  /* Note that this function makes a copy of the string.
   * CLEANUP_FREE macro above will free abs_psk_file when
   * we return, but this is safe.
   */
  gnutls_psk_set_server_credentials_file (psk_creds, abs_psk_file);

  return 0;
}

/* Initialize crypto.  This also handles the command line parameters
 * and loading the server certificate.
 */
void
crypto_init (bool tls_set_on_cli)
{
  int err, r;
  const char *what;

  err = gnutls_global_init ();
  if (err < 0) {
    print_gnutls_error (err, "initializing GnuTLS");
    exit (EXIT_FAILURE);
  }

  if (tls == 0)                 /* --tls=off */
    return;

  /* --tls-psk overrides certificates. */
  if (tls_psk != NULL) {
    what = "Pre-Shared Keys (PSK)";
    r = start_psk ();
    if (r == 0)
      crypto_auth = CRYPTO_AUTH_PSK;
  }
  else {
    what = "X.509 certificates";
    r = start_certificates ();
    if (r == 0)
      crypto_auth = CRYPTO_AUTH_CERTIFICATES;
  }

  if (r == 0) {
    debug ("TLS enabled using: %s", what);
    return;
  }

  /* If we get here, we didn't manage to load the PSK file /
   * certificates.  If --tls=require was given on the command line
   * then that's a problem.
   */
  if (tls == 2) {               /* --tls=require */
    fprintf (stderr,
             "%s: --tls=require but could not load TLS certificates.\n"
             "Try setting ‘--tls-certificates=/path/to/certificates’ or read\n"
             "the \"TLS\" section in nbdkit(1).\n",
             program_name);
    exit (EXIT_FAILURE);
  }

  /* If --tls=on was given on the command line, warn before we turn
   * TLS off.
   */
  if (tls == 1 && tls_set_on_cli) { /* explicit --tls=on */
    fprintf (stderr,
             "%s: warning: --tls=on but could not load TLS certificates.\n"
             "TLS will be disabled and TLS connections will be rejected.\n"
             "Try setting ‘--tls-certificates=/path/to/certificates’ or read\n"
             "the \"TLS\" section in nbdkit(1).\n",
             program_name);
  }

  tls = 0;
  debug ("TLS disabled: could not load TLS certificates");
}

void
crypto_free (void)
{
  if (tls > 0) {
    switch (crypto_auth) {
    case CRYPTO_AUTH_CERTIFICATES:
      gnutls_certificate_free_credentials (x509_creds);
      break;
    case CRYPTO_AUTH_PSK:
      gnutls_psk_free_server_credentials (psk_creds);
      break;
    }
  }

  gnutls_global_deinit ();
}

/* Read buffer from GnuTLS and either succeed completely
 * (returns > 0), read an EOF (returns 0), or fail (returns -1).
 */
static int
crypto_recv (void *vbuf, size_t len)
{
  GET_CONN;
  gnutls_session_t session = conn->crypto_session;
  char *buf = vbuf;
  ssize_t r;
  bool first_read = true;

  assert (session != NULL);

  while (len > 0) {
    r = gnutls_record_recv (session, buf, len);
    if (r < 0) {
      if (r == GNUTLS_E_INTERRUPTED || r == GNUTLS_E_AGAIN)
        continue;
      nbdkit_error ("gnutls_record_recv: %s", gnutls_strerror (r));
      errno = EIO;
      return -1;
    }
    if (r == 0) {
      if (first_read)
        return 0;
      /* Partial record read.  This is an error. */
      errno = EBADMSG;
      return -1;
    }
    first_read = false;
    buf += r;
    len -= r;
  }

  return 1;
}

/* If this send()'s length is so large that it is going to require
 * multiple TCP segments anyway, there's no need to try and merge it
 * with any corked data from a previous send that used SEND_MORE.
 */
#define MAX_SEND_MORE_LEN (64 * 1024)

/* Write buffer to GnuTLS and either succeed completely
 * (returns 0) or fail (returns -1).
 */
static int
crypto_send (const void *vbuf, size_t len, int flags)
{
  GET_CONN;
  gnutls_session_t session = conn->crypto_session;
  const char *buf = vbuf;
  int err;
  ssize_t r;

  assert (session != NULL);

  if (len + gnutls_record_check_corked (session) > MAX_SEND_MORE_LEN) {
    errno = 0;
    err = gnutls_record_uncork (session, GNUTLS_RECORD_WAIT);
    if (err < 0) {
      nbdkit_error ("gnutls_record_uncork: %s", gnutls_strerror (err));
      if (errno == 0) errno = EIO;
      return -1;
    }
  }
  else if (flags & SEND_MORE)
    gnutls_record_cork (session);

  while (len > 0) {
    errno = 0;
    r = gnutls_record_send (session, buf, len);
    if (r < 0) {
      if (r == GNUTLS_E_INTERRUPTED || r == GNUTLS_E_AGAIN)
        continue;
      nbdkit_error ("gnutls_record_send: %s", gnutls_strerror (r));
      if (errno == 0) errno = EIO;
      return -1;
    }
    buf += r;
    len -= r;
  }

  if (!(flags & SEND_MORE)) {
    errno = 0;
    err = gnutls_record_uncork (session, GNUTLS_RECORD_WAIT);
    if (err < 0) {
      nbdkit_error ("gnutls_record_uncork: %s", gnutls_strerror (err));
      if (errno == 0) errno = EIO;
      return -1;
    }
  }

  return 0;
}

/* There's no place in the NBD protocol to send back errors from
 * close, so this function ignores errors.
 */
static void
crypto_close (int how)
{
  GET_CONN;
  gnutls_session_t session = conn->crypto_session;
  int sockin, sockout;

  assert (session != NULL);

  if (how == SHUT_WR)
    gnutls_bye (session, GNUTLS_SHUT_WR);
  else {
    gnutls_transport_get_int2 (session, &sockin, &sockout);

    gnutls_bye (session, GNUTLS_SHUT_RDWR);

    if (sockin >= 0)
      closesocket (sockin);
    if (sockout >= 0 && sockin != sockout)
      closesocket (sockout);

    gnutls_deinit (session);
    conn->crypto_session = NULL;
  }
}

#ifdef WIN32
/* Push/pull functions.  Only required for Windows, because on Unix we
 * can use the default send(2) and recv(2).  Note these are actually
 * calling the wrappers win_send and win_recv in windows-compat.h
 */
static ssize_t
push (gnutls_transport_ptr_t ptr, const void *buf, size_t n)
{
  ssize_t r;

  r = send ((intptr_t) ptr, buf, n, 0);
  /* XXX call gnutls_transport_set_errno here */
  return r;
}

static ssize_t
pull (gnutls_transport_ptr_t ptr, void *buf, size_t n)
{
  ssize_t r;

  r = recv ((intptr_t) ptr, buf, n, 0);
  /* XXX call gnutls_transport_set_errno here */
  return r;
}

static int
pull_timeout (gnutls_transport_ptr_t ptr, unsigned ms)
{
#if 0
  /* XXX This is what you're supposed to do, but I couldn't get it to
   * work.
   */
  return gnutls_system_recv_timeout (ptr, ms);
#endif
  return 1;
}
#endif /* WIN32 */

/* Turn GnuTLS debug messages into nbdkit debug messages
 * when nbdkit -D nbdkit.tls.log > 0
 */
NBDKIT_DLL_PUBLIC int nbdkit_debug_tls_log = 0;

static void
tls_log (int level, const char *msg)
{
  size_t len;
  CLEANUP_FREE char *copy = NULL;

  /* Strip trailing \n added by GnuTLS. */
  len = strlen (msg);
  if (len > 0 && msg[len-1] == '\n') {
    copy = strndup (msg, len-1);
    msg = copy;
  }

  debug ("gnutls: %d: %s", level, msg);
}

/* Print additional information about the session using
 * nbdkit -D nbdkit.tls.session=1
 *
 * https://gnutls.org/manual/html_node/Obtaining-session-information.html
 */
NBDKIT_DLL_PUBLIC int nbdkit_debug_tls_session = 0;

static void
debug_x509_cert (gnutls_session_t session)
{
  const gnutls_datum_t *cert_list;
  unsigned int i, cert_list_size = 0;

  cert_list = gnutls_certificate_get_peers (session, &cert_list_size);
  if (cert_list == NULL) {
    /* Note unless you use --tls-verify-peer you will always see the
     * following message.
     */
    debug ("TLS: no peer certificates found");
    return;
  }

  debug ("TLS: peer provided %u certificate(s)", cert_list_size);
  for (i = 0; i < cert_list_size; ++i) {
    int ret;
    gnutls_x509_crt_t cert;
    gnutls_datum_t cinfo;

    /* This is for debugging; best-effort is okay */
    ret = gnutls_x509_crt_init (&cert);
    if (ret != 0)
      continue;
    ret = gnutls_x509_crt_import (cert, &cert_list[i], GNUTLS_X509_FMT_DER);
    if (ret != 0) {
      gnutls_x509_crt_deinit (cert);
      continue;
    }

    ret = gnutls_x509_crt_print (cert, GNUTLS_CRT_PRINT_ONELINE, &cinfo);
    if (ret == 0) {
      debug ("TLS: %s", cinfo.data);
      gnutls_free (cinfo.data);
    }

    gnutls_x509_crt_deinit (cert);
  }
}

static void
debug_session (gnutls_session_t session)
{
  gnutls_credentials_type_t cred;
  gnutls_kx_algorithm_t kx;
  bool dhe = false, ecdh = false;
  int grp;
  const char *desc, *username, *hint;
#if TRY_KTLS
  gnutls_transport_ktls_enable_flags_t ktls_enabled;
#endif

  if (nbdkit_debug_tls_session <= 0)
    return;

  desc = gnutls_session_get_desc (session);
  if (desc) debug ("TLS session: %s", desc);

#if TRY_KTLS
  ktls_enabled = gnutls_transport_is_ktls_enabled (session);
  switch (ktls_enabled) {
  case GNUTLS_KTLS_RECV:
    debug ("TLS: kTLS enabled for receive only"); break;
  case GNUTLS_KTLS_SEND:
    debug ("TLS: kTLS enabled for send only"); break;
  case GNUTLS_KTLS_DUPLEX:
    debug ("TLS: kTLS enabled full duplex"); break;
  default:
    if ((int) ktls_enabled == 0)
      debug ("TLS: kTLS disabled");
    else
      debug ("TLS: kTLS enabled unknown setting: %d", (int) ktls_enabled);
  }
#endif

  kx = gnutls_kx_get (session);
  cred = gnutls_auth_get_type (session);
  switch (cred) {
  case GNUTLS_CRD_SRP:
    debug ("TLS: authentication: SRP (Secure Remote Password)");
#ifdef HAVE_GNUTLS_SRP_SERVER_GET_USERNAME
    username = gnutls_srp_server_get_username (session);
#else
    username = NULL;
#endif
    if (username)
      debug ("TLS: SRP session username: %s", username);
    break;
  case GNUTLS_CRD_PSK:
    debug ("TLS: authentication: PSK (Pre-Shared Key)");
    hint = gnutls_psk_client_get_hint (session);
    if (hint)
      debug ("TLS: PSK hint: %s", hint);
    username = gnutls_psk_server_get_username (session);
    if (username)
      debug ("TLS: PSK username: %s", username);
    if (kx == GNUTLS_KX_ECDHE_PSK)
      ecdh = true;
    else if (kx == GNUTLS_KX_DHE_PSK)
      dhe = true;
    break;
  case GNUTLS_CRD_ANON:
    debug ("TLS: authentication: anonymous");
    if (kx == GNUTLS_KX_ANON_ECDH)
      ecdh = true;
    else if (kx == GNUTLS_KX_ANON_DH)
      dhe = true;
    break;
  case GNUTLS_CRD_CERTIFICATE:
    debug ("TLS: authentication: certificate");
    if (gnutls_certificate_type_get (session) == GNUTLS_CRT_X509)
      debug_x509_cert (session);
    if (kx == GNUTLS_KX_DHE_RSA || kx == GNUTLS_KX_DHE_DSS)
      dhe = true;
    else if (kx == GNUTLS_KX_ECDHE_RSA || kx == GNUTLS_KX_ECDHE_ECDSA)
      ecdh = true;
    break;
  default:
    debug ("TLS: authentication: unknown (%d)", (int) cred);
  }

#ifdef HAVE_GNUTLS_GROUP_GET
  grp = gnutls_group_get (session);
#else
  grp = 0;
#endif
  if (grp) {
    debug ("TLS: negotiated group: "
#ifdef HAVE_GNUTLS_GROUP_GET_NAME
           "%s", gnutls_group_get_name (grp));
#else
    "%d", grp);
#endif
  }
  else {
    if (ecdh)
      debug ("TLS: ephemeral ECDH using curve %s",
             gnutls_ecc_curve_get_name (gnutls_ecc_curve_get (session)));
    else if (dhe)
      debug ("TLS: ephemeral DH using prime of %d bits",
             gnutls_dh_get_prime_bits (session));
  }
}

/* Upgrade an existing connection to TLS.  Also this should do access
 * control if enabled.  The protocol code ensures this function can
 * only be called once per connection.
 */
int
crypto_negotiate_tls (int sockin, int sockout)
{
  GET_CONN;
  gnutls_session_t session;
  CLEANUP_FREE char *priority = NULL;
  int err;

  /* Create the GnuTLS session. */
  err = gnutls_init (&session, GNUTLS_SERVER);
  if (err < 0) {
    nbdkit_error ("gnutls_init: %s", gnutls_strerror (err));
    return -1;
  }

  if (nbdkit_debug_tls_log > 0)
    gnutls_global_set_log_level (nbdkit_debug_tls_log);
  gnutls_global_set_log_function (tls_log);

  switch (crypto_auth) {
  case CRYPTO_AUTH_CERTIFICATES:
    /* Associate the session with the server credentials (key, cert). */
    err = gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE,
                                  x509_creds);
    if (err < 0) {
      nbdkit_error ("gnutls_credentials_set: %s", gnutls_strerror (err));
      goto error;
    }

    /* If verify peer is enabled, tell GnuTLS to request the client
     * certificates.  (Note the default is to not request or verify
     * certificates).
     */
    if (tls_verify_peer) {
      gnutls_certificate_server_set_request (session, GNUTLS_CERT_REQUEST);
      gnutls_session_set_verify_cert (session, NULL, 0);
    }

    priority = strdup (TLS_PRIORITY);
    if (priority == NULL) {
      nbdkit_error ("strdup: %m");
      goto error;
    }
    break;

  case CRYPTO_AUTH_PSK:
    /* Associate the session with the server PSK credentials. */
    err = gnutls_credentials_set (session, GNUTLS_CRD_PSK, psk_creds);
    if (err < 0) {
      nbdkit_error ("gnutls_credentials_set: %s", gnutls_strerror (err));
      goto error;
    }

    if (asprintf (&priority,
                  "%s:+ECDHE-PSK:+DHE-PSK:+PSK", TLS_PRIORITY) == -1) {
      nbdkit_error ("asprintf: %m");
      goto error;
    }
    break;

  default:
    abort ();
  }

  assert (priority != NULL);
  err = gnutls_priority_set_direct (session, priority, NULL);
  if (err < 0) {
    nbdkit_error ("failed to set TLS session priority to %s: %s",
                  priority, gnutls_strerror (err));
    goto error;
  }

  /* Set up GnuTLS so it reads and writes on the raw sockets. */
  gnutls_transport_set_int2 (session, sockin, sockout);
#ifdef WIN32
  gnutls_transport_set_push_function (session, push);
  gnutls_transport_set_pull_function (session, pull);
  gnutls_transport_set_pull_timeout_function (session, pull_timeout);
#endif

  /* Perform the handshake. */
  debug ("starting TLS handshake");
  gnutls_handshake_set_timeout (session,
                                GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);

  do {
    err = gnutls_handshake (session);
  } while (err < 0 && gnutls_error_is_fatal (err) == 0);
  if (err < 0) {
    gnutls_handshake_description_t in, out;

    /* Get some additional debug information about where in the
     * handshake protocol it failed.  You have to look up these codes in
     * <gnutls/gnutls.h>.
     */
    in = gnutls_handshake_get_last_in (session);
    out = gnutls_handshake_get_last_out (session);
    nbdkit_error ("gnutls_handshake: %s (%d/%d)",
                  gnutls_strerror (err), (int) in, (int) out);
    goto error;
  }
  debug ("TLS handshake completed");
  debug_session (session);

  /* Set up the connection recv/send/close functions so they call
   * GnuTLS wrappers instead.
   */
  conn->crypto_session = session;
  conn->recv = crypto_recv;
  conn->send = crypto_send;
  conn->close = crypto_close;
  return 0;

 error:
  gnutls_deinit (session);
  return -1;
}

/* The prototype of both gnutls_x509_crt_get_dn3 and
 * gnutls_x509_crt_get_issuer_dn3.
 */
typedef int (*get_dn3_fn) (gnutls_x509_crt_t cert, gnutls_datum_t *dn,
                           unsigned  flags);

/* Common function to call either gnutls_x509_crt_get_dn3 or
 * gnutls_x509_crt_get_issuer_dn3.
 */
static char *
get_peer_dn (const char *fn, get_dn3_fn get_dn3)
{
  GET_CONN;
  gnutls_session_t session = conn->crypto_session;
  gnutls_credentials_type_t cred;
  const gnutls_datum_t *cert_list;
  unsigned int cert_list_size = 0;
  gnutls_x509_crt_t cert = NULL;
  gnutls_datum_t dn = { 0 };
  int r;
  char *ret = NULL;

  if (!session) {
    nbdkit_debug ("nbdkit_peer_tls_dn: no TLS session");
    goto out_no_dn;
  }

  cred = gnutls_auth_get_type (session);
  if (cred != GNUTLS_CRD_CERTIFICATE) {
    nbdkit_debug ("nbdkit_peer_tls_dn: "
                  "TLS session not using certificates");
    goto out_no_dn;
  }

  if (gnutls_certificate_type_get (session) != GNUTLS_CRT_X509) {
    nbdkit_debug ("nbdkit_peer_tls_dn: "
                  "TLS session not using X.509 certificates");
    goto out_no_dn;
  }

  cert_list = gnutls_certificate_get_peers (session, &cert_list_size);
  if (cert_list == NULL || cert_list_size == 0) {
    /* This can happen if tls_verify_peer is not set.  It could also
     * happen because gnutls cannot allocate the certificate list but
     * that should be a rare error.  Return no DN in this case.
     */
    nbdkit_debug ("nbdkit_peer_tls_dn: "
                  "no client certificates (is --tls-verify-peer set?)");
    goto out_no_dn;
  }

  /* XXX According to the spec, certificates should be sent with the
   * client's certificate first, but the man page for
   * gnutls_certificate_get_peers says there are X.509 clients which
   * send them in a random order.  However it's unlikely that there
   * are any NBD clients doing this.  Anyway we only consider the
   * first certificate in the list.
   */
  r = gnutls_x509_crt_init (&cert);
  if (r != 0) {
    nbdkit_error ("gnutls_x509_crt_init: %s", gnutls_strerror (r));
    goto out;
  }
  r = gnutls_x509_crt_import (cert, &cert_list[0], GNUTLS_X509_FMT_DER);
  if (r != 0) {
    nbdkit_error ("gnutls_x509_crt_import: %s", gnutls_strerror (r));
    goto out;
  }

  r = get_dn3 (cert, &dn, 0);
  if (r != 0) {
    nbdkit_error ("%s: %s", fn, gnutls_strerror (r));
    goto out;
  }
  ret = strdup ((char *) dn.data);
  if (ret == NULL) {
    nbdkit_error ("strdup: %m");
    ret = NULL;
    goto out;
  }

 out:
  if (cert)
    gnutls_x509_crt_deinit (cert);
  if (dn.data)
    gnutls_free (dn.data);
  return ret;

  /* Not an error, but no DN for this session. */
 out_no_dn:
  if (cert)
    gnutls_x509_crt_deinit (cert);
  if (dn.data)
    gnutls_free (dn.data);
  ret = strdup ("");
  if (ret == NULL) {
    nbdkit_error ("strdup: %m");
    return NULL;
  }
  return ret;
}

NBDKIT_DLL_PUBLIC char *
nbdkit_peer_tls_dn (void)
{
  return get_peer_dn ("gnutls_x509_crt_get_dn3",
                      gnutls_x509_crt_get_dn3);
}

NBDKIT_DLL_PUBLIC char *
nbdkit_peer_tls_issuer_dn (void)
{
  return get_peer_dn ("gnutls_x509_crt_get_issuer_dn3",
                      gnutls_x509_crt_get_issuer_dn3);
}

#else /* !HAVE_GNUTLS */

/* GnuTLS was not available at compile time.  These are stub versions
 * of the above functions which either do nothing or report errors as
 * appropriate.
 */

void
crypto_init (bool tls_set_on_cli)
{
  if (tls > 0) {
    fprintf (stderr,
             "%s: TLS cannot be enabled because "
             "this binary was compiled without GnuTLS.\n",
             program_name);
    exit (EXIT_FAILURE);
  }

  tls = 0;
  debug ("TLS disabled: nbdkit was not compiled with GnuTLS support");
}

void
crypto_free (void)
{
  /* nothing */
}

int
crypto_negotiate_tls (int sockin, int sockout)
{
  /* Should never be called because tls == 0. */
  abort ();
}

NBDKIT_DLL_PUBLIC char *
nbdkit_peer_tls_dn (void)
{
  nbdkit_error ("%s is not supported on this platform",
                "nbdkit_peer_tls_dn");
  return NULL;
}

NBDKIT_DLL_PUBLIC char *
nbdkit_peer_tls_issuer_dn (void)
{
  nbdkit_error ("%s is not supported on this platform",
                "nbdkit_peer_tls_issuer_dn");
  return NULL;
}

#endif /* !HAVE_GNUTLS */
