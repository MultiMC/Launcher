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

#pragma once

struct bytes
{
    int8_t *ptr;
    size_t len;
    int8_t *limit()
    {
        return ptr + len;
    }

    void set(int8_t *ptr_, size_t len_)
    {
        ptr = ptr_;
        len = len_;
    }
    void set(const char *str)
    {
        ptr = (int8_t *)str;
        len = strlen(str);
    }
    bool inBounds(const void *p); // p in [ptr, limit)
    void malloc(size_t len_);
    void realloc(size_t len_);
    void free();
    void copyFrom(const void *ptr_, size_t len_, size_t offset = 0);
    void saveFrom(const void *ptr_, size_t len_);
    void saveFrom(const char *str)
    {
        saveFrom(str, strlen(str));
    }
    void copyFrom(bytes &other, size_t offset = 0)
    {
        copyFrom(other.ptr, other.len, offset);
    }
    void saveFrom(bytes &other)
    {
        saveFrom(other.ptr, other.len);
    }
    void clear(int fill_byte = 0)
    {
        memset(ptr, fill_byte, len);
    }
    int8_t *writeTo(int8_t *bp);
    bool equals(bytes &other)
    {
        return 0 == compareTo(other);
    }
    int compareTo(bytes &other);
    bool contains(int8_t c)
    {
        return indexOf(c) >= 0;
    }
    int indexOf(int8_t c);
    // substrings:
    static bytes of(int8_t *ptr, size_t len)
    {
        bytes res;
        res.set(ptr, len);
        return res;
    }
    bytes slice(size_t beg, size_t end)
    {
        bytes res;
        res.ptr = ptr + beg;
        res.len = end - beg;
        assert(res.len == 0 ||(inBounds(res.ptr) && inBounds(res.limit() - 1)));
        return res;
    }
    // building C strings inside byte buffers:
    bytes &strcat(const char *str)
    {
        ::strcat((char *)ptr, str);
        return *this;
    }
    bytes &strcat(bytes &other)
    {
        ::strncat((char *)ptr, (char *)other.ptr, other.len);
        return *this;
    }
    char *strval()
    {
        assert(strlen((char *)ptr) == len);
        return (char *)ptr;
    }
};
#define BYTES_OF(var) (bytes::of((int8_t *)&(var), sizeof(var)))

struct fillbytes
{
    bytes b;
    size_t allocated;

    int8_t *base()
    {
        return b.ptr;
    }
    size_t size()
    {
        return b.len;
    }
    int8_t *limit()
    {
        return b.limit();
    } // logical limit
    void setLimit(int8_t *lp)
    {
        assert(isAllocated(lp));
        b.len = lp - b.ptr;
    }
    int8_t *end()
    {
        return b.ptr + allocated;
    } // physical limit
    int8_t *loc(size_t o)
    {
        assert(o < b.len);
        return b.ptr + o;
    }
    void init()
    {
        allocated = 0;
        b.set(nullptr, 0);
    }
    void init(size_t s)
    {
        init();
        ensureSize(s);
    }
    void free()
    {
        if (allocated != 0)
            b.free();
        allocated = 0;
    }
    void empty()
    {
        b.len = 0;
    }
    int8_t *grow(size_t s); // grow so that limit() += s
    int getByte(uint32_t i)
    {
        return *loc(i) & 0xFF;
    }
    void addByte(int8_t x)
    {
        *grow(1) = x;
    }
    void ensureSize(size_t s); // make sure allocated >= s
    void trimToSize()
    {
        if (allocated > size())
            b.realloc(allocated = size());
    }
    bool canAppend(size_t s)
    {
        return allocated > b.len + s;
    }
    bool isAllocated(int8_t *p)
    {
        return p >= base() && p <= end();
    } // asserts
    void set(bytes &src)
    {
        set(src.ptr, src.len);
    }

    void set(int8_t *ptr, size_t len)
    {
        b.set(ptr, len);
        allocated = 0; // mark as not reallocatable
    }

    // block operations on resizing byte buffer:
    fillbytes &append(const void *ptr_, size_t len_)
    {
        memcpy(grow(len_), ptr_, len_);
        return (*this);
    }
    fillbytes &append(bytes &other)
    {
        return append(other.ptr, other.len);
    }
    fillbytes &append(const char *str)
    {
        return append(str, strlen(str));
    }
};

struct ptrlist : fillbytes
{
    typedef const void *cvptr;
    int length()
    {
        return (int)(size() / sizeof(cvptr));
    }
    cvptr *base()
    {
        return (cvptr *)fillbytes::base();
    }
    cvptr &get(int i)
    {
        return *(cvptr *)loc(i * sizeof(cvptr));
    }
    cvptr *limit()
    {
        return (cvptr *)fillbytes::limit();
    }
    void add(cvptr x)
    {
        *(cvptr *)grow(sizeof(x)) = x;
    }
    void popTo(int l)
    {
        assert(l <= length());
        b.len = l * sizeof(cvptr);
    }
    int indexOf(cvptr x);
    bool contains(cvptr x)
    {
        return indexOf(x) >= 0;
    }
    void freeAll(); // frees every ptr on the list, plus the list itself
};
// Use a macro rather than mess with subtle mismatches
// between member and non-member function pointers.
#define PTRLIST_QSORT(ptrls, fn) ::qsort((ptrls).base(), (ptrls).length(), sizeof(void *), fn)

struct intlist : fillbytes
{
    int length()
    {
        return (int)(size() / sizeof(int));
    }
    int *base()
    {
        return (int *)fillbytes::base();
    }
    int &get(int i)
    {
        return *(int *)loc(i * sizeof(int));
    }
    int *limit()
    {
        return (int *)fillbytes::limit();
    }
    void add(int x)
    {
        *(int *)grow(sizeof(x)) = x;
    }
    void popTo(int l)
    {
        assert(l <= length());
        b.len = l * sizeof(int);
    }
    int indexOf(int x);
    bool contains(int x)
    {
        return indexOf(x) >= 0;
    }
};
