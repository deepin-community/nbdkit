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
requires jq --version
requires qemu-img --version
requires qemu-img map --help
error_if_sparse_page_not_32768

out="test-data-extents.out"
expected="test-data-extents.expected"
files="$out $expected"
rm -f $files
cleanup_fn rm -f $files

do_test ()
{
    # We use jq to normalize the output and convert it to plain text.
    nbdkit data "$1" size="$2" \
           --run 'qemu-img map -f raw --output=json "$uri"' |
        jq -c '.[] | {start:.start, length:.length, data:.data, zero:.zero}' \
           > $out
    if ! cmp $out $expected; then
        echo "$0: output did not match expected data"
        echo "expected:"
        cat $expected
        echo "output:"
        cat $out
        exit 1
    fi
}

# Completely sparse disk.
cat > $expected <<'EOF'
{"start":0,"length":1048576,"data":false,"zero":true}
EOF
do_test "" 1M

# Completely allocated disk.
cat > $expected <<'EOF'
{"start":0,"length":32768,"data":true,"zero":false}
EOF
do_test "1" 32K

# Allocated data at the start of a 1M disk.
cat > $expected <<'EOF'
{"start":0,"length":32768,"data":true,"zero":false}
{"start":32768,"length":1015808,"data":false,"zero":true}
EOF
do_test "1" 1M

# This should test coalescing in the extents code.
cat > $expected <<'EOF'
{"start":0,"length":32768,"data":false,"zero":true}
{"start":32768,"length":65536,"data":true,"zero":false}
{"start":98304,"length":32768,"data":false,"zero":true}
{"start":131072,"length":65536,"data":true,"zero":true}
{"start":196608,"length":851968,"data":false,"zero":true}
EOF
do_test "@32768 1 @65536 1 @131072 0 @163840 0" 1M

# Zero-length plugin.  Unlike nbdkit-zero-plugin, the data plugin
# advertises extents and so will behave differently.
cat > $expected <<'EOF'
{"start":0,"length":0,"data":false,"zero":false}
EOF
do_test "" 0
