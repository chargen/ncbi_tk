#ifndef OBJOSTRXML__HPP
#define OBJOSTRXML__HPP

/*  $Id$
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
* Author: Eugene Vasilchenko
*
* File Description:
*   XML objects output stream
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.6  2000/08/15 19:44:41  vasilche
* Added Read/Write hooks:
* CReadObjectHook/CWriteObjectHook for objects of specified type.
* CReadClassMemberHook/CWriteClassMemberHook for specified members.
* CReadChoiceVariantHook/CWriteChoiceVariant for specified choice variants.
* CReadContainerElementHook/CWriteContainerElementsHook for containers.
*
* Revision 1.5  2000/07/03 18:42:36  vasilche
* Added interface to typeinfo via CObjectInfo and CConstObjectInfo.
* Reduced header dependency.
*
* Revision 1.4  2000/06/16 16:31:07  vasilche
* Changed implementation of choices and classes info to allow use of the same classes in generated and user written classes.
*
* Revision 1.3  2000/06/07 19:45:44  vasilche
* Some code cleaning.
* Macros renaming in more clear way.
* BEGIN_NAMED_*_INFO, ADD_*_MEMBER, ADD_NAMED_*_MEMBER.
*
* Revision 1.2  2000/06/01 19:06:58  vasilche
* Added parsing of XML data.
*
* Revision 1.1  2000/05/24 20:08:14  vasilche
* Implemented XML dump.
*
* ===========================================================================
*/

#include <corelib/ncbistd.hpp>
#include <serial/objostr.hpp>

BEGIN_NCBI_SCOPE

class CObjectOStreamXml : public CObjectOStream
{
public:
    typedef unsigned char TByte;

    CObjectOStreamXml(CNcbiOstream& out, bool deleteOut);
    virtual ~CObjectOStreamXml(void);

    ESerialDataFormat GetDataFormat(void) const;

    virtual void WriteTypeName(const string& name);
    virtual bool WriteEnum(const CEnumeratedTypeValues& values, long value);

protected:
    virtual void WriteBool(bool data);
    virtual void WriteChar(char data);
    virtual void WriteInt(int data);
    virtual void WriteUInt(unsigned data);
    virtual void WriteLong(long data);
    virtual void WriteULong(unsigned long data);
    virtual void WriteDouble(double data);
    virtual void WriteString(const string& str);
    virtual void WriteCString(const char* str);

    virtual void WriteNullPointer(void);
    virtual void WriteObjectReference(TObjectIndex index);
    virtual void WriteOther(TConstObjectPtr object,
                            TTypeInfo typeInfo);
    void WriteId(const string& str);

    void WriteNull(void);
    void WriteEscapedChar(char c);

    virtual void WriteContainer(const CConstObjectInfo& container,
                                CWriteContainerElementsHook& hook);
    virtual void WriteContainerElement(const CConstObjectInfo& element);

    virtual void WriteNamedType(TTypeInfo namedTypeInfo,
                                TTypeInfo typeInfo, TConstObjectPtr object);

    virtual void BeginClass(CObjectStackClass& cls,
                            const CClassTypeInfo* classInfo);
    virtual void EndClass(CObjectStackClass& cls);
    virtual void BeginClassMember(CObjectStackClassMember& m,
                                  const CMemberId& id);
    virtual void EndClassMember(CObjectStackClassMember& m);
    virtual void DoWriteClass(const CConstObjectInfo& object,
                              CWriteClassMembersHook& hook);
    virtual void DoWriteClass(TConstObjectPtr objectPtr,
                              const CClassTypeInfo* objectType);
    virtual void DoWriteClassMember(const CMemberId& id,
                                    const CConstObjectInfo& object,
                                    TMemberIndex index,
                                    CWriteClassMemberHook& hook);
    virtual void DoWriteClassMember(const CMemberId& id,
                                    TConstObjectPtr memberPtr,
                                    TTypeInfo memberType);

    virtual void WriteChoice(const CConstObjectInfo& choice,
                             CWriteChoiceVariantHook& hook);
    void WriteChoiceContents(const CConstObjectInfo& choice,
                             CWriteChoiceVariantHook& hook);
    virtual void WriteChoice(const CConstObjectInfo& choice);
    void WriteChoiceContents(const CConstObjectInfo& choice);

	virtual void BeginBytes(const ByteBlock& block);
	virtual void WriteBytes(const ByteBlock& block,
                            const char* bytes, size_t length);
	virtual void EndBytes(const ByteBlock& block);

private:
    void WriteString(const char* str, size_t length);

    void OpenTagStart(void);
    void OpenTagEnd(void);
    bool CloseTagStart(bool forceEolBefore);
    void CloseTagEnd(void);

    void OpenTag(const string& name);
    void CloseTag(const string& name, bool forceEolBefore = false);
    void OpenTag(const CObjectStackFrame& e);
    void CloseTag(const CObjectStackFrame& e, bool forceEolBefore = false);
    enum ETagAction {
        eTagOpen,
        eTagClose,
        eTagSelfClosed
    };
    ETagAction m_LastTagAction;
};

END_NCBI_SCOPE

#endif
