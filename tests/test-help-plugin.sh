#!/usr/bin/env bash
# nbdkit
# Copyright Red Hat
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# * Neither the name of Red Hat nor the names of its contributors may be
# used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY RED HAT AND CONTRIBUTORS ''AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RED HAT OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

source ./functions.sh
set -e
set -x

run_test ()
{
    nbdkit $1 --help
}

do_test ()
{
    vg=; [ "$NBDKIT_VALGRIND" = "1" ] && vg="-valgrind"
    case "$1$vg" in
        python-valgrind | tcl-valgrind)
            echo "$0: skipping $1$vg because this language doesn't support valgrind"
            ;;
        example4*)
            # These tests are written in Perl so we have to check that
            # the Perl plugin was compiled.
            if nbdkit perl --version; then run_test $1; fi
            ;;
	nbd*)
	    # Because of macOS SIP misfeature the DYLD_* environment
	    # variable added by libnbd/run is filtered out and the
	    # test won't work.  Skip it entirely on Macs.
	    if test "$(uname)" != "Darwin"; then run_test $1; fi
	    ;;
        S3*)
            # Requires Python plugin and boto3 library.
            if nbdkit python --version && $PYTHON -c 'import boto3'; then
                run_test $1
            fi
            ;;
        gcs*)
            # Requires Python plugin and google-cloud-storage library.
            if nbdkit python --version && \
               $PYTHON -c 'import google.cloud.storage'; then
                run_test $1
            fi
            ;;
        *)
            run_test $1
            ;;
    esac
}
foreach_plugin do_test
