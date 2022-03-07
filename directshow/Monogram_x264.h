//-----------------------------------------------------------------------------
//
//	Monogram x264 Wrapper
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
 
// {1FB0F046-623C-40a7-B439-41E4BFCB8BAB}
static const GUID CLSID_MonogramX264 = 
{ 0x1fb0f046, 0x623c, 0x40a7, { 0xb4, 0x39, 0x41, 0xe4, 0xbf, 0xcb, 0x8b, 0xab } };
 
// {83B98647-D538-4d92-8BBD-513FCFE010D3}
static const GUID CLSID_MonogramX264Page = 
{ 0x83b98647, 0xd538, 0x4d92, { 0x8b, 0xbd, 0x51, 0x3f, 0xcf, 0xe0, 0x10, 0xd3 } };
 
// {31435641-0000-0010-8000-00AA00389B71}
static const GUID MEDIASUBTYPE_AVC =
{ 0x31435641, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
 
enum {
	MONO_AVC_LEVEL_1			= 0,
	MONO_AVC_LEVEL_1B			= 1,
	MONO_AVC_LEVEL_1_1			= 2,
	MONO_AVC_LEVEL_1_2			= 3,
	MONO_AVC_LEVEL_1_3			= 4,
	MONO_AVC_LEVEL_2			= 5,
	MONO_AVC_LEVEL_2_1			= 6,
	MONO_AVC_LEVEL_2_2			= 7,
	MONO_AVC_LEVEL_3			= 8,
	MONO_AVC_LEVEL_3_1			= 9,
	MONO_AVC_LEVEL_3_2			= 10,
	MONO_AVC_LEVEL_4			= 11,
	MONO_AVC_LEVEL_4_1			= 12,
	MONO_AVC_LEVEL_4_2			= 13,
	MONO_AVC_LEVEL_5			= 14,
	MONO_AVC_LEVEL_5_1			= 15
};
 
enum {
	MONO_AVC_RC_MODE_ABR		= 0,
	MONO_AVC_RC_MODE_CQP		= 1,
	MONO_AVC_RC_MODE_CRF		= 2
};
 
enum {
	MONO_AVC_INTRA_LOW			= 0,
	MONO_AVC_INTRA_HIGH			= 1
};
 
enum {
	MONO_AVC_INTER_LOW			= 0,
	MONO_AVC_INTER_MID			= 1,
	MONO_AVC_INTER_HIGH			= 2
};
 
enum {
	MONO_AVC_SUBPEL_NO			= 0,
	MONO_AVC_SUBPEL_HALF_LO		= 1,
	MONO_AVC_SUBPEL_HALF_HI		= 2,
	MONO_AVC_SUBPEL_QUART_LO	= 3,
	MONO_AVC_SUBPEL_QUART_HI	= 4
};
 
enum {
	MONO_AVC_TRELLIS_DISABLED	= 0,
	MONO_AVC_TRELLIS_MACROBLOCK	= 1,
	MONO_AVC_TRELLIS_ALLMODES	= 2
};
 
enum {
	MONO_AVC_DIRECTMV_NONE		= 0,
	MONO_AVC_DIRECTMV_AUTO		= 1,
	MONO_AVC_DIRECTMV_SPATIAL	= 2,
	MONO_AVC_DIRECTMV_TEMPORAL	= 3
};
 
enum {
	MONO_AVC_WEIGHTED_DISABLED	= 0,
	MONO_AVC_WEIGHTED_PFRAMES	= 1,
	MONO_AVC_WEIGHTED_PANDBFRAMES = 2
};
 
enum {
	MONO_AVC_BFRAMESADAPTIVE_DISABLED = 0,
	MONO_AVC_BFRAMESADAPTIVE_FAST = 1,
	MONO_AVC_BFRAMESADAPTIVE_OPTIMAL = 2
};
 
enum {
	MONO_AVC_BPYRAMID_NONE		= 0,
	MONO_AVC_BPYRAMID_STRICT	= 1,
	MONO_AVC_BPYRAMID_NORMAL	= 2
};
 
struct MONOGRAM_X264_VIDEOPROPS
{
	int		width;
	int		height;
	int		fps_num;		
	int		fps_den;		
};
 
struct MONOGRAM_X264_CONFIG2
{
	int		level;
 
	bool	parallel;
	bool	sliced_threading;
	bool	cabac;
	bool	interlaced;
	bool	deblocking;
	int		deblock_strength;
	bool	au_delimiters;
	bool	annexb;
 
	int		reference_frames;
	int		key_int_min;
	int		key_int_max;
	int		bframes;
	bool	periodic_intra_refresh;
 
	int		adaptive_b_frames;
	int		bframes_pyramid;
 
 
	int		rcmethod;
	int		bitrate_kbps;			// bitrate in kbps
	int		tolerance_percent;		// <0; 100>
	int		vbv_maxrate_kbps;
	int		vbv_buffer_size;
	int		qp_min;
	int		qp_max;
	int		const_qp;
 
	int		partitions_intra;
	int		partitions_inter;
	int		subpixel_refine;
 
	bool	mixed_references;
	int		weighted_prediction;
	int		direct_mv;
	int		trellis;
};
 
/*
	I've removed the old interface because the config structure had to be
	changed significantly which would break backward compatibility.
	This way the outdated software would not be able to configure the
	filter using the "old" structure.
*/
 
[uuid("56263DF8-1782-4916-806B-56D0A2FB55F0")]
struct IMonogramX264_2 : public IUnknown
{
	STDMETHOD(SetBitrate)(int bitrate_kbps) PURE;
	STDMETHOD(GetDefaults)(MONOGRAM_X264_CONFIG2 *config) PURE;
	STDMETHOD(GetConfig)(MONOGRAM_X264_CONFIG2 *config) PURE;
	STDMETHOD(SetConfig)(MONOGRAM_X264_CONFIG2 *config) PURE;
	STDMETHOD(GetVideoProps)(MONOGRAM_X264_VIDEOPROPS *props) PURE;
};
