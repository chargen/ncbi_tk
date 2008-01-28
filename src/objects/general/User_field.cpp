/* $Id$
 * ===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's official duties as a United States Government employee and
 *  thus cannot be copyrighted.  This software/database is freely available
 *  to the public for use. The National Library of Medicine and the U.S.
 *  Government have not placed any restriction on its use or reproduction.
 *
 *  Although all reasonable efforts have been taken to ensure the accuracy
 *  and reliability of the software and data, the NLM and the U.S.
 *  Government do not and cannot warrant the performance or results that
 *  may be obtained by using this software or data. The NLM and the U.S.
 *  Government disclaim all warranties, express or implied, including
 *  warranties of performance, merchantability or fitness for any particular
 *  purpose.
 *
 *  Please cite the author in any work or product based on this material.
 *
 * ===========================================================================
 *
 * Author:  .......
 *
 * File Description:
 *   .......
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using specifications from the data definition file
 *   'general.asn'.
 */

// standard includes

// generated includes
#include <ncbi_pch.hpp>
#include <objects/general/User_field.hpp>
#include <objects/general/User_object.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CUser_field::~CUser_field(void)
{
}



/// add fields to the current user field
CUser_field& CUser_field::AddField(const string& label, int value)
{
    CRef<CUser_field> field(new CUser_field());
    field->SetLabel().SetStr(label);
    field->SetData().SetInt(value);
    SetData().SetFields().push_back(field);
    return *this;
}


/// add fields to the current user field
CUser_field& CUser_field::AddField(const string& label, const string& value)
{
    CRef<CUser_field> field(new CUser_field());
    field->SetLabel().SetStr(label);
    field->SetData().SetStr(value);
    SetData().SetFields().push_back(field);
    return *this;
}


CUser_field& CUser_field::AddField(const string& label, double value)
{
    CRef<CUser_field> field(new CUser_field());
    field->SetLabel().SetStr(label);
    field->SetData().SetReal(value);
    SetData().SetFields().push_back(field);
    return *this;
}


CUser_field& CUser_field::AddField(const string& label, bool value)
{
    CRef<CUser_field> field(new CUser_field());
    field->SetLabel().SetStr(label);
    field->SetData().SetBool(value);
    SetData().SetFields().push_back(field);
    return *this;
}


CUser_field& CUser_field::AddField(const string& label,
                                   const vector<string>& value)
{
    CRef<CUser_field> field(new CUser_field());
    field->SetLabel().SetStr(label);
    field->SetNum(value.size());
    field->SetData().SetStrs() = value;
    SetData().SetFields().push_back(field);
    return *this;
}


CUser_field& CUser_field::AddField(const string& label,
                                   const vector<int>& value)
{
    CRef<CUser_field> field(new CUser_field());
    field->SetLabel().SetStr(label);
    field->SetNum(value.size());
    field->SetData().SetInts() = value;
    SetData().SetFields().push_back(field);
    return *this;
}


CUser_field& CUser_field::AddField(const string& label,
                                   const vector<double>& value)
{
    CRef<CUser_field> field(new CUser_field());
    field->SetLabel().SetStr(label);
    field->SetNum(value.size());
    field->SetData().SetReals() = value;
    SetData().SetFields().push_back(field);
    return *this;
}


CUser_field& CUser_field::AddField(const string& label,
                                   CUser_object& value)
{
    CRef<CUser_field> field(new CUser_field());
    field->SetLabel().SetStr(label);
    field->SetData().SetObject(value);
    SetData().SetFields().push_back(field);
    return *this;
}


CUser_field& CUser_field::AddField(const string& label,
                                   const vector< CRef<CUser_object> >& value)
{
    CRef<CUser_field> field(new CUser_field());
    field->SetLabel().SetStr(label);
    field->SetNum(value.size());
    field->SetData().SetObjects() = value;
    SetData().SetFields().push_back(field);
    return *this;
}


CUser_field& CUser_field::AddField(const string& label,
                                   const vector< CRef<CUser_field> >& value)
{
    CRef<CUser_field> field(new CUser_field());
    field->SetLabel().SetStr(label);
    field->SetNum(value.size());
    field->SetData().SetFields() = value;
    SetData().SetFields().push_back(field);
    return *this;
}


/// Access a named field in this user field.  This will tokenize the
/// string 'str' on the delimiters; if the field doesn't exist, an
/// exception will be thrown.
const CUser_field& CUser_field::GetField(const string& str,
                                         const string& delim) const
{
    CConstRef<CUser_field> f = GetFieldRef(str, delim);
    if ( !f ) {
        NCBI_THROW(CException, eUnknown,
                   "failed to find field named " + str);
    }
    return *f;
}


/// Return a field reference representing the tokenized key, or a
/// NULL reference if the key doesn't exist.
CConstRef<CUser_field> CUser_field::GetFieldRef(const string& str,
                                                const string& delim) const
{
    list<string> toks;
    NStr::Split(str, delim, toks);

    CConstRef<CUser_field> f(this);
    if ( !f->GetData().IsFields() ) {
        if (toks.size() == 1  &&
            f->GetLabel().IsStr()  &&  f->GetLabel().GetStr() == toks.front()) {
            return f;
        } else {
            return CConstRef<CUser_field>();
        }
    }

    list<string>::const_iterator last = toks.end();
    --last;

    ITERATE (list<string>, iter, toks) {
        CConstRef<CUser_field> new_f;

        ITERATE (TData::TFields, field_iter, f->GetData().GetFields()) {
            const CUser_field& field = **field_iter;
            if (field.GetLabel().IsStr()
                &&  field.GetLabel().GetStr() == *iter) {
                if (iter != last  &&  field.GetData().IsFields()) {
                    new_f = *field_iter;
                    break;
                } else if (iter == last) {
                    new_f = *field_iter;
                    break;
                }
            }
        }

        f = new_f;
        if ( !f ) {
            return f;
        }
    }

    return f;
}


/// Access a named field in this user field.  This will tokenize the
/// string 'str' on the delimiters and recursively add fields where needed
CUser_field& CUser_field::SetField(const string& str,
                                   const string& delim)
{
    CRef<CUser_field> f = SetFieldRef(str, delim);
    return *f;
}


/// Return a field reference representing the tokenized key, or a
/// NULL reference if the key cannot be created for some reason.
CRef<CUser_field> CUser_field::SetFieldRef(const string& str,
                                           const string& delim)
{
    list<string> toks;
    NStr::Split(str, delim, toks);

    CRef<CUser_field> f(this);
    if ( ! f->GetData().IsFields()  &&  f->GetData().Which() != CUser_field::TData::e_not_set ) {
        // There is a value here, not a list of User_fields, no place to recurse downward. 
        _ASSERT(false);
        NCBI_THROW(CException, eUnknown, "Too many parts in key: \"" + str + "\"");
    }
    list<string>::const_iterator last = toks.end();
    --last;
    ITERATE (list<string>, iter, toks) {
        CRef<CUser_field> new_f;
        NON_CONST_ITERATE (TData::TFields, field_iter, f->SetData().SetFields()) {
            const CUser_field& field = **field_iter;
            if (field.GetLabel().GetStr() == *iter) {
                if (iter == last) {
                    new_f = *field_iter;
                    break;
                } else if (field.GetData().IsFields()) {
                    new_f = *field_iter;
                    break;
                } else {
                    // There is a value here, not a list of User_fields, no place to recurse downward. 
                    _ASSERT(false);
                    NCBI_THROW(CException, eUnknown, "Too many parts in key: \"" + str + "\"");
                }
            }
        }

        if ( !new_f ) {
            new_f.Reset(new CUser_field());
            new_f->SetLabel().SetStr(*iter);
            f->SetData().SetFields().push_back(new_f);
        }

        f = new_f;
    }

    return f;
}


/// Verify that a named field exists
bool CUser_field::HasField(const string& str,
                           const string& delim) const
{
    CConstRef<CUser_field> f = GetFieldRef(str, delim);
    return f.GetPointer() != NULL;
}



/// delete a named field.
bool CUser_field::DeleteField(const string& str,
                              const string& delim)
{
    list<string> toks;
    NStr::Split(str, delim, toks);

    CRef<CUser_field> f(this);
    list<string>::const_iterator last = toks.end();
    --last;

    ITERATE (list<string>, iter, toks) {
        CRef<CUser_field> new_f;
        if ( !f->GetData().IsFields() ) {
            return false;
        }
        NON_CONST_ITERATE (TData::TFields, field_iter, f->SetData().SetFields()) {
            const CUser_field& field = **field_iter;
            if (field.GetLabel().IsStr()
                &&  field.GetLabel().GetStr() == *iter) {
                if (iter != last  &&  field.GetData().IsFields()) {
                    new_f = *field_iter;
                    break;
                } else if (iter == last) {
                    // delete this one from f, its parent.
                    f->SetData().SetFields().erase(field_iter);
                    return true;
                }
            }
        }
        if ( !new_f ) {
            return false;
        }
        f = new_f;
    }
    // Never reached.
    return false;
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 64, chars: 1886, CRC32: 5dc82940 */
