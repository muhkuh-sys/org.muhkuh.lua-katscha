/***************************************************************************
 *   Copyright (C) 2005, 2006, 2007, 2008, 2009 by Hilscher GmbH           *
 *                                                                         *
 *   Author: Christoph Thelen (cthelen@hilscher.com)                       *
 *                                                                         *
 *   Redistribution or unauthorized use without expressed written          *
 *   agreement from the Hilscher GmbH is forbidden.                        *
 ***************************************************************************/


#include <string.h>

#include "plode.h"


void plode(void *pvDst, const void *pvSrc, const PLODE_ENTRY_T *ptPlodeTable, unsigned int sizPlodeTable)
{
	unsigned char *pucDst;
	const unsigned char *pucSrc;
	const PLODE_ENTRY_T *ptCnt;
	const PLODE_ENTRY_T *ptEnd;


	/* apply storage size to src and dst type */
	pucDst = (unsigned char*)pvDst;
	pucSrc = (const unsigned char*)pvSrc;

	ptCnt = ptPlodeTable;
	ptEnd = ptPlodeTable + sizPlodeTable;
	while( ptCnt<ptEnd )
	{
		/* copy the chunk */
		memcpy(pucDst+ptCnt->dst_offset, pucSrc+ptCnt->src_offset, ptCnt->size_bytes);
		/* next chunk */
		++ptCnt;
	}
}

