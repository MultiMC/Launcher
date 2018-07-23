/*
 * Copyright (c) 2001, 2008, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */
#include <stdint.h>
typedef unsigned short ushort;
typedef unsigned int uint32_t;
typedef unsigned char uchar;

struct unpacker;

struct jar
{
    // JAR file writer
    FILE *jarfp;
    int default_modtime;

    // Used by unix2dostime:
    int modtime_cache;
    uint32_t dostime_cache;

    // Private members
    fillbytes central_directory;
    ushort central_directory_count;
    uint32_t output_file_offset;
    fillbytes deflated; // temporary buffer

    // pointer to outer unpacker, for error checks etc.
    unpacker *u;

    // Public Methods
    void openJarFile(const char *fname);
    void addJarEntry(const char *fname, bool deflate_hint, int modtime, bytes &head,
                     bytes &tail);
    void addDirectoryToJarFile(const char *dir_name);
    void closeJarFile(bool central);

    void init(unpacker *u_);

    void free()
    {
        central_directory.free();
        deflated.free();
    }

    void reset()
    {
        free();
        init(u);
    }

    // Private Methods
    void write_data(void *ptr, int len);
    void write_data(bytes &b)
    {
        write_data(b.ptr, (int)b.len);
    }
    void add_to_jar_directory(const char *fname, bool store, int modtime, int len, int clen,
                              uint32_t crc);
    void write_jar_header(const char *fname, bool store, int modtime, int len, int clen,
                          unsigned int crc);
    void write_central_directory();
    uint32_t dostime(int y, int n, int d, int h, int m, int s);
    uint32_t get_dostime(int modtime);

    // The definitions of these depend on the NO_ZLIB option:
    bool deflate_bytes(bytes &head, bytes &tail);
    static uint32_t get_crc32(uint32_t c, unsigned char *ptr, uint32_t len);
};

struct gunzip
{
    // optional gzip input stream control block

    // pointer to outer unpacker, for error checks etc.
    unpacker *u;

    void *read_input_fn; // underlying \bchar\b stream
    void *zstream;       // inflater state
    char inbuf[1 << 14]; // input buffer

    void init(unpacker *u_); // pushes new value on u->read_input_fn

    void free();

    void start(int magic);

    // private stuff
    void read_fixed_field(char *buf, size_t buflen);
};
