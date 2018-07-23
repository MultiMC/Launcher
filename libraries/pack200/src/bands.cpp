/*
 * Copyright (c) 2002, 2009, Oracle and/or its affiliates. All rights reserved.
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

// -*- C++ -*-
// Small program for unpacking specially compressed Java packages.
// John R. Rose

#include <sys/types.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <stdint.h>

#include "defines.h"
#include "bytes.h"
#include "utils.h"
#include "coding.h"
#include "bands.h"

#include "constants.h"
#include "unpack.h"

void band::readData(int expectedLength)
{
    assert(expectedLength >= 0);
    assert(vs[0].cmk == cmk_ERROR);
    if (expectedLength != 0)
    {
        assert(length == 0);
        length = expectedLength;
    }
    if (length == 0)
    {
        assert((rplimit = cm.vs0.rp = u->rp) != nullptr);
        return;
    }
    assert(length > 0);

    bool is_BYTE1 = (defc->spec == BYTE1_spec);

    if (is_BYTE1)
    {
        // No possibility of coding change.  Sizing is exact.
        u->ensure_input(length);
    }
    else
    {
        // Make a conservatively generous estimate of band size in bytes.
        // Assume B == 5 everywhere.
        // Assume awkward pop with all {U} values (2*5 per value)
        int64_t generous = (int64_t)length * (B_MAX * 3 + 1) + C_SLOP;
        u->ensure_input(generous);
    }

    // Read one value to see what it might be.
    int XB = _meta_default;
    if (!is_BYTE1)
    {
        // must be a variable-length coding
        assert(defc->B() > 1 && defc->L() > 0);

        value_stream xvs;
        coding *valc = defc;
        if (valc->D() != 0)
        {
            valc = coding::findBySpec(defc->B(), defc->H(), defc->S());
            assert(!valc->isMalloc);
        }
        xvs.init(u->rp, u->rplimit, valc);
        int X = xvs.getInt();
        if (valc->S() != 0)
        {
            assert(valc->min <= -256);
            XB = -1 - X;
        }
        else
        {
            int L = valc->L();
            assert(valc->max >= L + 255);
            XB = X - L;
        }
        if (0 <= XB && XB < 256)
        {
            // Skip over the escape value.
            u->rp = xvs.rp;
        }
        else
        {
            // No, it's still default.
            XB = _meta_default;
        }
    }

    if (XB <= _meta_canon_max)
    {
        byte XB_byte = (byte)XB;
        byte *XB_ptr = &XB_byte;
        cm.init(u->rp, u->rplimit, XB_ptr, 0, defc, length, nullptr);
    }
    else
    {
        assert(u->meta_rp != nullptr);
        // Scribble the initial byte onto the band.
        byte *save_meta_rp = --u->meta_rp;
        byte save_meta_xb = (*save_meta_rp);
        (*save_meta_rp) = (byte)XB;
        cm.init(u->rp, u->rplimit, u->meta_rp, 0, defc, length, nullptr);
        (*save_meta_rp) = save_meta_xb; // put it back, just to be tidy
    }
    rplimit = u->rp;

    rewind();
}

void band::setIndex(cpindex *ix_)
{
    assert(ix_ == nullptr || ixTag == ix_->ixTag);
    ix = ix_;
}
void band::setIndexByTag(byte tag)
{
    setIndex(u->cp.getIndex(tag));
}

entry *band::getRefCommon(cpindex *ix_, bool nullOKwithCaller)
{
    assert(ix_->ixTag == ixTag ||
           (ixTag == CONSTANT_Literal && ix_->ixTag >= CONSTANT_Integer &&
            ix_->ixTag <= CONSTANT_String));
    int n = vs[0].getInt() - nullOK;
    // Note: band-local nullOK means nullptr encodes as 0.
    // But nullOKwithCaller means caller is willing to tolerate a nullptr.
    entry *ref = ix_->get(n);
    if (ref == nullptr && !(nullOKwithCaller && n == -1))
        unpack_abort(n == -1 ? "nullptr ref" : "bad ref");
    return ref;
}

int64_t band::getLong(band &lo_band, bool have_hi)
{
    band &hi_band = (*this);
    assert(lo_band.bn == hi_band.bn + 1);
    uint32_t lo = lo_band.getInt();
    if (!have_hi)
    {
        assert(hi_band.length == 0);
        return makeLong(0, lo);
    }
    uint32_t hi = hi_band.getInt();
    return makeLong(hi, lo);
}

int band::getIntTotal()
{
    if (length == 0)
        return 0;
    if (total_memo > 0)
        return total_memo - 1;
    int total = getInt();
    // overflow checks require that none of the addends are <0,
    // and that the partial sums never overflow (wrap negative)
    if (total < 0)
    {
        unpack_abort("overflow detected");
    }
    for (int k = length - 1; k > 0; k--)
    {
        int prev_total = total;
        total += vs[0].getInt();
        if (total < prev_total)
        {
            unpack_abort("overflow detected");
        }
    }
    rewind();
    total_memo = total + 1;
    return total;
}

int band::getIntCount(int tag)
{
    if (length == 0)
        return 0;
    if (tag >= HIST0_MIN && tag <= HIST0_MAX)
    {
        if (hist0 == nullptr)
        {
            // Lazily calculate an approximate histogram.
            hist0 = U_NEW(int, (HIST0_MAX - HIST0_MIN) + 1);
            for (int k = length; k > 0; k--)
            {
                int x = vs[0].getInt();
                if (x >= HIST0_MIN && x <= HIST0_MAX)
                    hist0[x - HIST0_MIN] += 1;
            }
            rewind();
        }
        return hist0[tag - HIST0_MIN];
    }
    int total = 0;
    for (int k = length; k > 0; k--)
    {
        total += (vs[0].getInt() == tag) ? 1 : 0;
    }
    rewind();
    return total;
}

#define INDEX_INIT(tag, nullOK, subindex) ((tag) + (subindex) * SUBINDEX_BIT + (nullOK) * 256)

#define INDEX(tag) INDEX_INIT(tag, 0, 0)
#define NULL_OR_INDEX(tag) INDEX_INIT(tag, 1, 0)
#define SUB_INDEX(tag) INDEX_INIT(tag, 0, 1)
#define NO_INDEX 0

struct band_init
{
    int defc;
    int index;
};

#define BAND_INIT(name, cspec, ix)                                                             \
    {                                                                                          \
        cspec, ix                                                                              \
    }

const band_init all_band_inits[] =
    {
        // BAND_INIT(archive_magic, BYTE1_spec, 0),
        // BAND_INIT(archive_header, UNSIGNED5_spec, 0),
        // BAND_INIT(band_headers, BYTE1_spec, 0),
        BAND_INIT(cp_Utf8_prefix, DELTA5_spec, 0), BAND_INIT(cp_Utf8_suffix, UNSIGNED5_spec, 0),
        BAND_INIT(cp_Utf8_chars, CHAR3_spec, 0), BAND_INIT(cp_Utf8_big_suffix, DELTA5_spec, 0),
        BAND_INIT(cp_Utf8_big_chars, DELTA5_spec, 0), BAND_INIT(cp_Int, UDELTA5_spec, 0),
        BAND_INIT(cp_Float, UDELTA5_spec, 0), BAND_INIT(cp_Long_hi, UDELTA5_spec, 0),
        BAND_INIT(cp_Long_lo, DELTA5_spec, 0), BAND_INIT(cp_Double_hi, UDELTA5_spec, 0),
        BAND_INIT(cp_Double_lo, DELTA5_spec, 0),
        BAND_INIT(cp_String, UDELTA5_spec, INDEX(CONSTANT_Utf8)),
        BAND_INIT(cp_Class, UDELTA5_spec, INDEX(CONSTANT_Utf8)),
        BAND_INIT(cp_Signature_form, DELTA5_spec, INDEX(CONSTANT_Utf8)),
        BAND_INIT(cp_Signature_classes, UDELTA5_spec, INDEX(CONSTANT_Class)),
        BAND_INIT(cp_Descr_name, DELTA5_spec, INDEX(CONSTANT_Utf8)),
        BAND_INIT(cp_Descr_type, UDELTA5_spec, INDEX(CONSTANT_Signature)),
        BAND_INIT(cp_Field_class, DELTA5_spec, INDEX(CONSTANT_Class)),
        BAND_INIT(cp_Field_desc, UDELTA5_spec, INDEX(CONSTANT_NameandType)),
        BAND_INIT(cp_Method_class, DELTA5_spec, INDEX(CONSTANT_Class)),
        BAND_INIT(cp_Method_desc, UDELTA5_spec, INDEX(CONSTANT_NameandType)),
        BAND_INIT(cp_Imethod_class, DELTA5_spec, INDEX(CONSTANT_Class)),
        BAND_INIT(cp_Imethod_desc, UDELTA5_spec, INDEX(CONSTANT_NameandType)),
        BAND_INIT(attr_definition_headers, BYTE1_spec, 0),
        BAND_INIT(attr_definition_name, UNSIGNED5_spec, INDEX(CONSTANT_Utf8)),
        BAND_INIT(attr_definition_layout, UNSIGNED5_spec, INDEX(CONSTANT_Utf8)),
        BAND_INIT(ic_this_class, UDELTA5_spec, INDEX(CONSTANT_Class)),
        BAND_INIT(ic_flags, UNSIGNED5_spec, 0),
        BAND_INIT(ic_outer_class, DELTA5_spec, NULL_OR_INDEX(CONSTANT_Class)),
        BAND_INIT(ic_name, DELTA5_spec, NULL_OR_INDEX(CONSTANT_Utf8)),
        BAND_INIT(class_this, DELTA5_spec, INDEX(CONSTANT_Class)),
        BAND_INIT(class_super, DELTA5_spec, INDEX(CONSTANT_Class)),
        BAND_INIT(class_interface_count, DELTA5_spec, 0),
        BAND_INIT(class_interface, DELTA5_spec, INDEX(CONSTANT_Class)),
        BAND_INIT(class_field_count, DELTA5_spec, 0),
        BAND_INIT(class_method_count, DELTA5_spec, 0),
        BAND_INIT(field_descr, DELTA5_spec, INDEX(CONSTANT_NameandType)),
        BAND_INIT(field_flags_hi, UNSIGNED5_spec, 0),
        BAND_INIT(field_flags_lo, UNSIGNED5_spec, 0),
        BAND_INIT(field_attr_count, UNSIGNED5_spec, 0),
        BAND_INIT(field_attr_indexes, UNSIGNED5_spec, 0),
        BAND_INIT(field_attr_calls, UNSIGNED5_spec, 0),
        BAND_INIT(field_ConstantValue_KQ, UNSIGNED5_spec, INDEX(CONSTANT_Literal)),
        BAND_INIT(field_Signature_RS, UNSIGNED5_spec, INDEX(CONSTANT_Signature)),
        BAND_INIT(field_metadata_bands, -1, -1), BAND_INIT(field_attr_bands, -1, -1),
        BAND_INIT(method_descr, MDELTA5_spec, INDEX(CONSTANT_NameandType)),
        BAND_INIT(method_flags_hi, UNSIGNED5_spec, 0),
        BAND_INIT(method_flags_lo, UNSIGNED5_spec, 0),
        BAND_INIT(method_attr_count, UNSIGNED5_spec, 0),
        BAND_INIT(method_attr_indexes, UNSIGNED5_spec, 0),
        BAND_INIT(method_attr_calls, UNSIGNED5_spec, 0),
        BAND_INIT(method_Exceptions_N, UNSIGNED5_spec, 0),
        BAND_INIT(method_Exceptions_RC, UNSIGNED5_spec, INDEX(CONSTANT_Class)),
        BAND_INIT(method_Signature_RS, UNSIGNED5_spec, INDEX(CONSTANT_Signature)),
        BAND_INIT(method_metadata_bands, -1, -1), BAND_INIT(method_attr_bands, -1, -1),
        BAND_INIT(class_flags_hi, UNSIGNED5_spec, 0),
        BAND_INIT(class_flags_lo, UNSIGNED5_spec, 0),
        BAND_INIT(class_attr_count, UNSIGNED5_spec, 0),
        BAND_INIT(class_attr_indexes, UNSIGNED5_spec, 0),
        BAND_INIT(class_attr_calls, UNSIGNED5_spec, 0),
        BAND_INIT(class_SourceFile_RUN, UNSIGNED5_spec, NULL_OR_INDEX(CONSTANT_Utf8)),
        BAND_INIT(class_EnclosingMethod_RC, UNSIGNED5_spec, INDEX(CONSTANT_Class)),
        BAND_INIT(class_EnclosingMethod_RDN, UNSIGNED5_spec,
                  NULL_OR_INDEX(CONSTANT_NameandType)),
        BAND_INIT(class_Signature_RS, UNSIGNED5_spec, INDEX(CONSTANT_Signature)),
        BAND_INIT(class_metadata_bands, -1, -1),
        BAND_INIT(class_InnerClasses_N, UNSIGNED5_spec, 0),
        BAND_INIT(class_InnerClasses_RC, UNSIGNED5_spec, INDEX(CONSTANT_Class)),
        BAND_INIT(class_InnerClasses_F, UNSIGNED5_spec, 0),
        BAND_INIT(class_InnerClasses_outer_RCN, UNSIGNED5_spec, NULL_OR_INDEX(CONSTANT_Class)),
        BAND_INIT(class_InnerClasses_name_RUN, UNSIGNED5_spec, NULL_OR_INDEX(CONSTANT_Utf8)),
        BAND_INIT(class_ClassFile_version_minor_H, UNSIGNED5_spec, 0),
        BAND_INIT(class_ClassFile_version_major_H, UNSIGNED5_spec, 0),
        BAND_INIT(class_attr_bands, -1, -1), BAND_INIT(code_headers, BYTE1_spec, 0),
        BAND_INIT(code_max_stack, UNSIGNED5_spec, 0),
        BAND_INIT(code_max_na_locals, UNSIGNED5_spec, 0),
        BAND_INIT(code_handler_count, UNSIGNED5_spec, 0),
        BAND_INIT(code_handler_start_P, BCI5_spec, 0),
        BAND_INIT(code_handler_end_PO, BRANCH5_spec, 0),
        BAND_INIT(code_handler_catch_PO, BRANCH5_spec, 0),
        BAND_INIT(code_handler_class_RCN, UNSIGNED5_spec, NULL_OR_INDEX(CONSTANT_Class)),
        BAND_INIT(code_flags_hi, UNSIGNED5_spec, 0),
        BAND_INIT(code_flags_lo, UNSIGNED5_spec, 0),
        BAND_INIT(code_attr_count, UNSIGNED5_spec, 0),
        BAND_INIT(code_attr_indexes, UNSIGNED5_spec, 0),
        BAND_INIT(code_attr_calls, UNSIGNED5_spec, 0),
        BAND_INIT(code_StackMapTable_N, UNSIGNED5_spec, 0),
        BAND_INIT(code_StackMapTable_frame_T, BYTE1_spec, 0),
        BAND_INIT(code_StackMapTable_local_N, UNSIGNED5_spec, 0),
        BAND_INIT(code_StackMapTable_stack_N, UNSIGNED5_spec, 0),
        BAND_INIT(code_StackMapTable_offset, UNSIGNED5_spec, 0),
        BAND_INIT(code_StackMapTable_T, BYTE1_spec, 0),
        BAND_INIT(code_StackMapTable_RC, UNSIGNED5_spec, INDEX(CONSTANT_Class)),
        BAND_INIT(code_StackMapTable_P, BCI5_spec, 0),
        BAND_INIT(code_LineNumberTable_N, UNSIGNED5_spec, 0),
        BAND_INIT(code_LineNumberTable_bci_P, BCI5_spec, 0),
        BAND_INIT(code_LineNumberTable_line, UNSIGNED5_spec, 0),
        BAND_INIT(code_LocalVariableTable_N, UNSIGNED5_spec, 0),
        BAND_INIT(code_LocalVariableTable_bci_P, BCI5_spec, 0),
        BAND_INIT(code_LocalVariableTable_span_O, BRANCH5_spec, 0),
        BAND_INIT(code_LocalVariableTable_name_RU, UNSIGNED5_spec, INDEX(CONSTANT_Utf8)),
        BAND_INIT(code_LocalVariableTable_type_RS, UNSIGNED5_spec, INDEX(CONSTANT_Signature)),
        BAND_INIT(code_LocalVariableTable_slot, UNSIGNED5_spec, 0),
        BAND_INIT(code_LocalVariableTypeTable_N, UNSIGNED5_spec, 0),
        BAND_INIT(code_LocalVariableTypeTable_bci_P, BCI5_spec, 0),
        BAND_INIT(code_LocalVariableTypeTable_span_O, BRANCH5_spec, 0),
        BAND_INIT(code_LocalVariableTypeTable_name_RU, UNSIGNED5_spec, INDEX(CONSTANT_Utf8)),
        BAND_INIT(code_LocalVariableTypeTable_type_RS, UNSIGNED5_spec,
                  INDEX(CONSTANT_Signature)),
        BAND_INIT(code_LocalVariableTypeTable_slot, UNSIGNED5_spec, 0),
        BAND_INIT(code_attr_bands, -1, -1), BAND_INIT(bc_codes, BYTE1_spec, 0),
        BAND_INIT(bc_case_count, UNSIGNED5_spec, 0), BAND_INIT(bc_case_value, DELTA5_spec, 0),
        BAND_INIT(bc_byte, BYTE1_spec, 0), BAND_INIT(bc_short, DELTA5_spec, 0),
        BAND_INIT(bc_local, UNSIGNED5_spec, 0), BAND_INIT(bc_label, BRANCH5_spec, 0),
        BAND_INIT(bc_intref, DELTA5_spec, INDEX(CONSTANT_Integer)),
        BAND_INIT(bc_floatref, DELTA5_spec, INDEX(CONSTANT_Float)),
        BAND_INIT(bc_longref, DELTA5_spec, INDEX(CONSTANT_Long)),
        BAND_INIT(bc_doubleref, DELTA5_spec, INDEX(CONSTANT_Double)),
        BAND_INIT(bc_stringref, DELTA5_spec, INDEX(CONSTANT_String)),
        BAND_INIT(bc_classref, UNSIGNED5_spec, NULL_OR_INDEX(CONSTANT_Class)),
        BAND_INIT(bc_fieldref, DELTA5_spec, INDEX(CONSTANT_Fieldref)),
        BAND_INIT(bc_methodref, UNSIGNED5_spec, INDEX(CONSTANT_Methodref)),
        BAND_INIT(bc_imethodref, DELTA5_spec, INDEX(CONSTANT_InterfaceMethodref)),
        BAND_INIT(bc_thisfield, UNSIGNED5_spec, SUB_INDEX(CONSTANT_Fieldref)),
        BAND_INIT(bc_superfield, UNSIGNED5_spec, SUB_INDEX(CONSTANT_Fieldref)),
        BAND_INIT(bc_thismethod, UNSIGNED5_spec, SUB_INDEX(CONSTANT_Methodref)),
        BAND_INIT(bc_supermethod, UNSIGNED5_spec, SUB_INDEX(CONSTANT_Methodref)),
        BAND_INIT(bc_initref, UNSIGNED5_spec, SUB_INDEX(CONSTANT_Methodref)),
        BAND_INIT(bc_escref, UNSIGNED5_spec, INDEX(CONSTANT_All)),
        BAND_INIT(bc_escrefsize, UNSIGNED5_spec, 0), BAND_INIT(bc_escsize, UNSIGNED5_spec, 0),
        BAND_INIT(bc_escbyte, BYTE1_spec, 0),
        BAND_INIT(file_name, UNSIGNED5_spec, INDEX(CONSTANT_Utf8)),
        BAND_INIT(file_size_hi, UNSIGNED5_spec, 0), BAND_INIT(file_size_lo, UNSIGNED5_spec, 0),
        BAND_INIT(file_modtime, DELTA5_spec, 0), BAND_INIT(file_options, UNSIGNED5_spec, 0),
        // BAND_INIT(file_bits, BYTE1_spec, 0),
        {0, 0}};

band *band::makeBands(unpacker *u)
{
    band *tmp_all_bands = U_NEW(band, BAND_LIMIT);
    for (int i = 0; i < BAND_LIMIT; i++)
    {
        assert((byte *)&all_band_inits[i + 1] <
               (byte *)all_band_inits + sizeof(all_band_inits));
        const band_init &bi = all_band_inits[i];
        band &b = tmp_all_bands[i];
        coding *defc = coding::findBySpec(bi.defc);
        assert((defc == nullptr) == (bi.defc == -1)); // no garbage, please
        assert(defc == nullptr || !defc->isMalloc);
        b.init(u, i, defc);
        if (bi.index > 0)
        {
            b.nullOK = ((bi.index >> 8) & 1);
            b.ixTag = (bi.index & 0xFF);
        }
    }
    return tmp_all_bands;
}

void band::initIndexes(unpacker *u)
{
    band *tmp_all_bands = u->all_bands;
    for (int i = 0; i < BAND_LIMIT; i++)
    {
        band *scan = &tmp_all_bands[i];
        uint32_t tag = scan->ixTag; // Cf. #define INDEX(tag) above
        if (tag != 0 && tag != CONSTANT_Literal && (tag & SUBINDEX_BIT) == 0)
        {
            scan->setIndex(u->cp.getIndex(tag));
        }
    }
}
