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
 *   using the following specifications:
 *   'omssa.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/omssa/MSModSpecSet.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CMSModSpecSet::~CMSModSpecSet(void)
{
}

//! creates arrays for the existing set
void CMSModSpecSet::CreateArrays(void)
{
    Tdata::const_iterator iMods;
    for(iMods = Get().begin(); iMods != Get().end(); ++iMods) {
        int ModNum = (*iMods)->GetMod();
        if(ModNum >= eMSMod_max){
            ERR_POST(Warning << "CMSModSpecSet::CreateArrays modification number unknown");
            continue;
        }
        // ignore unspecified mods
        if((*iMods)->GetMonomass() == 0.0) continue;

        ModTypes[ModNum] = static_cast <EMSModType> ((*iMods)->GetType());
        ModMass[ModNum] = static_cast <int> ((*iMods)->GetMonomass() * MSSCALE);
        strncpy(ModNames[ModNum], (*iMods)->GetName().c_str(), kMaxNameSize - 1);
        // make sure name is null terminated
        ModNames[ModNum][kMaxNameSize] = '\0';

        int iChars(0);
        CMSModSpec::TResidues::const_iterator iRes;
        // loop thru chars
        for(iRes = (*iMods)->GetResidues().begin(); iRes != (*iMods)->GetResidues().end(); ++iRes) {
            if (iChars >= kMaxAAs) {
                ERR_POST(Warning << "CMSModSpecSet::CreateArrays too many AAs in mod " << ModNum);
                break;
            }
            if(strchr(UniqueAA, (*iRes)[0]) != 0)
                ModChar[ModNum][iChars] = strchr(UniqueAA, (*iRes)[0]) - UniqueAA;
            else {
                ERR_POST(Warning << "CMSModSpecSet::CreateArrays unknown AA " <<
                         (*iRes) << " specified for mod " << ModNum);
                 break;
            }

            iChars++;
        }
        NumModChars[ModNum] = iChars;
    }
    isArrayed = true;
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.1  2005/03/14 22:29:54  lewisg
* add mod file input
*
*
* ===========================================================================
*/
/* Original file checksum: lines: 65, chars: 1896, CRC32: ed716b4 */
