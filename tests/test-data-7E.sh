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

# Test the data plugin creating a 7 EB partitioned disk, and
# the partition filter on top.

source ./functions.sh
set -e
set -x

requires_nbdsh_uri

sock=$(mktemp -u /tmp/nbdkit-test-sock.XXXXXX)
files="data-7E.pid $sock"
rm -f $files
cleanup_fn rm -f $files

# Run nbdkit.
start_nbdkit -P data-7E.pid -U $sock \
       --filter=partition \
       data size=7E partition=1 \
       '
   @0x1c0 2 0 0xee 0xfe 0xff 0xff 0x01 0  0 0 0xff 0xff 0xff 0xff
   @0x1fe 0x55 0xaa
   @0x200 0x45 0x46 0x49 0x20 0x50 0x41 0x52 0x54
                 0 0 1 0 0x5c 0 0 0
          0x9b 0xe5 0x6a 0xc5 0 0 0 0  1 0 0 0 0 0 0 0
          0xff 0xff 0xff 0xff 0xff 0xff 0x37 0  0x22 0 0 0 0 0 0 0
          0xde 0xff 0xff 0xff 0xff 0xff 0x37 0
                 0x72 0xb6 0x9e 0x0c 0x6b 0x76 0xb0 0x4f
          0xb3 0x94 0xb2 0xf1 0x61 0xec 0xdd 0x3c  2 0 0 0 0 0 0 0
          0x80 0 0 0 0x80 0 0 0  0x79 0x8a 0xd0 0x7e 0 0 0 0
   @0x400 0xaf 0x3d 0xc6 0x0f 0x83 0x84 0x72 0x47
                 0x8e 0x79 0x3d 0x69 0xd8 0x47 0x7d 0xe4
          0xd5 0x19 0x46 0x95 0xe3 0x82 0xa8 0x4c
                 0x95 0x82 0x7a 0xbe 0x1c 0xfc 0x62 0x90
          0x80 0 0 0 0 0 0 0  0x80 0xff 0xff 0xff 0xff 0xff 0x37 0
          0 0 0 0 0 0 0 0  0x70 0 0x31 0 0 0 0 0
   @0x6fffffffffffbe00
          0xaf 0x3d 0xc6 0x0f 0x83 0x84 0x72 0x47
                 0x8e 0x79 0x3d 0x69 0xd8 0x47 0x7d 0xe4
          0xd5 0x19 0x46 0x95 0xe3 0x82 0xa8 0x4c
                 0x95 0x82 0x7a 0xbe 0x1c 0xfc 0x62 0x90
          0x80 0 0 0 0 0 0 0  0x80 0xff 0xff 0xff 0xff 0xff 0x37 0
          0 0 0 0 0 0 0 0  0x70 0 0x31 0 0 0 0 0
   @0x6ffffffffffffe00
          0x45 0x46 0x49 0x20 0x50 0x41 0x52 0x54
                 0 0 1 0 0x5c 0 0 0
          0x6c 0x76 0xa1 0xa0 0 0 0 0
                 0xff 0xff 0xff 0xff 0xff 0xff 0x37 0
          1 0 0 0 0 0 0 0  0x22 0 0 0 0 0 0 0
          0xde 0xff 0xff 0xff 0xff 0xff 0x37 0
                 0x72 0xb6 0x9e 0x0c 0x6b 0x76 0xb0 0x4f
          0xb3 0x94 0xb2 0xf1 0x61 0xec 0xdd 0x3c
                 0xdf 0xff 0xff 0xff 0xff 0xff 0x37 0
          0x80 0 0 0 0x80 0 0 0  0x79 0x8a 0xd0 0x7e 0 0 0 0
   '

# Since we're reading the empty first partition, any read returns zeroes.
nbdsh --connect "nbd+unix://?socket=$sock" \
      -c '
buf = h.pread(16, 498)
assert buf == bytearray(16)
'
