/***************************************************************************
 *   Copyright (C) 2005, 2006, 2007, 2008, 2009 by Hilscher GmbH           *
 *                                                                         *
 *   Author: Christoph Thelen (cthelen@hilscher.com)                       *
 *                                                                         *
 *   Redistribution or unauthorized use without expressed written          *
 *   agreement from the Hilscher GmbH is forbidden.                        *
 ***************************************************************************/

#ifndef __PLODE_H__
#define __PLODE_H__


typedef struct
{
	unsigned int src_offset : 12;
	unsigned int dst_offset : 12;
	unsigned int size_bytes : 8;
} PLODE_ENTRY_T;

void plode(void *pvDst, const void *pvSrc, const PLODE_ENTRY_T *ptPlodeTable, unsigned int sizPlodeTable);


#endif	/* __PLODE_H__ */

