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

requires_run
requires hexdump --version
requires $PYTHON --version
requires_nbdcopy
requires_plugin python
skip_if_valgrind "because Python code leaks memory"

# Skip this test if the real google-cloud-storage module is installed,
# since that module abuses pth files to insert itself into the module
# system above any path or precedence we can override using
# PYTHONPATH.
requires_not $PYTHON -c 'import google.cloud.storage'

# There is a fake google-cloud-storage module in test-gcs/ which
# we use as a test harness for the plugin.
requires test -d "$srcdir/test-gcs"
prepend PYTHONPATH "$srcdir/test-gcs"
export PYTHONPATH

file=gcs.out
rm -f $file
cleanup_fn rm -f $file

# The fake module checks the parameters have these particular values.
nbdkit -v gcs \
       json-credentials=TEST_JSON_CREDENTIALS \
       bucket=MY_FILES \
       key=MY_KEY \
       --run "nbdcopy \"\$uri\" $file"

ls -l $file
hexdump -C $file

if [ "$(hexdump -C $file)" != "00000000  78 78 78 78 78 78 78 78  78 78 78 78 78 78 78 78  |xxxxxxxxxxxxxxxx|
*
00001000  79 79 79 79 79 79 79 79  79 79 79 79 79 79 79 79  |yyyyyyyyyyyyyyyy|
*
00001800  7a 7a 7a 7a 7a 7a 7a 7a  7a 7a 7a 7a 7a 7a 7a 7a  |zzzzzzzzzzzzzzzz|
*
00002000" ]; then
    echo "$0: unexpected output from test"
    exit 1
fi
