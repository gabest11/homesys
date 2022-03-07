/*
 * This file is part of Media Player Classic HomeCinema.
 *
 * MPC-HC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "avcodec.h"
#include "internal.h"

#ifdef __GNUC__
#define _aligned_malloc		__mingw_aligned_malloc
#define _aligned_realloc	__mingw_aligned_realloc
#define _aligned_free		__mingw_aligned_free
#endif

void* FF_aligned_malloc(size_t size, size_t alignment)
{
	return _aligned_malloc(size,alignment);
}

void FF_aligned_free(void* mem_ptr)
{
	if (mem_ptr)
		_aligned_free(mem_ptr);
}

void* FF_aligned_realloc(void *ptr,size_t size,size_t alignment)
{
	if (!ptr)
		return FF_aligned_malloc(size,alignment);
	else
		if (size==0)
		{
			FF_aligned_free(ptr);
			return NULL;
		}
		else
			return _aligned_realloc(ptr,size,alignment);
}
