/* 
 * $Id: FfmpegContext.h 1579 2010-01-30 10:05:05Z casimir666 $
 *
 * (C) 2006-2010 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <dxva.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "./libavcodec/avcodec.h"

extern int av_h264_decode_frame(struct AVCodecContext* avctx, int* nOutPOC, __int64* rtStartTime, __int64* rtStopTime, unsigned char* buf, int buf_size);
extern int av_vc1_decode_frame(struct AVCodecContext *avctx, unsigned char* buf, int buf_size);

enum PCI_Vendors
{
	PCIV_ATI				= 0x1002,
	PCIV_nVidia				= 0x10DE,
	PCIV_Intel				= 0x8086,
	PCIV_S3_Graphics		= 0x5333
};

// === H264 functions
void			FFH264DecodeBuffer(struct AVCodecContext* pAVCtx, BYTE* pBuffer, UINT nSize, int* pFramePOC, int* pOutPOC, REFERENCE_TIME* pOutrtStart, REFERENCE_TIME* pOutrtStop);
int 			FFH264CheckCompatibility(int nWidth, int nHeight, struct AVCodecContext* pAVCtx, BYTE* pBuffer, UINT nSize, int nPCIVendor, LARGE_INTEGER VideoDriverVersion);
HRESULT			FFH264BuildPicParams(DXVA_PicParams_H264* pDXVAPicParams, DXVA_Qmatrix_H264* pDXVAScalingMatrix, int* nFieldType, int* nSliceType, struct AVCodecContext* pAVCtx, int nPCIVendor);
void			FFH264SetCurrentPicture(int nIndex, DXVA_PicParams_H264* pDXVAPicParams, struct AVCodecContext* pAVCtx);
void			FFH264UpdateRefFramesList(DXVA_PicParams_H264* pDXVAPicParams, struct AVCodecContext* pAVCtx);
BOOL			FFH264IsRefFrameInUse(int nFrameNum, struct AVCodecContext* pAVCtx);
void			FF264UpdateRefFrameSliceLong(DXVA_PicParams_H264* pDXVAPicParams, DXVA_Slice_H264_Long* pSlice, struct AVCodecContext* pAVCtx, int vendor);
void 			FFH264SetDxvaSliceLong(struct AVCodecContext* pAVCtx, void* pSliceLong);

// === VC1 functions
HRESULT			FFVC1UpdatePictureParam (DXVA_PictureParameters* pPicParams, struct AVCodecContext* pAVCtx, int* nFieldType, int* nSliceType, BYTE* pBuffer, UINT nSize);
int				FFIsSkipped(struct AVCodecContext* pAVCtx);

// === Mpeg2 functions
HRESULT			FFMpeg2DecodeFrame(DXVA_PictureParameters* pPicParams, DXVA_QmatrixData* m_QMatrixData, DXVA_SliceInfo* pSliceInfo, int* nSliceCount, struct AVCodecContext* pAVCtx, struct AVFrame* pFrame, int* nNextCodecIndex, int* nFieldType, int* nSliceType, BYTE* pBuffer, UINT nSize);

// === Common functions
int				FFGetCodedPicture(struct AVCodecContext* pAVCtx);
BOOL			FFGetAlternateScan(struct AVCodecContext* pAVCtx);

#ifdef __cplusplus
}
#endif