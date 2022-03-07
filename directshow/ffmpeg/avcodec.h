/*
 * copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

// ffmpeg and ffmpeg-mt must have the same header guard
#ifndef AVCODEC_AVCODEC_H
#define AVCODEC_AVCODEC_H

/**
 * @file
 * external API header
 */

#include "ffcodecs.h"
#include "ffImgfmt.h"

#include "avutil.h"

#define LIBAVCODEC_VERSION_MAJOR 52
#define LIBAVCODEC_VERSION_MINOR 93
#define LIBAVCODEC_VERSION_MICRO  0

#define LIBAVCODEC_VERSION_INT  AV_VERSION_INT(LIBAVCODEC_VERSION_MAJOR, \
                                               LIBAVCODEC_VERSION_MINOR, \
                                               LIBAVCODEC_VERSION_MICRO)
#define LIBAVCODEC_VERSION      AV_VERSION(LIBAVCODEC_VERSION_MAJOR,    \
                                           LIBAVCODEC_VERSION_MINOR,    \
                                           LIBAVCODEC_VERSION_MICRO)
#define LIBAVCODEC_BUILD        LIBAVCODEC_VERSION_INT

#define LIBAVCODEC_IDENT        "Lavc" AV_STRINGIFY(LIBAVCODEC_VERSION)

/**
 * Those FF_API_* defines are not part of public API.
 * They may change, break or disappear at any time.
 */
#ifndef FF_API_PALETTE_CONTROL
#define FF_API_PALETTE_CONTROL  (LIBAVCODEC_VERSION_MAJOR < 54)
#endif
#ifndef FF_API_MM_FLAGS
#define FF_API_MM_FLAGS         (LIBAVCODEC_VERSION_MAJOR < 53)
#endif
#ifndef FF_API_OPT_SHOW
#define FF_API_OPT_SHOW         (LIBAVCODEC_VERSION_MAJOR < 53)
#endif
#ifndef FF_API_AUDIO_OLD
#define FF_API_AUDIO_OLD        (LIBAVCODEC_VERSION_MAJOR < 53)
#endif
#ifndef FF_API_VIDEO_OLD
#define FF_API_VIDEO_OLD        (LIBAVCODEC_VERSION_MAJOR < 53)
#endif
#ifndef FF_API_SUBTITLE_OLD
#define FF_API_SUBTITLE_OLD     (LIBAVCODEC_VERSION_MAJOR < 53)
#endif
#ifndef FF_API_USE_LPC
#define FF_API_USE_LPC          (LIBAVCODEC_VERSION_MAJOR < 53)
#endif
#ifndef FF_API_SET_STRING_OLD
#define FF_API_SET_STRING_OLD   (LIBAVCODEC_VERSION_MAJOR < 53)
#endif
#ifndef FF_API_INOFFICIAL
#define FF_API_INOFFICIAL       (LIBAVCODEC_VERSION_MAJOR < 53)
#endif

#define AV_NOPTS_VALUE          INT64_C(0x8000000000000000)
#define AV_TIME_BASE            1000000
static const AVRational AV_TIME_BASE_Q={1, AV_TIME_BASE};

#if LIBAVCODEC_VERSION_MAJOR < 53
#define CodecType AVMediaType

#define CODEC_TYPE_UNKNOWN    AVMEDIA_TYPE_UNKNOWN
#define CODEC_TYPE_VIDEO      AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_AUDIO      AVMEDIA_TYPE_AUDIO
#define CODEC_TYPE_DATA       AVMEDIA_TYPE_DATA
#define CODEC_TYPE_SUBTITLE   AVMEDIA_TYPE_SUBTITLE
#define CODEC_TYPE_ATTACHMENT AVMEDIA_TYPE_ATTACHMENT
#define CODEC_TYPE_NB         AVMEDIA_TYPE_NB
#endif

/**
 * all in native-endian format
 */
enum SampleFormat {
    SAMPLE_FMT_NONE = -1,
    SAMPLE_FMT_U8,              ///< unsigned 8 bits
    SAMPLE_FMT_S16,             ///< signed 16 bits
    SAMPLE_FMT_S32,             ///< signed 32 bits
    SAMPLE_FMT_FLT,             ///< float
    SAMPLE_FMT_DBL,             ///< double
    SAMPLE_FMT_NB               ///< Number of sample formats. DO NOT USE if dynamically linking to libavcodec
};

/* Audio channel masks */
#define CH_FRONT_LEFT             0x00000001
#define CH_FRONT_RIGHT            0x00000002
#define CH_FRONT_CENTER           0x00000004
#define CH_LOW_FREQUENCY          0x00000008
#define CH_BACK_LEFT              0x00000010
#define CH_BACK_RIGHT             0x00000020
#define CH_FRONT_LEFT_OF_CENTER   0x00000040
#define CH_FRONT_RIGHT_OF_CENTER  0x00000080
#define CH_BACK_CENTER            0x00000100
#define CH_SIDE_LEFT              0x00000200
#define CH_SIDE_RIGHT             0x00000400
#define CH_TOP_CENTER             0x00000800
#define CH_TOP_FRONT_LEFT         0x00001000
#define CH_TOP_FRONT_CENTER       0x00002000
#define CH_TOP_FRONT_RIGHT        0x00004000
#define CH_TOP_BACK_LEFT          0x00008000
#define CH_TOP_BACK_CENTER        0x00010000
#define CH_TOP_BACK_RIGHT         0x00020000
#define CH_STEREO_LEFT            0x20000000  ///< Stereo downmix.
#define CH_STEREO_RIGHT           0x40000000  ///< See CH_STEREO_LEFT.

/** Channel mask value used for AVCodecContext2.request_channel_layout
    to indicate that the user requests the channel order of the decoder output
    to be the native codec channel order. */
#define CH_LAYOUT_NATIVE          0x8000000000000000LL

/* Audio channel convenience macros */
#define CH_LAYOUT_MONO              (CH_FRONT_CENTER)
#define CH_LAYOUT_STEREO            (CH_FRONT_LEFT|CH_FRONT_RIGHT)
#define CH_LAYOUT_2_1               (CH_LAYOUT_STEREO|CH_BACK_CENTER)
#define CH_LAYOUT_SURROUND          (CH_LAYOUT_STEREO|CH_FRONT_CENTER)
#define CH_LAYOUT_4POINT0           (CH_LAYOUT_SURROUND|CH_BACK_CENTER)
#define CH_LAYOUT_2_2               (CH_LAYOUT_STEREO|CH_SIDE_LEFT|CH_SIDE_RIGHT)
#define CH_LAYOUT_QUAD              (CH_LAYOUT_STEREO|CH_BACK_LEFT|CH_BACK_RIGHT)
#define CH_LAYOUT_5POINT0           (CH_LAYOUT_SURROUND|CH_SIDE_LEFT|CH_SIDE_RIGHT)
#define CH_LAYOUT_5POINT1           (CH_LAYOUT_5POINT0|CH_LOW_FREQUENCY)
#define CH_LAYOUT_5POINT0_BACK      (CH_LAYOUT_SURROUND|CH_BACK_LEFT|CH_BACK_RIGHT)
#define CH_LAYOUT_5POINT1_BACK      (CH_LAYOUT_5POINT0_BACK|CH_LOW_FREQUENCY)
#define CH_LAYOUT_7POINT0           (CH_LAYOUT_5POINT0|CH_BACK_LEFT|CH_BACK_RIGHT)
#define CH_LAYOUT_7POINT1           (CH_LAYOUT_5POINT1|CH_BACK_LEFT|CH_BACK_RIGHT)
#define CH_LAYOUT_7POINT1_WIDE      (CH_LAYOUT_5POINT1_BACK|\
                                          CH_FRONT_LEFT_OF_CENTER|CH_FRONT_RIGHT_OF_CENTER)
#define CH_LAYOUT_STEREO_DOWNMIX    (CH_STEREO_LEFT|CH_STEREO_RIGHT)

/* in bytes */
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

/**
 * Required number of additionally allocated bytes at the end of the input bitstream for decoding.
 * This is mainly needed because some optimized bitstream readers read
 * 32 or 64 bit at once and could read over the end.<br>
 * Note: If the first 23 bits of the additional bytes are not 0, then damaged
 * MPEG bitstreams could cause overread and segfault.
 */
#define FF_INPUT_BUFFER_PADDING_SIZE 8

/**
 * minimum encoding buffer size
 * Used to avoid some checks during header writing.
 */
#define FF_MIN_BUFFER_SIZE 16384


/**
 * motion estimation type.
 */
enum Motion_Est_ID {
    ME_ZERO = 1,    ///< no search, that is use 0,0 vector whenever one is needed
    ME_FULL,
    ME_LOG,
    ME_PHODS,
    ME_EPZS,        ///< enhanced predictive zonal search
    ME_X1,          ///< reserved for experiments
    ME_HEX,         ///< hexagon based search
    ME_UMH,         ///< uneven multi-hexagon search
    ME_ITER,        ///< iterative search
    ME_TESA,        ///< transformed exhaustive search algorithm
};

enum AVDiscard{
    /* We leave some space between them for extensions (drop some
     * keyframes for intra-only or drop just some bidir frames). */
    AVDISCARD_NONE   =-16, ///< discard nothing
    AVDISCARD_DEFAULT=  0, ///< discard useless packets like 0 size packets in avi
    AVDISCARD_NONREF =  8, ///< discard all non reference
    AVDISCARD_BIDIR  = 16, ///< discard all bidirectional frames
    AVDISCARD_NONKEY = 32, ///< discard all frames except keyframes
    AVDISCARD_ALL    = 48, ///< discard all
};

enum AVColorPrimaries{
    AVCOL_PRI_BT709      =1, ///< also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
    AVCOL_PRI_UNSPECIFIED=2,
    AVCOL_PRI_BT470M     =4,
    AVCOL_PRI_BT470BG    =5, ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
    AVCOL_PRI_SMPTE170M  =6, ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
    AVCOL_PRI_SMPTE240M  =7, ///< functionally identical to above
    AVCOL_PRI_FILM       =8,
    AVCOL_PRI_NB           , ///< Not part of ABI
};

enum AVColorTransferCharacteristic{
    AVCOL_TRC_BT709      =1, ///< also ITU-R BT1361
    AVCOL_TRC_UNSPECIFIED=2,
    AVCOL_TRC_GAMMA22    =4, ///< also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
    AVCOL_TRC_GAMMA28    =5, ///< also ITU-R BT470BG
    AVCOL_TRC_NB           , ///< Not part of ABI
};

enum AVColorSpace{
    AVCOL_SPC_RGB        =0,
    AVCOL_SPC_BT709      =1, ///< also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
    AVCOL_SPC_UNSPECIFIED=2,
    AVCOL_SPC_FCC        =4,
    AVCOL_SPC_BT470BG    =5, ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601
    AVCOL_SPC_SMPTE170M  =6, ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC / functionally identical to above
    AVCOL_SPC_SMPTE240M  =7,
    AVCOL_SPC_NB           , ///< Not part of ABI
};

enum AVColorRange{
    AVCOL_RANGE_UNSPECIFIED=0,
    AVCOL_RANGE_MPEG       =1, ///< the normal 219*2^(n-8) "MPEG" YUV ranges
    AVCOL_RANGE_JPEG       =2, ///< the normal     2^n-1   "JPEG" YUV ranges
    AVCOL_RANGE_NB           , ///< Not part of ABI
};

/**
 *  X   X      3 4 X      X are luma samples,
 *             1 2        1-6 are possible chroma positions
 *  X   X      5 6 X      0 is undefined/unknown position
 */
enum AVChromaLocation{
    AVCHROMA_LOC_UNSPECIFIED=0,
    AVCHROMA_LOC_LEFT       =1, ///< mpeg2/4, h264 default
    AVCHROMA_LOC_CENTER     =2, ///< mpeg1, jpeg, h263
    AVCHROMA_LOC_TOPLEFT    =3, ///< DV
    AVCHROMA_LOC_TOP        =4,
    AVCHROMA_LOC_BOTTOMLEFT =5,
    AVCHROMA_LOC_BOTTOM     =6,
    AVCHROMA_LOC_NB           , ///< Not part of ABI
};

/**
 * LPC analysis type
 */
enum AVLPCType {
    AV_LPC_TYPE_DEFAULT     = -1, ///< use the codec default LPC type
    AV_LPC_TYPE_NONE        =  0, ///< do not use LPC prediction or use all zero coefficients
    AV_LPC_TYPE_FIXED       =  1, ///< fixed LPC coefficients
    AV_LPC_TYPE_LEVINSON    =  2, ///< Levinson-Durbin recursion
    AV_LPC_TYPE_CHOLESKY    =  3, ///< Cholesky factorization
    AV_LPC_TYPE_NB              , ///< Not part of ABI
};

/**
 * H.264 VUI colour primaries (matrix_coefficients)
 */
typedef enum {
    YCbCr_RGB_coeff_GBR                                = 0,
    YCbCr_RGB_coeff_ITUR_BT_709                        = 1,
    YCbCr_RGB_coeff_Unspecified                        = 2,
    YCbCr_RGB_coeff_Reserved                           = 3,
    YCbCr_RGB_coeff_US_Federal_Regulations_2003_73_682 = 4,
    YCbCr_RGB_coeff_ITUR_BT_601_625                    = 5,
    YCbCr_RGB_coeff_ITUR_BT_601_525                    = 6,
    YCbCr_RGB_coeff_SMPTE240M                          = 7,
    YCbCr_RGB_coeff_YCgCo                              = 8
} YCbCr_RGB_MatrixCoefficientsType;

typedef enum {
    VIDEO_FULL_RANGE_TV         = 0,
    VIDEO_FULL_RANGE_PC         = 1,
    VIDEO_FULL_RANGE_INVALID    = 2
} VideoFullRangeType;

typedef struct RcOverride{
    int start_frame;
    int end_frame;
    int qscale; // If this is 0 then quality_factor will be used instead.
    float quality_factor;
} RcOverride;

#define FF_MAX_B_FRAMES 16

/* encoding support
   These flags can be passed in AVCodecContext2.flags before initialization.
   Note: Not everything is supported yet.
*/

#define CODEC_FLAG_QSCALE 0x0002  ///< Use fixed qscale.
#define CODEC_FLAG_4MV    0x0004  ///< 4 MV per MB allowed / advanced prediction for H.263.
#define CODEC_FLAG_QPEL   0x0010  ///< Use qpel MC.
#define CODEC_FLAG_GMC    0x0020  ///< Use GMC.
#define CODEC_FLAG_MV0    0x0040  ///< Always try a MB with MV=<0,0>.
#define CODEC_FLAG_PART   0x0080  ///< Use data partitioning.
/**
 * The parent program guarantees that the input for B-frames containing
 * streams is not written to for at least s->max_b_frames+1 frames, if
 * this is not set the input will be copied.
 */
#define CODEC_FLAG_INPUT_PRESERVED 0x0100
#define CODEC_FLAG_PASS1           0x0200   ///< Use internal 2pass ratecontrol in first pass mode.
#define CODEC_FLAG_PASS2           0x0400   ///< Use internal 2pass ratecontrol in second pass mode.
#define CODEC_FLAG_EXTERN_HUFF     0x1000   ///< Use external Huffman table (for MJPEG).
#define CODEC_FLAG_GRAY            0x2000   ///< Only decode/encode grayscale.
#define CODEC_FLAG_EMU_EDGE        0x4000   ///< Don't draw edges.
#define CODEC_FLAG_PSNR            0x8000   ///< error[?] variables will be set during encoding.
#define CODEC_FLAG_TRUNCATED       0x00010000 /** Input bitstream might be truncated at a random
                                                  location instead of only at frame boundaries. */
#define CODEC_FLAG_NORMALIZE_AQP  0x00020000 ///< Normalize adaptive quantization.
#define CODEC_FLAG_INTERLACED_DCT 0x00040000 ///< Use interlaced DCT.
#define CODEC_FLAG_LOW_DELAY      0x00080000 ///< Force low delay.
#define CODEC_FLAG_ALT_SCAN       0x00100000 ///< Use alternate scan.
#define CODEC_FLAG_GLOBAL_HEADER  0x00400000 ///< Place global headers in extradata instead of every keyframe.
#define CODEC_FLAG_BITEXACT       0x00800000 ///< Use only bitexact stuff (except (I)DCT).
/* Fx : Flag for h263+ extra options */
#define CODEC_FLAG_AC_PRED        0x01000000 ///< H.263 advanced intra coding / MPEG-4 AC prediction
#define CODEC_FLAG_H263P_UMV      0x02000000 ///< unlimited motion vector
#define CODEC_FLAG_CBP_RD         0x04000000 ///< Use rate distortion optimization for cbp.
#define CODEC_FLAG_QP_RD          0x08000000 ///< Use rate distortion optimization for qp selectioon.
#define CODEC_FLAG_H263P_AIV      0x00000008 ///< H.263 alternative inter VLC
#define CODEC_FLAG_OBMC           0x00000001 ///< OBMC
#define CODEC_FLAG_LOOP_FILTER    0x00000800 ///< loop filter
#define CODEC_FLAG_H263P_SLICE_STRUCT 0x10000000
#define CODEC_FLAG_INTERLACED_ME  0x20000000 ///< interlaced motion estimation
#define CODEC_FLAG_SVCD_SCAN_OFFSET 0x40000000 ///< Will reserve space for SVCD scan offset user data.
#define CODEC_FLAG_CLOSED_GOP     0x80000000
#define CODEC_FLAG2_FAST          0x00000001 ///< Allow non spec compliant speedup tricks.
#define CODEC_FLAG2_STRICT_GOP    0x00000002 ///< Strictly enforce GOP size.
#define CODEC_FLAG2_NO_OUTPUT     0x00000004 ///< Skip bitstream encoding.
#define CODEC_FLAG2_LOCAL_HEADER  0x00000008 ///< Place global headers at every keyframe instead of in extradata.
#define CODEC_FLAG2_BPYRAMID      0x00000010 ///< H.264 allow B-frames to be used as references.
#define CODEC_FLAG2_WPRED         0x00000020 ///< H.264 weighted biprediction for B-frames
#define CODEC_FLAG2_MIXED_REFS    0x00000040 ///< H.264 one reference per partition, as opposed to one reference per macroblock
#define CODEC_FLAG2_8X8DCT        0x00000080 ///< H.264 high profile 8x8 transform
#define CODEC_FLAG2_FASTPSKIP     0x00000100 ///< H.264 fast pskip
#define CODEC_FLAG2_AUD           0x00000200 ///< H.264 access unit delimiters
#define CODEC_FLAG2_BRDO          0x00000400 ///< B-frame rate-distortion optimization
#define CODEC_FLAG2_INTRA_VLC     0x00000800 ///< Use MPEG-2 intra VLC table.
#define CODEC_FLAG2_MEMC_ONLY     0x00001000 ///< Only do ME/MC (I frames -> ref, P frame -> ME+MC).
#define CODEC_FLAG2_DROP_FRAME_TIMECODE 0x00002000 ///< timecode is in drop frame format.
#define CODEC_FLAG2_SKIP_RD       0x00004000 ///< RD optimal MB level residual skipping
#define CODEC_FLAG2_CHUNKS        0x00008000 ///< Input bitstream might be truncated at a packet boundaries instead of only at frame boundaries.
#define CODEC_FLAG2_NON_LINEAR_QUANT 0x00010000 ///< Use MPEG-2 nonlinear quantizer.
#define CODEC_FLAG2_BIT_RESERVOIR 0x00020000 ///< Use a bit reservoir when encoding if possible
#define CODEC_FLAG2_MBTREE        0x00040000 ///< Use macroblock tree ratecontrol (x264 only)
#define CODEC_FLAG2_PSY           0x00080000 ///< Use psycho visual optimizations.
#define CODEC_FLAG2_SSIM          0x00100000 ///< Compute SSIM during encoding, error[] values are undefined.
#define CODEC_FLAG2_INTRA_REFRESH 0x00200000 ///< Use periodic insertion of intra blocks instead of keyframes.

/* Unsupported options :
 *              Syntax Arithmetic coding (SAC)
 *              Reference Picture Selection
 *              Independent Segment Decoding */
/* /Fx */
/* codec capabilities */

#define CODEC_CAP_DRAW_HORIZ_BAND 0x0001 ///< Decoder can use draw_horiz_band callback.
/**
 * Codec uses get_buffer() for allocating buffers and supports custom allocators.
 * If not set, it might not use get_buffer() at all or use operations that
 * assume the buffer was allocated by avcodec_default_get_buffer.
 */
#define CODEC_CAP_DR1             0x0002
/* If 'parse_only' field is true, then avcodec_parse_frame() can be used. */
#define CODEC_CAP_PARSE_ONLY      0x0004
#define CODEC_CAP_TRUNCATED       0x0008
/* Codec can export data for HW decoding (XvMC). */
#define CODEC_CAP_HWACCEL         0x0010
/**
 * Codec has a nonzero delay and needs to be fed with NULL at the end to get the delayed data.
 * If this is not set, the codec is guaranteed to never be fed with NULL data.
 */
#define CODEC_CAP_DELAY           0x0020
/**
 * Codec can be fed a final frame with a smaller size.
 * This can be used to prevent truncation of the last audio samples.
 */
#define CODEC_CAP_SMALL_LAST_FRAME 0x0040
/**
 * Codec can export data for HW decoding (VDPAU).
 */
#define CODEC_CAP_HWACCEL_VDPAU    0x0080
/**
 * Codec can output multiple frames per AVPacket2
 * Normally demuxers return one frame at a time, demuxers which do not do
 * are connected to a parser to split what they return into proper frames.
 * This flag is reserved to the very rare category of codecs which have a
 * bitstream that cannot be split into frames without timeconsuming
 * operations like full decoding. Demuxers carring such bitstreams thus
 * may return multiple frames in a packet. This has many disadvantages like
 * prohibiting stream copy in many cases thus it should only be considered
 * as a last resort.
 */
#define CODEC_CAP_SUBFRAMES        0x0100
/**
 * Codec is experimental and is thus avoided in favor of non experimental
 * encoders
 */
#define CODEC_CAP_EXPERIMENTAL     0x0200
/**
 * Codec should fill in channel configuration and samplerate instead of container
 */
#define CODEC_CAP_CHANNEL_CONF     0x0400
/**
 * Codec supports frame-level multithreading.
 */
#define CODEC_CAP_FRAME_THREADS    0x0800


//The following defines may change, don't expect compatibility if you use them.
#define MB_TYPE_INTRA4x4   0x0001
#define MB_TYPE_INTRA16x16 0x0002 //FIXME H.264-specific
#define MB_TYPE_INTRA_PCM  0x0004 //FIXME H.264-specific
#define MB_TYPE_16x16      0x0008
#define MB_TYPE_16x8       0x0010
#define MB_TYPE_8x16       0x0020
#define MB_TYPE_8x8        0x0040
#define MB_TYPE_INTERLACED 0x0080
#define MB_TYPE_DIRECT2    0x0100 //FIXME
#define MB_TYPE_ACPRED     0x0200
#define MB_TYPE_GMC        0x0400
#define MB_TYPE_SKIP       0x0800
#define MB_TYPE_P0L0       0x1000
#define MB_TYPE_P1L0       0x2000
#define MB_TYPE_P0L1       0x4000
#define MB_TYPE_P1L1       0x8000
#define MB_TYPE_L0         (MB_TYPE_P0L0 | MB_TYPE_P1L0)
#define MB_TYPE_L1         (MB_TYPE_P0L1 | MB_TYPE_P1L1)
#define MB_TYPE_L0L1       (MB_TYPE_L0   | MB_TYPE_L1)
#define MB_TYPE_QUANT      0x00010000
#define MB_TYPE_CBP        0x00020000
//Note bits 24-31 are reserved for codec specific use (h264 ref0, mpeg1 0mv, ...)

/**
 * Pan Scan area.
 * This specifies the area which should be displayed.
 * Note there may be multiple such areas for one frame.
 */
typedef struct AVPanScan2 {
    /**
     * id
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
    int id;

    /**
     * width and height in 1/16 pel
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
    int width;
    int height;

    /**
     * position of the top left corner in 1/16 pel for up to 3 fields/frames
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
    int16_t position[3][2];
}AVPanScan2;

#define FF_COMMON_FRAME \
    /**\
     * pointer to the picture planes.\
     * This might be different from the first allocated byte\
     * - encoding: \
     * - decoding: \
     */\
    uint8_t *data[4];\
    int linesize[4];\
    /**\
     * pointer to the first allocated byte of the picture. Can be used in get_buffer/release_buffer.\
     * This isn't used by libavcodec unless the default get/release_buffer() is used.\
     * - encoding: \
     * - decoding: \
     */\
    uint8_t *base[4];\
    /**\
     * 1 -> keyframe, 0-> not\
     * - encoding: Set by libavcodec.\
     * - decoding: Set by libavcodec.\
     */\
    int key_frame;\
\
    /**\
     * Picture type of the frame, see ?_TYPE below.\
     * - encoding: Set by libavcodec. for coded_picture (and set by user for input).\
     * - decoding: Set by libavcodec.\
     */\
    int pict_type;\
\
    /**\
     * presentation timestamp in time_base units (time when frame should be shown to user)\
     * If AV_NOPTS_VALUE then frame_rate = 1/time_base will be assumed.\
     * - encoding: MUST be set by user.\
     * - decoding: Set by libavcodec.\
     */\
    int64_t pts;\
\
    /**\
     * picture number in bitstream order\
     * - encoding: set by\
     * - decoding: Set by libavcodec.\
     */\
    int coded_picture_number;\
    /**\
     * picture number in display order\
     * - encoding: set by\
     * - decoding: Set by libavcodec.\
     */\
    int display_picture_number;\
\
    /**\
     * quality (between 1 (good) and FF_LAMBDA_MAX (bad)) \
     * - encoding: Set by libavcodec. for coded_picture (and set by user for input).\
     * - decoding: Set by libavcodec.\
     */\
    int quality; \
\
    /**\
     * buffer age (1->was last buffer and dint change, 2->..., ...).\
     * Set to INT_MAX if the buffer has not been used yet.\
     * - encoding: unused\
     * - decoding: MUST be set by get_buffer().\
     */\
    int age;\
\
    /**\
     * is this picture used as reference\
     * The values for this are the same as the MpegEncContext.picture_structure\
     * variable, that is 1->top field, 2->bottom field, 3->frame/both fields.\
     * Set to 4 for delayed, non-reference frames.\
     * - encoding: unused\
     * - decoding: Set by libavcodec. (before get_buffer() call)).\
     */\
    int reference;\
\
    /**\
     * QP table\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */\
    int8_t *qscale_table;\
    /**\
     * QP store stride\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */\
    int qstride;\
\
    /**\
     * mbskip_table[mb]>=1 if MB didn't change\
     * stride= mb_width = (width+15)>>4\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */\
    uint8_t *mbskip_table;\
\
    /**\
     * motion vector table\
     * @code\
     * example:\
     * int mv_sample_log2= 4 - motion_subsample_log2;\
     * int mb_width= (width+15)>>4;\
     * int mv_stride= (mb_width << mv_sample_log2) + 1;\
     * motion_val[direction][x + y*mv_stride][0->mv_x, 1->mv_y];\
     * @endcode\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */\
    int16_t (*motion_val[2])[2];\
\
    /**\
     * macroblock type table\
     * mb_type_base + mb_width + 2\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */\
    uint32_t *mb_type;\
\
    /**\
     * log2 of the size of the block which a single vector in motion_val represents: \
     * (4->16x16, 3->8x8, 2-> 4x4, 1-> 2x2)\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */\
    uint8_t motion_subsample_log2;\
\
    /**\
     * for some private data of the user\
     * - encoding: unused\
     * - decoding: Set by user.\
     */\
    void *opaque;\
\
    /**\
     * error\
     * - encoding: Set by libavcodec. if flags&CODEC_FLAG_PSNR.\
     * - decoding: unused\
     */\
    uint64_t error[4];\
\
    /**\
     * type of the buffer (to keep track of who has to deallocate data[*])\
     * - encoding: Set by the one who allocates it.\
     * - decoding: Set by the one who allocates it.\
     * Note: User allocated (direct rendering) & internal buffers cannot coexist currently.\
     */\
    int type;\
    \
    /**\
     * When decoding, this signals how much the picture must be delayed.\
     * extra_delay = repeat_pict / (2*fps)\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */\
    int repeat_pict;\
    \
    /**\
     * \
     */\
    int qscale_type;\
    \
    /**\
     * The content of the picture is interlaced.\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec. (default 0)\
     */\
    int interlaced_frame;\
    \
    /**\
     * If the content is interlaced, is top field displayed first.\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */\
    int top_field_first;\
    \
    /**\
     * Pan scan.\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */\
    AVPanScan2 *pan_scan;\
    \
    /**\
     * Tell user application that palette has changed from previous frame.\
     * - encoding: ??? (no palette-enabled encoder yet)\
     * - decoding: Set by libavcodec. (default 0).\
     */\
    int palette_has_changed;\
    \
    /**\
     * codec suggestion on buffer type if != 0\
     * - encoding: unused\
     * - decoding: Set by libavcodec. (before get_buffer() call)).\
     */\
    int buffer_hints;\
\
    /**\
     * DCT coefficients\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */\
    short *dct_coeff;\
\
    /**\
     * motion reference frame index\
     * the order in which these are stored can depend on the codec.\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */\
    int8_t *ref_index[2];\
\
    /**\
     * reordered opaque 64bit number (generally a PTS) from AVCodecContext2.reordered_opaque\
     * output in AVFrame2.reordered_opaque\
     * - encoding: unused\
     * - decoding: Read by user.\
     */\
    int64_t reordered_opaque;\
    int64_t reordered_opaque2; /* ffdshow custom code */\
    int64_t reordered_opaque3; /* ffdshow custom code */\
\
    /**\
     * hardware accelerator private data (FFmpeg allocated)\
     * - encoding: unused\
     * - decoding: Set by libavcodec\
     */\
    void *hwaccel_picture_private;\
\
    /* ffdshow custom code */\
    int mb_width,mb_height,mb_stride,b8_stride;\
    int num_sprite_warping_points,real_sprite_warping_points;\
    int play_flags;\
\
    /* ffdshow custom stuff (begin) */\
\
    /**\
     * the AVCodecContext2 which ff_thread_get_buffer() was last called on\
     * - encoding: Set by libavcodec.\
     * - decoding: Set by libavcodec.\
     */\
    struct AVCodecContext2 *owner;\
\
    /**\
     * used by multithreading to store frame-specific info\
     * - encoding: Set by libavcodec.\
     * - decoding: Set by libavcodec.\
     */\
    void *thread_opaque;\
\
    int h264_poc_decoded;\
    int h264_poc_outputed;\
    int h264_frame_num_decoded;\
    int h264_max_frame_num;\
\
    int mpeg2_sequence_end_flag;\
\
    /**\
     * video_full_range_flag\
     * - encoding: unused\
     * - decoding: Set by libavcodec.  -1: invalid, 0: TV (16-235) 1: PC (1-254)\
     */\
    VideoFullRangeType video_full_range_flag;\
    /**\
     * YCbCr_RGB_matrix_coefficients\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */\
    YCbCr_RGB_MatrixCoefficientsType YCbCr_RGB_matrix_coefficients;
    /* ffdshow custom stuff (end) */


#define FF_QSCALE_TYPE_MPEG1 0
#define FF_QSCALE_TYPE_MPEG2 1
#define FF_QSCALE_TYPE_H264  2
#define FF_QSCALE_TYPE_VP56  3

#define FF_BUFFER_TYPE_INTERNAL 1
#define FF_BUFFER_TYPE_USER     2 ///< direct rendering buffers (image is (de)allocated by user)
#define FF_BUFFER_TYPE_SHARED   4 ///< Buffer from somewhere else; don't deallocate image (data/base), all other tables are not shared.
#define FF_BUFFER_TYPE_COPY     8 ///< Just a (modified) copy of some other buffer, don't deallocate anything.


#define FF_I_TYPE  1 ///< Intra
#define FF_P_TYPE  2 ///< Predicted
#define FF_B_TYPE  3 ///< Bi-dir predicted
#define FF_S_TYPE  4 ///< S(GMC)-VOP MPEG4
#define FF_SI_TYPE 5 ///< Switching Intra
#define FF_SP_TYPE 6 ///< Switching Predicted
#define FF_BI_TYPE 7

#define FF_BUFFER_HINTS_VALID    0x01 // Buffer hints value is meaningful (if 0 ignore).
#define FF_BUFFER_HINTS_READABLE 0x02 // Codec will read from buffer.
#define FF_BUFFER_HINTS_PRESERVE 0x04 // User must not alter buffer content.
#define FF_BUFFER_HINTS_REUSABLE 0x08 // Codec will reuse the buffer (update).

typedef struct AVPacket2 {
    /**
     * Presentation timestamp in AVStream->time_base units; the time at which
     * the decompressed packet will be presented to the user.
     * Can be AV_NOPTS_VALUE if it is not stored in the file.
     * pts MUST be larger or equal to dts as presentation cannot happen before
     * decompression, unless one wants to view hex dumps. Some formats misuse
     * the terms dts and pts/cts to mean something different. Such timestamps
     * must be converted to true pts/dts before they are stored in AVPacket2.
     */
    int64_t pts;
    /**
     * Decompression timestamp in AVStream->time_base units; the time at which
     * the packet is decompressed.
     * Can be AV_NOPTS_VALUE if it is not stored in the file.
     */
    int64_t dts;
    uint8_t *data;
    int   size;
    int   stream_index;
    int   flags;
    /**
     * Duration of this packet in AVStream->time_base units, 0 if unknown.
     * Equals next_pts - this_pts in presentation order.
     */
    int   duration;
    void  (*destruct)(struct AVPacket2 *);
    void  *priv;
    int64_t pos;                            ///< byte position in stream, -1 if unknown

    /**
     * Time difference in AVStream->time_base units from the pts of this
     * packet to the point at which the output from the decoder has converged
     * independent from the availability of previous frames. That is, the
     * frames are virtually identical no matter if decoding started from
     * the very first frame or from this keyframe.
     * Is AV_NOPTS_VALUE if unknown.
     * This field is not the display duration of the current packet.
     * This field has no meaning if the packet does not have AV_PKT_FLAG_KEY
     * set.
     *
     * The purpose of this field is to allow seeking in streams that have no
     * keyframes in the conventional sense. It corresponds to the
     * recovery point SEI in H.264 and match_time_delta in NUT. It is also
     * essential for some types of subtitle streams to ensure that all
     * subtitles are correctly displayed after seeking.
     */
    int64_t convergence_duration;
} AVPacket2;
#define AV_PKT_FLAG_KEY   0x0001
#if LIBAVCODEC_VERSION_MAJOR < 53
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#endif

/**
 * Audio Video Frame.
 * New fields can be added to the end of FF_COMMON_FRAME with minor version
 * bumps.
 * Removal, reordering and changes to existing fields require a major
 * version bump. No fields should be added into AVFrame2 before or after
 * FF_COMMON_FRAME!
 * sizeof(AVFrame2) must not be used outside libav*.
 */
typedef struct AVFrame2 {
    FF_COMMON_FRAME
} AVFrame2;

/**
 * main external API structure.
 * New fields can be added to the end with minor version bumps.
 * Removal, reordering and changes to existing fields require a major
 * version bump.
 * sizeof(AVCodecContext2) must not be used outside libav*.
 */
struct TlibavcodecExt2;
typedef struct AVCodecContext2 {
    /**
     * information on struct for av_log
     * - set by avcodec_alloc_context
     */
    const AVClass *av_class;
    /**
     * the average bitrate
     * - encoding: Set by user; unused for constant quantizer encoding.
     * - decoding: Set by libavcodec. 0 or some bitrate if this info is available in the stream.
     */
    int bit_rate;

    /**
     * number of bits the bitstream is allowed to diverge from the reference.
     *           the reference can be CBR (for CBR pass1) or VBR (for pass2)
     * - encoding: Set by user; unused for constant quantizer encoding.
     * - decoding: unused
     */
    int bit_rate_tolerance;

    /**
     * CODEC_FLAG_*.
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int flags;

    /**
     * Some codecs need additional format info. It is stored here.
     * If any muxer uses this then ALL demuxers/parsers AND encoders for the
     * specific codec MUST set it correctly otherwise stream copy breaks.
     * In general use of this field by muxers is not recommanded.
     * - encoding: Set by libavcodec.
     * - decoding: Set by libavcodec. (FIXME: Is this OK?)
     */
    int sub_id;

    /**
     * Motion estimation algorithm used for video coding.
     * 1 (zero), 2 (full), 3 (log), 4 (phods), 5 (epzs), 6 (x1), 7 (hex),
     * 8 (umh), 9 (iter), 10 (tesa) [7, 8, 10 are x264 specific, 9 is snow specific]
     * - encoding: MUST be set by user.
     * - decoding: unused
     */
    int me_method;

    /**
     * some codecs need / can use extradata like Huffman tables.
     * mjpeg: Huffman tables
     * rv10: additional flags
     * mpeg4: global headers (they can be in the bitstream or here)
     * The allocated memory should be FF_INPUT_BUFFER_PADDING_SIZE bytes larger
     * than extradata_size to avoid prolems if it is read with the bitstream reader.
     * The bytewise contents of extradata must not depend on the architecture or CPU endianness.
     * - encoding: Set/allocated/freed by libavcodec.
     * - decoding: Set/allocated/freed by user.
     */
    const uint8_t *extradata;
    int extradata_size;

    /**
     * This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identically 1.
     * - encoding: MUST be set by user.
     * - decoding: Set by libavcodec.
     */
    AVRational time_base;

    /* video only */
    /**
     * picture width / height.
     * - encoding: MUST be set by user.
     * - decoding: Set by libavcodec.
     * Note: For compatibility it is possible to set this instead of
     * coded_width/height before decoding.
     */
    int width, height;

#define FF_ASPECT_EXTENDED 15

    /**
     * the number of pictures in a group of pictures, or 0 for intra_only
     * - encoding: Set by user.
     * - decoding: unused
     */
    int gop_size;

    /**
     * Pixel format, see PIX_FMT_xxx.
     * May be set by the demuxer if known from headers.
     * May be overriden by the decoder if it knows better.
     * - encoding: Set by user.
     * - decoding: Set by user if known, overridden by libavcodec if known
     */
    enum PixelFormat pix_fmt;

    /**
     * Frame rate emulation. If not zero, the lower layer (i.e. format handler)
     * has to read frames at native frame rate.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int rate_emu;

    /**
     * If non NULL, 'draw_horiz_band' is called by the libavcodec
     * decoder to draw a horizontal band. It improves cache usage. Not
     * all codecs can do that. You must check the codec capabilities
     * beforehand. May be called by different threads at the same time.
     * The function is also used by hardware acceleration APIs.
     * It is called at least once during frame decoding to pass
     * the data needed for hardware render.
     * In that mode instead of pixel data, AVFrame2 points to
     * a structure specific to the acceleration API. The application
     * reads the structure and can change some fields to indicate progress
     * or mark state.
     * - encoding: unused
     * - decoding: Set by user.
     * @param height the height of the slice
     * @param y the y position of the slice
     * @param type 1->top field, 2->bottom field, 3->frame
     * @param offset offset into the AVFrame2.data from which the slice should be read
     */
    void (*draw_horiz_band)(struct AVCodecContext2 *s,
                            const AVFrame2 *src, int offset[4],
                            int y, int type, int height);

    /* audio only */
    int sample_rate; ///< samples per second
    int channels;    ///< number of audio channels

    /**
     * audio sample format
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
    enum SampleFormat sample_fmt;  ///< sample format

    /* The following data should not be initialized. */
    /**
     * Samples per packet, initialized when calling 'init'.
     */
    int frame_size;
    int frame_number;   ///< audio or video frame number
#if LIBAVCODEC_VERSION_MAJOR < 53
    int real_pict_num;  ///< Returns the real picture number of previous encoded frame.
#endif

    /**
     * Number of frames the decoded output will be delayed relative to
     * the encoded input.
     * - encoding: Set by libavcodec.
     * - decoding: unused
     */
    int delay;

    /* - encoding parameters */
    float qcompress;  ///< amount of qscale change between easy & hard scenes (0.0-1.0)
    float qblur;      ///< amount of qscale smoothing over time (0.0-1.0)

    /**
     * minimum quantizer
     * - encoding: Set by user.
     * - decoding: unused
     */
    int qmin;

    /**
     * maximum quantizer
     * - encoding: Set by user.
     * - decoding: unused
     */
    int qmax;

    /**
     * maximum quantizer difference between frames
     * - encoding: Set by user.
     * - decoding: unused
     */
    int max_qdiff;

    /**
     * maximum number of B-frames between non-B-frames
     * Note: The output will be delayed by max_b_frames+1 relative to the input.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int max_b_frames;

    /**
     * qscale factor between IP and B-frames
     * If > 0 then the last P-frame quantizer will be used (q= lastp_q*factor+offset).
     * If < 0 then normal ratecontrol will be done (q= -normal_q*factor+offset).
     * - encoding: Set by user.
     * - decoding: unused
     */
    float b_quant_factor;

    /** obsolete FIXME remove */
    int rc_strategy;
#define FF_RC_STRATEGY_XVID 1

    int b_frame_strategy;

    /**
     * hurry up amount
     * - encoding: unused
     * - decoding: Set by user. 1-> Skip B-frames, 2-> Skip IDCT/dequant too, 5-> Skip everything except header
     * @deprecated Deprecated in favor of skip_idct and skip_frame.
     */
    int hurry_up;

    struct AVCodec2 *codec;

    void *priv_data;

    int rtp_payload_size;   /* The size of the RTP payload: the coder will  */
                            /* do its best to deliver a chunk with size     */
                            /* below rtp_payload_size, the chunk will start */
                            /* with a start code on some codecs like H.263. */
                            /* This doesn't take account of any particular  */
                            /* headers inside the transmitted RTP payload.  */


    /* The RTP callback: This function is called    */
    /* every time the encoder has a packet to send. */
    /* It depends on the encoder if the data starts */
    /* with a Start Code (it should). H.263 does.   */
    /* mb_nb contains the number of macroblocks     */
    /* encoded in the RTP payload.                  */
    void (*rtp_callback)(struct AVCodecContext2 *avctx, void *data, int size, int mb_nb);

    /* statistics, used for 2-pass encoding */
    int mv_bits;
    int header_bits;
    int i_tex_bits;
    int p_tex_bits;
    int i_count;
    int p_count;
    int skip_count;
    int misc_bits;

    /**
     * number of bits used for the previously encoded frame
     * - encoding: Set by libavcodec.
     * - decoding: unused
     */
    int frame_bits;

    /**
     * Private data of the user, can be used to carry app specific stuff.
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    struct TlibavcodecExt2 *opaque;

    char codec_name[32];
    enum AVMediaType codec_type; /* see AVMEDIA_TYPE_xxx */
    enum CodecID codec_id; /* see CODEC_ID_xxx */

    /**
     * fourcc (LSB first, so "ABCD" -> ('D'<<24) + ('C'<<16) + ('B'<<8) + 'A').
     * This is used to work around some encoder bugs.
     * A demuxer should set this to what is stored in the field used to identify the codec.
     * If there are multiple such fields in a container then the demuxer should choose the one
     * which maximizes the information about the used codec.
     * If the codec tag field in a container is larger then 32 bits then the demuxer should
     * remap the longer ID to 32 bits with a table or other structure. Alternatively a new
     * extra_codec_tag + size could be added but for this a clear advantage must be demonstrated
     * first.
     * - encoding: Set by user, if not then the default based on codec_id will be used.
     * - decoding: Set by user, will be converted to uppercase by libavcodec during init.
     */
    unsigned int codec_tag;

    /**
     * Work around bugs in encoders which sometimes cannot be detected automatically.
     * - encoding: Set by user
     * - decoding: Set by user
     */
    int workaround_bugs;
#define FF_BUG_AUTODETECT       1  ///< autodetection
#define FF_BUG_OLD_MSMPEG4      2
#define FF_BUG_XVID_ILACE       4
#define FF_BUG_UMP4             8
#define FF_BUG_NO_PADDING       16
#define FF_BUG_AMV              32
#define FF_BUG_AC_VLC           0  ///< Will be removed, libavcodec can now handle these non-compliant files by default.
#define FF_BUG_QPEL_CHROMA      64
#define FF_BUG_STD_QPEL         128
#define FF_BUG_QPEL_CHROMA2     256
#define FF_BUG_DIRECT_BLOCKSIZE 512
#define FF_BUG_EDGE             1024
#define FF_BUG_HPEL_CHROMA      2048
#define FF_BUG_DC_CLIP          4096
#define FF_BUG_MS               8192 ///< Work around various bugs in Microsoft's broken decoders.
#define FF_BUG_TRUNCATED       16384
//#define FF_BUG_FAKE_SCALABILITY 16 //Autodetection should work 100%.

    /**
     * luma single coefficient elimination threshold
     * - encoding: Set by user.
     * - decoding: unused
     */
    int luma_elim_threshold;

    /**
     * chroma single coeff elimination threshold
     * - encoding: Set by user.
     * - decoding: unused
     */
    int chroma_elim_threshold;

    /**
     * strictly follow the standard (MPEG4, ...).
     * - encoding: Set by user.
     * - decoding: Set by user.
     * Setting this to STRICT or higher means the encoder and decoder will
     * generally do stupid things, whereas setting it to unofficial or lower
     * will mean the encoder might produce output that is not supported by all
     * spec-compliant decoders. Decoders don't differentiate between normal,
     * unofficial and experimental (that is, they always try to decode things
     * when they can) unless they are explicitly asked to behave stupidly
     * (=strictly conform to the specs)
     */
    int strict_std_compliance;
#define FF_COMPLIANCE_VERY_STRICT   2 ///< Strictly conform to an older more strict version of the spec or reference software.
#define FF_COMPLIANCE_STRICT        1 ///< Strictly conform to all the things in the spec no matter what consequences.
#define FF_COMPLIANCE_NORMAL        0
#if FF_API_INOFFICIAL
#define FF_COMPLIANCE_INOFFICIAL   -1 ///< Allow inofficial extensions (deprecated - use FF_COMPLIANCE_UNOFFICIAL instead).
#endif
#define FF_COMPLIANCE_UNOFFICIAL   -1 ///< Allow unofficial extensions
#define FF_COMPLIANCE_EXPERIMENTAL -2 ///< Allow nonstandardized experimental things.

    /**
     * qscale offset between IP and B-frames
     * - encoding: Set by user.
     * - decoding: unused
     */
    float b_quant_offset;

    /**
     * Error recognization; higher values will detect more errors but may
     * misdetect some more or less valid parts as errors.
     * - encoding: unused
     * - decoding: Set by user.
     */
    int error_recognition;
#define FF_ER_CAREFUL         1
#define FF_ER_COMPLIANT       2
#define FF_ER_AGGRESSIVE      3
#define FF_ER_VERY_AGGRESSIVE 4

    /**
     * Called at the beginning of each frame to get a buffer for it.
     * If pic.reference is set then the frame will be read later by libavcodec.
     * avcodec_align_dimensions2() should be used to find the required width and
     * height, as they normally need to be rounded up to the next multiple of 16.
     * if CODEC_CAP_DR1 is not set then get_buffer() must call
     * avcodec_default_get_buffer() instead of providing buffers allocated by
     * some other means. May be called from a different thread if FF_THREAD_FRAME
     * is set, but does not need to be reentrant.
     * - encoding: unused
     * - decoding: Set by libavcodec, user can override.
     */
    int (*get_buffer)(struct AVCodecContext2 *c, AVFrame2 *pic);

    /**
     * Called to release buffers which were allocated with get_buffer.
     * A released buffer can be reused in get_buffer().
     * pic.data[*] must be set to NULL. May be called by different threads
     * if frame threading is enabled, but not more than one at the same time.
     *
     * - encoding: unused
     * - decoding: Set by libavcodec, user can override.
     */
    void (*release_buffer)(struct AVCodecContext2 *c, AVFrame2 *pic);

    /**
     * Size of the frame reordering buffer in the decoder.
     * For MPEG-2 it is 1 IPB or 0 low delay IP.
     * - encoding: Set by libavcodec.
     * - decoding: Set by libavcodec.
     */
    int has_b_frames;

    /**
     * number of bytes per packet if constant and known or 0
     * Used by some WAV based audio codecs.
     */
    int block_align;

    int parse_only; /* - decoding only: If true, only parsing is done
                       (function avcodec_parse_frame()). The frame
                       data is returned. Only MPEG codecs support this now. */

    /**
     * 0-> h263 quant 1-> mpeg quant
     * - encoding: Set by user.
     * - decoding: unused
     */
    int mpeg_quant;

    /**
     * pass1 encoding statistics output buffer
     * - encoding: Set by libavcodec.
     * - decoding: unused
     */
    char *stats_out;

    /**
     * pass2 encoding statistics input buffer
     * Concatenated stuff from stats_out of pass1 should be placed here.
     * - encoding: Allocated/set/freed by user.
     * - decoding: unused
     */
    char *stats_in;

    /**
     * ratecontrol qmin qmax limiting method
     * 0-> clipping, 1-> use a nice continous function to limit qscale wthin qmin/qmax.
     * - encoding: Set by user.
     * - decoding: unused
     */
    float rc_qsquish;

    float rc_qmod_amp;
    int rc_qmod_freq;

    /**
     * ratecontrol override, see RcOverride
     * - encoding: Allocated/set/freed by user.
     * - decoding: unused
     */
    RcOverride *rc_override;
    int rc_override_count;

    /**
     * rate control equation
     * - encoding: Set by user
     * - decoding: unused
     */
    const char *rc_eq;

    /**
     * maximum bitrate
     * - encoding: Set by user.
     * - decoding: unused
     */
    int rc_max_rate;

    /**
     * minimum bitrate
     * - encoding: Set by user.
     * - decoding: unused
     */
    int rc_min_rate;

    /**
     * decoder bitstream buffer size
     * - encoding: Set by user.
     * - decoding: unused
     */
    int rc_buffer_size;
    float rc_buffer_aggressivity;

    /**
     * qscale factor between P and I-frames
     * If > 0 then the last p frame quantizer will be used (q= lastp_q*factor+offset).
     * If < 0 then normal ratecontrol will be done (q= -normal_q*factor+offset).
     * - encoding: Set by user.
     * - decoding: unused
     */
    float i_quant_factor;

    /**
     * qscale offset between P and I-frames
     * - encoding: Set by user.
     * - decoding: unused
     */
    float i_quant_offset;

    /**
     * initial complexity for pass1 ratecontrol
     * - encoding: Set by user.
     * - decoding: unused
     */
    float rc_initial_cplx;

    /**
     * DCT algorithm, see FF_DCT_* below
     * - encoding: Set by user.
     * - decoding: unused
     */
    int dct_algo;
#define FF_DCT_AUTO    0
#define FF_DCT_FASTINT 1
#define FF_DCT_INT     2
#define FF_DCT_MMX     3
#define FF_DCT_MLIB    4
#define FF_DCT_ALTIVEC 5
#define FF_DCT_FAAN    6

    /**
     * luminance masking (0-> disabled)
     * - encoding: Set by user.
     * - decoding: unused
     */
    float lumi_masking;

    /**
     * temporary complexity masking (0-> disabled)
     * - encoding: Set by user.
     * - decoding: unused
     */
    float temporal_cplx_masking;

    /**
     * spatial complexity masking (0-> disabled)
     * - encoding: Set by user.
     * - decoding: unused
     */
    float spatial_cplx_masking;

    /**
     * p block masking (0-> disabled)
     * - encoding: Set by user.
     * - decoding: unused
     */
    float p_masking;

    /**
     * darkness masking (0-> disabled)
     * - encoding: Set by user.
     * - decoding: unused
     */
    float dark_masking;

    /**
     * IDCT algorithm, see FF_IDCT_* below.
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int idct_algo;
/*
 * idct_algo is set by ffdshow.ax.
 * It uses the list "idctNames[]" found in Tlibavcodec.cpp.
 * The indexes of the items in that list should match with the values below.
 */
#define FF_IDCT_AUTO         0
#define FF_IDCT_LIBMPEG2MMX  1
#define FF_IDCT_SIMPLEMMX    2
#define FF_IDCT_XVIDMMX      3
#define FF_IDCT_SIMPLE       4
#define FF_IDCT_INT          5
#define FF_IDCT_FAAN         6
#define FF_IDCT_H264         7
#define FF_IDCT_VP3          8
#define FF_IDCT_CAVS         9
#define FF_IDCT_WMV2         10

    /**
     * slice count
     * - encoding: Set by libavcodec.
     * - decoding: Set by user (or 0).
     */
    int slice_count;
    /**
     * slice offsets in the frame in bytes
     * - encoding: Set/allocated by libavcodec.
     * - decoding: Set/allocated by user (or NULL).
     */
    int *slice_offset;

    /**
     * error concealment flags
     * - encoding: unused
     * - decoding: Set by user.
     */
    int error_concealment;
#define FF_EC_GUESS_MVS   1
#define FF_EC_DEBLOCK     2

    /**
     * dsp_mask could be add used to disable unwanted CPU features
     * CPU features (i.e. MMX, SSE. ...)
     *
     * With the FORCE flag you may instead enable given CPU features.
     * (Dangerous: Usable in case of misdetection, improper usage however will
     * result into program crash.)
     */
    unsigned dsp_mask;

#if FF_API_MM_FLAGS
#define FF_MM_FORCE      AV_CPU_FLAG_FORCE
#define FF_MM_MMX        AV_CPU_FLAG_MMX
#define FF_MM_3DNOW      AV_CPU_FLAG_3DNOW
#define FF_MM_MMXEXT     AV_CPU_FLAG_MMX2
#define FF_MM_MMX2       AV_CPU_FLAG_MMX2
#define FF_MM_SSE        AV_CPU_FLAG_SSE
#define FF_MM_SSE2       AV_CPU_FLAG_SSE2
#define FF_MM_SSE2SLOW   AV_CPU_FLAG_SSE2SLOW
#define FF_MM_3DNOWEXT   AV_CPU_FLAG_3DNOWEXT
#define FF_MM_SSE3       AV_CPU_FLAG_SSE3
#define FF_MM_SSE3SLOW   AV_CPU_FLAG_SSE3SLOW
#define FF_MM_SSSE3      AV_CPU_FLAG_SSSE3
#define FF_MM_SSE4       AV_CPU_FLAG_SSE4
#define FF_MM_SSE42      AV_CPU_FLAG_SSE42
#define FF_MM_IWMMXT     AV_CPU_FLAG_IWMMXT
#define FF_MM_ALTIVEC    AV_CPU_FLAG_ALTIVEC
#endif

    /**
     * bits per sample/pixel from the demuxer (needed for huffyuv).
     * - encoding: Set by libavcodec.
     * - decoding: Set by user.
     */
     int bits_per_coded_sample;

    /**
     * prediction method (needed for huffyuv)
     * - encoding: Set by user.
     * - decoding: unused
     */
     int prediction_method;
#define FF_PRED_LEFT   0
#define FF_PRED_PLANE  1
#define FF_PRED_MEDIAN 2

    /**
     * sample aspect ratio (0 if unknown)
     * That is the width of a pixel divided by the height of the pixel.
     * Numerator and denominator must be relatively prime and smaller than 256 for some video standards.
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
    AVRational sample_aspect_ratio;

    /* ffdshow custom code
     * Regarding MPEG-2, currently there are two ways of encoding SAR.
     * Of course one is wrong. However, considerable number of videos are encoded in a wrong way.
     * We set the spec compliant value in sample_aspect_ratio and
     * wrong spec-value in sample_aspect_ratio2.
     */
    AVRational sample_aspect_ratio2;

    /**
     * the picture in the bitstream
     * - encoding: Set by libavcodec.
     * - decoding: Set by libavcodec.
     */
    AVFrame2 *coded_frame;

    /**
     * debug
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int debug;
#define FF_DEBUG_PICT_INFO   1
#define FF_DEBUG_RC          2
#define FF_DEBUG_BITSTREAM   4
#define FF_DEBUG_MB_TYPE     8
#define FF_DEBUG_QP          16
#define FF_DEBUG_MV          32
#define FF_DEBUG_DCT_COEFF   0x00000040
#define FF_DEBUG_SKIP        0x00000080
#define FF_DEBUG_STARTCODE   0x00000100
#define FF_DEBUG_PTS         0x00000200
#define FF_DEBUG_ER          0x00000400
#define FF_DEBUG_MMCO        0x00000800
#define FF_DEBUG_BUGS        0x00001000
#define FF_DEBUG_VIS_QP      0x00002000
#define FF_DEBUG_VIS_MB_TYPE 0x00004000
#define FF_DEBUG_BUFFERS     0x00008000
#define FF_DEBUG_THREADS     0x00010000

    /**
     * debug
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int debug_mv;
#define FF_DEBUG_VIS_MV_P_FOR  0x00000001 //visualize forward predicted MVs of P frames
#define FF_DEBUG_VIS_MV_B_FOR  0x00000002 //visualize forward predicted MVs of B frames
#define FF_DEBUG_VIS_MV_B_BACK 0x00000004 //visualize backward predicted MVs of B frames

    /**
     * error
     * - encoding: Set by libavcodec if flags&CODEC_FLAG_PSNR.
     * - decoding: unused
     */
    uint64_t error[4];

    /**
     * minimum MB quantizer
     * - encoding: unused
     * - decoding: unused
     */
    int mb_qmin;

    /**
     * maximum MB quantizer
     * - encoding: unused
     * - decoding: unused
     */
    int mb_qmax;

    /**
     * motion estimation comparison function
     * - encoding: Set by user.
     * - decoding: unused
     */
    int me_cmp;
    /**
     * subpixel motion estimation comparison function
     * - encoding: Set by user.
     * - decoding: unused
     */
    int me_sub_cmp;
    /**
     * macroblock comparison function (not supported yet)
     * - encoding: Set by user.
     * - decoding: unused
     */
    int mb_cmp;
    /**
     * interlaced DCT comparison function
     * - encoding: Set by user.
     * - decoding: unused
     */
    int ildct_cmp;
#define FF_CMP_SAD    0
#define FF_CMP_SSE    1
#define FF_CMP_SATD   2
#define FF_CMP_DCT    3
#define FF_CMP_PSNR   4
#define FF_CMP_BIT    5
#define FF_CMP_RD     6
#define FF_CMP_ZERO   7
#define FF_CMP_VSAD   8
#define FF_CMP_VSSE   9
#define FF_CMP_NSSE   10
#define FF_CMP_W53    11
#define FF_CMP_W97    12
#define FF_CMP_DCTMAX 13
#define FF_CMP_DCT264 14
#define FF_CMP_CHROMA 256

    /**
     * ME diamond size & shape
     * - encoding: Set by user.
     * - decoding: unused
     */
    int dia_size;

    /**
     * amount of previous MV predictors (2a+1 x 2a+1 square)
     * - encoding: Set by user.
     * - decoding: unused
     */
    int last_predictor_count;

    /**
     * prepass for motion estimation
     * - encoding: Set by user.
     * - decoding: unused
     */
    int pre_me;

    /**
     * motion estimation prepass comparison function
     * - encoding: Set by user.
     * - decoding: unused
     */
    int me_pre_cmp;

    /**
     * ME prepass diamond size & shape
     * - encoding: Set by user.
     * - decoding: unused
     */
    int pre_dia_size;

    /**
     * subpel ME quality
     * - encoding: Set by user.
     * - decoding: unused
     */
    int me_subpel_quality;

    /**
     * callback to negotiate the pixelFormat
     * @param fmt is the list of formats which are supported by the codec,
     * it is terminated by -1 as 0 is a valid format, the formats are ordered by quality.
     * The first is always the native one.
     * @return the chosen format
     * - encoding: unused
     * - decoding: Set by user, if not set the native format will be chosen.
     */
    enum PixelFormat (*get_format)(struct AVCodecContext2 *s, const enum PixelFormat * fmt);

    /**
     * DTG active format information (additional aspect ratio
     * information only used in DVB MPEG-2 transport streams)
     * 0 if not set.
     *
     * - encoding: unused
     * - decoding: Set by decoder.
     */
    int dtg_active_format;
#define FF_DTG_AFD_SAME         8
#define FF_DTG_AFD_4_3          9
#define FF_DTG_AFD_16_9         10
#define FF_DTG_AFD_14_9         11
#define FF_DTG_AFD_4_3_SP_14_9  13
#define FF_DTG_AFD_16_9_SP_14_9 14
#define FF_DTG_AFD_SP_4_3       15

    /**
     * maximum motion estimation search range in subpel units
     * If 0 then no limit.
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
    int me_range;

    /**
     * intra quantizer bias
     * - encoding: Set by user.
     * - decoding: unused
     */
    int intra_quant_bias;
#define FF_DEFAULT_QUANT_BIAS 999999

    /**
     * inter quantizer bias
     * - encoding: Set by user.
     * - decoding: unused
     */
    int inter_quant_bias;

    /**
     * color table ID
     * - encoding: unused
     * - decoding: Which clrtable should be used for 8bit RGB images.
     *             Tables have to be stored somewhere. FIXME
     */
    int color_table_id;

    /**
     * internal_buffer count
     * Don't touch, used by libavcodec default_get_buffer().
     */
    int internal_buffer_count;

    /**
     * internal_buffers
     * Don't touch, used by libavcodec default_get_buffer().
     */
    void *internal_buffer;

    /**
     * Global quality for codecs which cannot change it per frame.
     * This should be proportional to MPEG-1/2/4 qscale.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int global_quality;

#define FF_CODER_TYPE_VLC       0
#define FF_CODER_TYPE_AC        1
#define FF_CODER_TYPE_RAW       2
#define FF_CODER_TYPE_RLE       3
#define FF_CODER_TYPE_DEFLATE   4
    /**
     * coder type
     * - encoding: Set by user.
     * - decoding: unused
     */
    int coder_type;

    /**
     * context model
     * - encoding: Set by user.
     * - decoding: unused
     */
    int context_model;
#if 0
    /**
     *
     * - encoding: unused
     * - decoding: Set by user.
     */
    uint8_t * (*realloc)(struct AVCodecContext2 *s, uint8_t *buf, int buf_size);
#endif

    /**
     * slice flags
     * - encoding: unused
     * - decoding: Set by user.
     */
    int slice_flags;
#define SLICE_FLAG_CODED_ORDER    0x0001 ///< draw_horiz_band() is called in coded order instead of display
#define SLICE_FLAG_ALLOW_FIELD    0x0002 ///< allow draw_horiz_band() with field slices (MPEG2 field pics)
#define SLICE_FLAG_ALLOW_PLANE    0x0004 ///< allow draw_horiz_band() with 1 component at a time (SVQ1)

    /**
     * macroblock decision mode
     * - encoding: Set by user.
     * - decoding: unused
     */
    int mb_decision;
#define FF_MB_DECISION_SIMPLE 0        ///< uses mb_cmp
#define FF_MB_DECISION_BITS   1        ///< chooses the one which needs the fewest bits
#define FF_MB_DECISION_RD     2        ///< rate distortion

    /**
     * custom intra quantization matrix
     * - encoding: Set by user, can be NULL.
     * - decoding: Set by libavcodec.
     */
    uint16_t *intra_matrix;

    /**
     * custom inter quantization matrix
     * - encoding: Set by user, can be NULL.
     * - decoding: Set by libavcodec.
     */
    uint16_t *inter_matrix;

    /**
     * fourcc from the AVI stream header (LSB first, so "ABCD" -> ('D'<<24) + ('C'<<16) + ('B'<<8) + 'A').
     * This is used to work around some encoder bugs.
     * - encoding: unused
     * - decoding: Set by user, will be converted to uppercase by libavcodec during init.
     */
    unsigned int stream_codec_tag;

    /**
     * scene change detection threshold
     * 0 is default, larger means fewer detected scene changes.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int scenechange_threshold;

    /**
     * minimum Lagrange multipler
     * - encoding: Set by user.
     * - decoding: unused
     */
    int lmin;

    /**
     * maximum Lagrange multipler
     * - encoding: Set by user.
     * - decoding: unused
     */
    int lmax;

#if FF_API_PALETTE_CONTROL
    /**
     * palette control structure
     * - encoding: ??? (no palette-enabled encoder yet)
     * - decoding: Set by user.
     */
    struct AVPaletteControl *palctrl;
#endif

    /**
     * noise reduction strength
     * - encoding: Set by user.
     * - decoding: unused
     */
    int noise_reduction;

    /**
     * Called at the beginning of a frame to get cr buffer for it.
     * Buffer type (size, hints) must be the same. libavcodec won't check it.
     * libavcodec will pass previous buffer in pic, function should return
     * same buffer or new buffer with old frame "painted" into it.
     * If pic.data[0] == NULL must behave like get_buffer().
     * if CODEC_CAP_DR1 is not set then reget_buffer() must call
     * avcodec_default_reget_buffer() instead of providing buffers allocated by
     * some other means.
     * - encoding: unused
     * - decoding: Set by libavcodec, user can override.
     */
    int (*reget_buffer)(struct AVCodecContext2 *c, AVFrame2 *pic);

    /**
     * Number of bits which should be loaded into the rc buffer before decoding starts.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int rc_initial_buffer_occupancy;

    /**
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
    int inter_threshold;

    /**
     * CODEC_FLAG2_*
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int flags2;

    /**
     * Simulates errors in the bitstream to test error concealment.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int error_rate;

    /**
     * MP3 antialias algorithm, see FF_AA_* below.
     * - encoding: unused
     * - decoding: Set by user.
     */
    int antialias_algo;
#define FF_AA_AUTO    0
#define FF_AA_FASTINT 1 //not implemented yet
#define FF_AA_INT     2
#define FF_AA_FLOAT   3
    /**
     * quantizer noise shaping
     * - encoding: Set by user.
     * - decoding: unused
     */
    int quantizer_noise_shaping;

    /**
     * thread count
     * is used to decide how many independent tasks should be passed to execute()
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int thread_count;

    /**
     * The codec may call this to execute several independent things.
     * It will return only after finishing all tasks.
     * The user may replace this with some multithreaded implementation,
     * the default implementation will execute the parts serially.
     * @param count the number of things to execute
     * - encoding: Set by libavcodec, user can override.
     * - decoding: Set by libavcodec, user can override.
     */
    int (*execute)(struct AVCodecContext2 *c, int (*func)(struct AVCodecContext2 *c2, void *arg), void **arg2, int *ret, int count);

    /**
     * thread opaque
     * Can be used by execute() to store some per AVCodecContext2 stuff.
     * - encoding: set by execute()
     * - decoding: set by execute()
     */
    void *thread_opaque;

    /**
     * Motion estimation threshold below which no motion estimation is
     * performed, but instead the user specified motion vectors are used.
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
     int me_threshold;

    /**
     * Macroblock threshold below which the user specified macroblock types will be used.
     * - encoding: Set by user.
     * - decoding: unused
     */
     int mb_threshold;

    /**
     * precision of the intra DC coefficient - 8
     * - encoding: Set by user.
     * - decoding: unused
     */
     int intra_dc_precision;

    /**
     * noise vs. sse weight for the nsse comparsion function
     * - encoding: Set by user.
     * - decoding: unused
     */
     int nsse_weight;

    /**
     * Number of macroblock rows at the top which are skipped.
     * - encoding: unused
     * - decoding: Set by user.
     */
     int skip_top;

    /**
     * Number of macroblock rows at the bottom which are skipped.
     * - encoding: unused
     * - decoding: Set by user.
     */
     int skip_bottom;

    /**
     * profile
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
     int profile;
#define FF_PROFILE_UNKNOWN -99

#define FF_PROFILE_AAC_MAIN 0
#define FF_PROFILE_AAC_LOW  1
#define FF_PROFILE_AAC_SSR  2
#define FF_PROFILE_AAC_LTP  3

#define FF_PROFILE_H264_BASELINE    66
#define FF_PROFILE_H264_MAIN        77
#define FF_PROFILE_H264_EXTENDED    88
#define FF_PROFILE_H264_HIGH        100
#define FF_PROFILE_H264_HIGH_10     110
#define FF_PROFILE_H264_HIGH_422    122
#define FF_PROFILE_H264_HIGH_444    244
#define FF_PROFILE_H264_CAVLC_444   44

    /**
     * level
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
     int level;
#define FF_LEVEL_UNKNOWN -99

    /**
     * low resolution decoding, 1-> 1/2 size, 2->1/4 size
     * - encoding: unused
     * - decoding: Set by user.
     */
     int lowres;

    /**
     * Bitstream width / height, may be different from width/height if lowres
     * or other things are used.
     * - encoding: unused
     * - decoding: Set by user before init if known. Codec should override / dynamically change if needed.
     */
    int coded_width, coded_height;

    /**
     * frame skip threshold
     * - encoding: Set by user.
     * - decoding: unused
     */
    int frame_skip_threshold;

    /**
     * frame skip factor
     * - encoding: Set by user.
     * - decoding: unused
     */
    int frame_skip_factor;

    /**
     * frame skip exponent
     * - encoding: Set by user.
     * - decoding: unused
     */
    int frame_skip_exp;

    /**
     * frame skip comparison function
     * - encoding: Set by user.
     * - decoding: unused
     */
    int frame_skip_cmp;

    /**
     * Border processing masking, raises the quantizer for mbs on the borders
     * of the picture.
     * - encoding: Set by user.
     * - decoding: unused
     */
    float border_masking;

    /**
     * minimum MB lagrange multipler
     * - encoding: Set by user.
     * - decoding: unused
     */
    int mb_lmin;

    /**
     * maximum MB lagrange multipler
     * - encoding: Set by user.
     * - decoding: unused
     */
    int mb_lmax;

    /**
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
    int me_penalty_compensation;

    /**
     *
     * - encoding: unused
     * - decoding: Set by user.
     */
    enum AVDiscard skip_loop_filter;

    /**
     *
     * - encoding: unused
     * - decoding: Set by user.
     */
    enum AVDiscard skip_idct;

    /**
     *
     * - encoding: unused
     * - decoding: Set by user.
     */
    enum AVDiscard skip_frame;

    /**
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
    int bidir_refine;

    /**
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
    int brd_scale;

    /**
     * constant rate factor - quality-based VBR - values ~correspond to qps
     * - encoding: Set by user.
     * - decoding: unused
     */
    float crf;

    /**
     * constant quantization parameter rate control method
     * - encoding: Set by user.
     * - decoding: unused
     */
    int cqp;

    /**
     * minimum GOP size
     * - encoding: Set by user.
     * - decoding: unused
     */
    int keyint_min;

    /**
     * number of reference frames
     * - encoding: Set by user.
     * - decoding: Set by lavc.
     */
    int refs;

    /**
     * chroma qp offset from luma
     * - encoding: Set by user.
     * - decoding: unused
     */
    int chromaoffset;

    /**
     * Influences how often B-frames are used.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int bframebias;

    /**
     * trellis RD quantization
     * - encoding: Set by user.
     * - decoding: unused
     */
    int trellis;

    /**
     * Reduce fluctuations in qp (before curve compression).
     * - encoding: Set by user.
     * - decoding: unused
     */
    float complexityblur;

    /**
     * in-loop deblocking filter alphac0 parameter
     * alpha is in the range -6...6
     * - encoding: Set by user.
     * - decoding: unused
     */
    int deblockalpha;

    /**
     * in-loop deblocking filter beta parameter
     * beta is in the range -6...6
     * - encoding: Set by user.
     * - decoding: unused
     */
    int deblockbeta;

    /**
     * macroblock subpartition sizes to consider - p8x8, p4x4, b8x8, i8x8, i4x4
     * - encoding: Set by user.
     * - decoding: unused
     */
    int partitions;
#define X264_PART_I4X4 0x001  /* Analyze i4x4 */
#define X264_PART_I8X8 0x002  /* Analyze i8x8 (requires 8x8 transform) */
#define X264_PART_P8X8 0x010  /* Analyze p16x8, p8x16 and p8x8 */
#define X264_PART_P4X4 0x020  /* Analyze p8x4, p4x8, p4x4 */
#define X264_PART_B8X8 0x100  /* Analyze b16x8, b8x16 and b8x8 */

    /**
     * direct MV prediction mode - 0 (none), 1 (spatial), 2 (temporal), 3 (auto)
     * - encoding: Set by user.
     * - decoding: unused
     */
    int directpred;

    /**
     * Audio cutoff bandwidth (0 means "automatic")
     * - encoding: Set by user.
     * - decoding: unused
     */
    int cutoff;

    /**
     * Multiplied by qscale for each frame and added to scene_change_score.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int scenechange_factor;

    /**
     *
     * Note: Value depends upon the compare function used for fullpel ME.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int mv0_threshold;

    /**
     * Adjusts sensitivity of b_frame_strategy 1.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int b_sensitivity;

    /**
     * - encoding: Set by user.
     * - decoding: unused
     */
    int compression_level;
#define FF_COMPRESSION_DEFAULT -1

#if FF_API_USE_LPC
    /**
     * Sets whether to use LPC mode - used by FLAC encoder.
     * - encoding: Set by user.
     * - decoding: unused
     * @deprecated Deprecated in favor of lpc_type and lpc_passes.
     */
    int use_lpc;
#endif

    /**
     * LPC coefficient precision - used by FLAC encoder
     * - encoding: Set by user.
     * - decoding: unused
     */
    int lpc_coeff_precision;

    /**
     * - encoding: Set by user.
     * - decoding: unused
     */
    int min_prediction_order;

    /**
     * - encoding: Set by user.
     * - decoding: unused
     */
    int max_prediction_order;

    /**
     * search method for selecting prediction order
     * - encoding: Set by user.
     * - decoding: unused
     */
    int prediction_order_method;

    /**
     * - encoding: Set by user.
     * - decoding: unused
     */
    int min_partition_order;

    /**
     * - encoding: Set by user.
     * - decoding: unused
     */
    int max_partition_order;

    /**
     * GOP timecode frame start number, in non drop frame format
     * - encoding: Set by user.
     * - decoding: unused
     */
    int64_t timecode_frame_start;

#if LIBAVCODEC_VERSION_MAJOR < 53
    /**
     * Decoder should decode to this many channels if it can (0 for default)
     * - encoding: unused
     * - decoding: Set by user.
     * @deprecated Deprecated in favor of request_channel_layout.
     */
    int request_channels;
#endif

    /**
     * Percentage of dynamic range compression to be applied by the decoder.
     * The default value is 1.0, corresponding to full compression.
     * - encoding: unused
     * - decoding: Set by user.
     */
    float drc_scale;

    /**
     * opaque 64bit number (generally a PTS) that will be reordered and
     * output in AVFrame2.reordered_opaque
     * - encoding: unused
     * - decoding: Set by user.
     */
    int64_t reordered_opaque;
    int64_t reordered_opaque2; /* ffdshow custom code */
    int64_t reordered_opaque3; /* ffdshow custom code */

    /**
     * Bits per sample/pixel of internal libavcodec pixel/sample format.
     * This field is applicable only when sample_fmt is SAMPLE_FMT_S32.
     * - encoding: set by user.
     * - decoding: set by libavcodec.
     */
    int bits_per_raw_sample;

    /**
     * Audio channel layout.
     * - encoding: set by user.
     * - decoding: set by libavcodec.
     */
    int64_t channel_layout;

    /**
     * Request decoder to use this channel layout if it can (0 for default)
     * - encoding: unused
     * - decoding: Set by user.
     */
    int64_t request_channel_layout;

    /**
     * Ratecontrol attempt to use, at maximum, <value> of what can be used without an underflow.
     * - encoding: Set by user.
     * - decoding: unused.
     */
    float rc_max_available_vbv_use;

    /**
     * Ratecontrol attempt to use, at least, <value> times the amount needed to prevent a vbv overflow.
     * - encoding: Set by user.
     * - decoding: unused.
     */
    float rc_min_vbv_overflow_use;

    /**
     * For some codecs, the time base is closer to the field rate than the frame rate.
     * Most notably, H.264 and MPEG-2 specify time_base as half of frame duration
     * if no telecine is used ...
     *
     * Set to time_base ticks per frame. Default 1, e.g., H.264/MPEG-2 set it to 2.
     */
    int ticks_per_frame;

    /**
     * Chromaticity coordinates of the source primaries.
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum AVColorPrimaries color_primaries;

    /**
     * Color Transfer Characteristic.
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum AVColorTransferCharacteristic color_trc;

    /**
     * YUV colorspace type.
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum AVColorSpace colorspace;

    /**
     * MPEG vs JPEG YUV range.
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum AVColorRange color_range;

    /**
     * This defines the location of chroma samples.
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum AVChromaLocation chroma_sample_location;

    /**
     * The codec may call this to execute several independent things.
     * It will return only after finishing all tasks.
     * The user may replace this with some multithreaded implementation,
     * the default implementation will execute the parts serially.
     * Also see avcodec_thread_init and e.g. the --enable-pthread configure option.
     * @param c context passed also to func
     * @param count the number of things to execute
     * @param arg2 argument passed unchanged to func
     * @param ret return values of executed functions, must have space for "count" values. May be NULL.
     * @param func function that will be called count times, with jobnr from 0 to count-1.
     *             threadnr will be in the range 0 to c->thread_count-1 < MAX_THREADS and so that no
     *             two instances of func executing at the same time will have the same threadnr.
     * @return always 0 currently, but code should handle a future improvement where when any call to func
     *         returns < 0 no further calls to func may be done and < 0 is returned.
     * - encoding: Set by libavcodec, user can override.
     * - decoding: Set by libavcodec, user can override.
     */
    int (*execute2)(struct AVCodecContext2 *c, int (*func)(struct AVCodecContext2 *c2, void *arg, int jobnr, int threadnr), void *arg2, int *ret, int count);

    /**
     * explicit P-frame weighted prediction analysis method
     * 0: off
     * 1: fast blind weighting (one reference duplicate with -1 offset)
     * 2: smart weighting (full fade detection analysis)
     * - encoding: Set by user.
     * - decoding: unused
     */
    int weighted_p_pred;

    /**
     * AQ mode
     * 0: Disabled
     * 1: Variance AQ (complexity mask)
     * 2: Auto-variance AQ (experimental)
     * - encoding: Set by user
     * - decoding: unused
     */
    int aq_mode;

    /**
     * AQ strength
     * Reduces blocking and blurring in flat and textured areas.
     * - encoding: Set by user
     * - decoding: unused
     */
    float aq_strength;

    /**
     * PSY RD
     * Strength of psychovisual optimization
     * - encoding: Set by user
     * - decoding: unused
     */
    float psy_rd;

    /**
     * PSY trellis
     * Strength of psychovisual optimization
     * - encoding: Set by user
     * - decoding: unused
     */
    float psy_trellis;

    /**
     * RC lookahead
     * Number of frames for frametype and ratecontrol lookahead
     * - encoding: Set by user
     * - decoding: unused
     */
    int rc_lookahead;

    /**
     * Constant rate factor maximum
     * With CRF encoding mode and VBV restrictions enabled, prevents quality from being worse
     * than crf_max, even if doing so would violate VBV restrictions.
     * - encoding: Set by user.
     * - decoding: unused
     */
    float crf_max;

    int log_level_offset;

    /**
     * Determines which LPC analysis algorithm to use.
     * - encoding: Set by user
     * - decoding: unused
     */
    enum AVLPCType lpc_type;

    /**
     * Number of passes to use for Cholesky factorization during LPC analysis
     * - encoding: Set by user
     * - decoding: unused
     */
    int lpc_passes;

    /**
     * Number of slices.
     * Indicates number of picture subdivisions. Used for parallelized
     * decoding.
     * - encoding: Set by user
     * - decoding: unused
     */
    int slices;

    /* ffdshow custom stuff (begin) */
    /**
     * Whether this is a copy of the context which had init() called on it.
     * This is used by multithreading - shared tables and picture pointers
     * should be freed from the original context only.
     * - encoding: Set by libavcodec.
     * - decoding: Set by libavcodec.
     */
    int is_copy;

    /**
     * Which multithreading methods to use.
     * Use of FF_THREAD_FRAME will increase decoding delay by one frame per thread,
     * so clients which require strictly conforming DTS must not use it.
     *
     * - encoding: Set by user, otherwise the default is used.
     * - decoding: Set by user, otherwise the default is used.
     */
    int thread_type;
#define FF_THREAD_FRAME   1 //< Decode more than one frame at once
#define FF_THREAD_SLICE   2 //< Decode more than one part of a single frame at once

    /**
     * Which multithreading methods are actually active at the moment.
     * - encoding: Set by libavcodec.
     * - decoding: Set by libavcodec.
     */
    int active_thread_type;
       
    /**
     * minimum and maxminum quantizer for I frames. If 0, derived from qmin, i_quant_factor, i_quant_offset
     * - encoding: set by user.
     * - decoding: unused
     */
    int qmin_i,qmax_i;

    /**
     * minimum and maximum quantizer for B frames. If 0, derived from qmin, b_quant_factor, b_quant_offset
     * - encoding: set by user.
     * - decoding: unused
     */
    int qmin_b,qmax_b;
    
    float postgain;
    int ac3mode,ac3lfe;
    int ac3channels[6];
    int nal_length_size;
    int vorbis_header_size[3];
    int64_t granulepos;
    int64_t *parserRtStart;
    void (*handle_user_data)(struct AVCodecContext2 *c,const uint8_t *buf,int buf_size);
    int h264_has_to_drop_first_non_ref;    // Workaround Haali's media splitter (http://forum.doom9.org/showthread.php?p=1226434#post1226434)

     /**
     * Force 4:3 or 16:9 as DAR (MPEG-2 only)
     * - encoding: unused.
     * - decoding: unused in ffmpeg-mt.
     */
    int isDVD;

    /* ffdshow custom stuff (end) */
} AVCodecContext2;

/**
 * AVCodec2.
 */
typedef struct AVCodec2 {
    /**
     * Name of the codec implementation.
     * The name is globally unique among encoders and among decoders (but an
     * encoder and a decoder can share the same name).
     * This is the primary way to find a codec from the user perspective.
     */
    const char *name;
    enum AVMediaType type;
    enum CodecID id;
    int priv_data_size;
    int (*init)(AVCodecContext2 *);
    int (*encode)(AVCodecContext2 *, uint8_t *buf, int buf_size, void *data);
    int (*close)(AVCodecContext2 *);
    int (*decode)(AVCodecContext2 *, void *outdata, int *outdata_size, AVPacket2 *avpkt);
    /**
     * Codec capabilities.
     * see CODEC_CAP_*
     */
    int capabilities;
    struct AVCodec2 *next;
    /**
     * Flush buffers.
     * Will be called when seeking
     */
    void (*flush)(AVCodecContext2 *);
    const AVRational *supported_framerates; ///< array of supported framerates, or NULL if any, array is terminated by {0,0}
    const enum PixelFormat *pix_fmts;       ///< array of supported pixel formats, or NULL if unknown, array is terminated by -1
    /**
     * Descriptive name for the codec, meant to be more human readable than name.
     * You should use the NULL_IF_CONFIG_SMALL() macro to define it.
     */
    const char *long_name;
    const int *supported_samplerates;       ///< array of supported audio samplerates, or NULL if unknown, array is terminated by 0
    const enum SampleFormat *sample_fmts;   ///< array of supported sample formats, or NULL if unknown, array is terminated by -1
    const int64_t *channel_layouts;         ///< array of support channel layouts, or NULL if unknown. array is terminated by 0
    uint8_t max_lowres;                     ///< maximum value for lowres supported by the decoder

    /* ffmpeg-mt */
    /**
     * @defgroup framethreading Frame-level threading support functions.
     * @{
     */
    /**
     * If defined, called on thread contexts when they are created.
     * If the codec allocates writable tables in init(), re-allocate them here.
     * priv_data will be set to a copy of the original.
     */
    int (*init_thread_copy)(AVCodecContext2 *);
    /**
     * Copy necessary context variables from a previous thread context to the current one.
     * If not defined, the next thread will start automatically; otherwise, the codec
     * must call ff_thread_finish_setup().
     *
     * dst and src will (rarely) point to the same context, in which case memcpy should be skipped.
     */
    int (*update_thread_context)(AVCodecContext2 *dst, AVCodecContext2 *src);
    /** @} */
    
    /* this must be at the end of the struct */
    AVClass *priv_class;                    ///< AVClass for the private context
} AVCodec2;

/**
 * four components are given, that's all.
 * the last component is alpha
 */
typedef struct AVPicture2 {
    uint8_t *data[4];
    int linesize[4];       ///< number of bytes per line
} AVPicture2;

#endif /* AVCODEC_AVCODEC_H */
