#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mp4muxer.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif
#ifdef  LITTLE_ENDIAN
#define ntohl(a) ((((a) & 0xFF) << 24) | ((((a) >> 8) & 0xFF) << 16) | ((((a) >> 16) & 0xFF) << 8) | ((((a) >> 24) & 0xFF) << 0))
#define htonl(a) ((((a) & 0xFF) << 24) | ((((a) >> 8) & 0xFF) << 16) | ((((a) >> 16) & 0xFF) << 8) | ((((a) >> 24) & 0xFF) << 0))
#else
#define ntohl(a) (a)
#define htonl(a) (a)
#endif

#ifndef offsetof
#define offsetof(type, member) ((size_t)&((type*)0)->member)
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#define MP4_FOURCC(a, b, c, d)  (((a) << 0) | ((b) << 8) | ((c) << 16) | ((d) << 24))

#define ENABLE_RECALCULATE_DURATION     1
#define VIDEO_TIMESCALE_BY_FRAME_RATE   1
#define AUDIO_TIMESCALE_BY_SAMPLE_RATE  1

#pragma pack(1)
typedef struct {
    uint32_t  ftyp_size;
    uint32_t  ftyp_type;
    uint32_t  ftyp_brand;
    uint32_t  ftyp_version;
    uint32_t  ftyp_compat1;
    uint32_t  ftyp_compat2;
    uint32_t  ftyp_compat3;

    uint32_t  moov_size;
    uint32_t  moov_type;

    uint32_t  mvhd_size;
    uint32_t  mvhd_type;
    uint8_t   mvhd_version;
    uint8_t   mvhd_flags[3];
    uint32_t  mvhd_create_time;
    uint32_t  mvhd_modify_time;
    uint32_t  mvhd_timescale;
    uint32_t  mvhd_duration;
    uint32_t  mvhd_playrate;
    uint16_t  mvhd_volume;
    uint8_t   mvhd_reserved[10];
    uint32_t  mvhd_matrix[9];
    uint8_t   mvhd_predefined[24];
    uint32_t  mvhd_next_trackid;

    // video track
    uint32_t  trakv_size;
    uint32_t  trakv_type;

    uint32_t  tkhdv_size;
    uint32_t  tkhdv_type;
    uint8_t   tkhdv_version;
    uint8_t   tkhdv_flags[3];
    uint32_t  tkhdv_create_time;
    uint32_t  tkhdV_modify_time;
    uint32_t  tkhdv_trackid;
    uint32_t  tkhdv_reserved1;
    uint32_t  tkhdv_duration;
    uint32_t  tkhdv_reserved2;
    uint32_t  tkhdv_reserved3;
    uint16_t  tkhdv_layer;
    uint16_t  tkhdv_alternate_group;
    uint16_t  tkhdv_volume;
    uint16_t  tkhdv_reserved4;
    uint32_t  tkhdv_matrix[9];
    uint32_t  tkhdv_width;
    uint32_t  tkhdv_height;

    uint32_t  mdiav_size;
    uint32_t  mdiav_type;

    uint32_t  mdhdv_size;
    uint32_t  mdhdv_type;
    uint8_t   mdhdv_version;
    uint8_t   mdhdv_flags[3];
    uint32_t  mdhdv_create_time;
    uint32_t  mdhdv_modify_time;
    uint32_t  mdhdv_timescale;
    uint32_t  mdhdv_duration;
    uint16_t  mdhdv_language;
    uint16_t  mdhdv_predefined;

    uint32_t  hdlrv_size;
    uint32_t  hdlrv_type;
    uint8_t   hdlrv_version;
    uint8_t   hdlrv_flags[3];
    uint8_t   hdlrv_predefined[4];
    uint32_t  hdlrv_handler_type;
    uint8_t   hdlrv_reserved[12];
    uint8_t   hdlrv_name[16];

    uint32_t  minfv_size;
    uint32_t  minfv_type;

    uint32_t  vmhdv_size;
    uint32_t  vmhdv_type;
    uint8_t   vmhdv_version;
    uint8_t   vmhdv_flags[3];
    uint32_t  vmhdv_graph_mode;
    uint32_t  vmhdv_opcolor;

    uint32_t  dinfv_size;
    uint32_t  dinfv_type;

    uint32_t  drefv_size;
    uint32_t  drefv_type;
    uint8_t   drefv_version;
    uint8_t   drefv_flags[3];
    uint32_t  drefv_entry_count;

    uint32_t  urlv_size;
    uint32_t  urlv_type;
    uint8_t   urlv_version;
    uint8_t   urlv_flags[3];

    uint32_t  stblv_size;
    uint32_t  stblv_type;

    uint32_t  stsdv_size;
    uint32_t  stsdv_type;
    uint8_t   stsdv_version;
    uint8_t   stsdv_flags[3];
    uint32_t  stsdv_entry_count;

    #define STSDV_RESERVED_SIZE 256
    uint32_t  stsdv_ahvc1_size;  // ahvc1 <- avc1 & hvc1
    uint32_t  stsdv_ahvc1_type;
    uint8_t   stsdv_ahvc1_reserved1[6];
    uint16_t  stsdv_ahvc1_data_refidx;
    uint16_t  stsdv_ahvc1_predefined1;
    uint16_t  stsdv_ahvc1_reserved2;
    uint32_t  stsdv_ahvc1_predefined2[3];
    uint16_t  stsdv_ahvc1_width;
    uint16_t  stsdv_ahvc1_height;
    uint32_t  stsdv_ahvc1_horiz_res;
    uint32_t  stsdv_ahvc1_vert_res;
    uint32_t  stsdv_ahvc1_reserved3;
    uint16_t  stsdv_ahvc1_frame_count;
    uint8_t   stsdv_ahvc1_compressor[32];
    uint16_t  stsdv_ahvc1_depth;
    uint16_t  stsdv_ahvc1_predefined;
    uint8_t   stsdv_ahvcc_reserved[STSDV_RESERVED_SIZE];

    uint32_t  sttsv_size;
    uint32_t  sttsv_type;
    uint8_t   sttsv_version;
    uint8_t   sttsv_flags[3];
    uint32_t  sttsv_count;

    uint32_t  stssv_size;
    uint32_t  stssv_type;
    uint8_t   stssv_version;
    uint8_t   stssv_flags[3];
    uint32_t  stssv_count;

    uint32_t  stscv_size;
    uint32_t  stscv_type;
    uint8_t   stscv_version;
    uint8_t   stscv_flags[3];
    uint32_t  stscv_count;
    uint32_t  stscv_first_chunk;
    uint32_t  stscv_samp_per_chunk;
    uint32_t  stscv_samp_desc_id;

    uint32_t  stszv_size;
    uint32_t  stszv_type;
    uint8_t   stszv_version;
    uint8_t   stszv_flags[3];
    uint32_t  stszv_sample_size;
    uint32_t  stszv_count;

    uint32_t  stcov_size;
    uint32_t  stcov_type;
    uint8_t   stcov_version;
    uint8_t   stcov_flags[3];
    uint32_t  stcov_count;

    // audio track
    uint32_t  traka_size;
    uint32_t  traka_type;

    uint32_t  tkhda_size;
    uint32_t  tkhda_type;
    uint8_t   tkhda_version;
    uint8_t   tkhda_flags[3];
    uint32_t  tkhda_create_time;
    uint32_t  tkhda_modify_time;
    uint32_t  tkhda_trackid;
    uint32_t  tkhda_reserved1;
    uint32_t  tkhda_duration;
    uint32_t  tkhda_reserved2;
    uint32_t  tkhda_reserved3;
    uint16_t  tkhda_layer;
    uint16_t  tkhda_alternate_group;
    uint16_t  tkhda_volume;
    uint16_t  tkhda_reserved4;
    uint32_t  tkhda_matrix[9];
    uint32_t  tkhda_width;
    uint32_t  tkhda_height;

    uint32_t  mdiaa_size;
    uint32_t  mdiaa_type;

    uint32_t  mdhda_size;
    uint32_t  mdhda_type;
    uint8_t   mdhda_version;
    uint8_t   mdhda_flags[3];
    uint32_t  mdhda_create_time;
    uint32_t  mdhda_modify_time;
    uint32_t  mdhda_timescale;
    uint32_t  mdhda_duration;
    uint16_t  mdhda_language;
    uint16_t  mdhda_predefined;

    uint32_t  hdlra_size;
    uint32_t  hdlra_type;
    uint8_t   hdlra_version;
    uint8_t   hdlra_flags[3];
    uint8_t   hdlra_predefined[4];
    uint32_t  hdlra_handler_type;
    uint8_t   hdlra_reserved[12];
    uint8_t   hdlra_name[16];

    uint32_t  minfa_size;
    uint32_t  minfa_type;

    uint32_t  smhda_size;
    uint32_t  smhda_type;
    uint8_t   smhda_version;
    uint8_t   smhda_flags[3];
    uint16_t  smhda_balance;
    uint16_t  smhda_reserved;

    uint32_t  dinfa_size;
    uint32_t  dinfa_type;

    uint32_t  drefa_size;
    uint32_t  drefa_type;
    uint8_t   drefa_version;
    uint8_t   drefa_flags[3];
    uint32_t  drefa_entry_count;

    uint32_t  urla_size;
    uint32_t  urla_type;
    uint8_t   urla_version;
    uint8_t   urla_flags[3];

    uint32_t  stbla_size;
    uint32_t  stbla_type;

    uint32_t  stsda_size;
    uint32_t  stsda_type;
    uint8_t   stsda_version;
    uint8_t   stsda_flags[3];
    uint32_t  stsda_entry_count;

    uint32_t  mp4a_size;
    uint32_t  mp4a_type;
    uint8_t   mp4a_reserved1[6];
    uint16_t  mp4a_data_refidx;
    uint32_t  mp4a_reserved2[2];
    uint16_t  mp4a_channel_num;
    uint16_t  mp4a_sample_size;
    uint16_t  mp4a_predefined1;
    uint16_t  mp4a_reserved3;
    uint32_t  mp4a_sample_rate;

    uint32_t  esds_size;
    uint32_t  esds_type;
    uint8_t   esds_version;
    uint8_t   esds_flags[3];
    uint8_t   esds_esdesc_tag;    // 0x03
    uint16_t  esds_esdesc_len;    // ((25 << 8) | 0x80)
    uint16_t  esds_esdesc_id;     // 0x0200
    uint8_t   esds_esdesc_flags;  // 0x00
    uint8_t   esds_deccfg_tag;    // 0x04
    uint8_t   esds_deccfg_len;    // 17
    uint8_t   esds_deccfg_object; // 0x40 - aac, 0x6B - mp3
    uint8_t   esds_deccfg_stream; // 0x15
    uint8_t   esds_deccfg_buffer_size[3];
    uint32_t  esds_deccfg_max_bitrate;
    uint32_t  esds_deccfg_avg_bitrate;
    //++ if esds_deccfg_object == aac
    uint8_t   esds_decspec_tag;   // 0x05
    uint8_t   esds_decspec_len;   // 2
    uint16_t  esds_decspec_info;
    //-- if esds_deccfg_object == aac
    uint8_t   esds_slcfg_tag;     // 0x06
    uint8_t   esds_slcfg_len;     // 1
    uint8_t   esds_slcfg_reserved;// 0x02

    uint32_t  sttsa_size;
    uint32_t  sttsa_type;
    uint8_t   sttsa_version;
    uint8_t   sttsa_flags[3];
    uint32_t  sttsa_count;

    uint32_t  stsca_size;
    uint32_t  stsca_type;
    uint8_t   stsca_version;
    uint8_t   stsca_flags[3];
    uint32_t  stsca_count;
    uint32_t  stsca_first_chunk;
    uint32_t  stsca_samp_per_chunk;
    uint32_t  stsca_samp_desc_id;

    uint32_t  stsza_size;
    uint32_t  stsza_type;
    uint8_t   stsza_version;
    uint8_t   stsza_flags[3];
    uint32_t  stsza_sample_size;
    uint32_t  stsza_count;

    uint32_t  stcoa_size;
    uint32_t  stcoa_type;
    uint8_t   stcoa_version;
    uint8_t   stcoa_flags[3];
    uint32_t  stcoa_count;

    // mdat box
    uint32_t  mdat_size;
    uint32_t  mdat_type;

    uint8_t   reserved[3];
    FILE     *fp;
    int       vw, vh;
    int       frate;
    int       samprate;
    int       sampnum;

    int       sttsv_off;
    int       stssv_off;
    int       stszv_off;
    int       stcov_off;
    int       sttsv_cur;
    int       stssv_cur;
    int       stszv_cur;
    int       stcov_cur;

    int       sttsa_off;
    int       stsza_off;
    int       stcoa_off;
    int       sttsa_cur;
    int       stsza_cur;
    int       stcoa_cur;

    int       chunk_off;
    uint32_t  vpts_last;
    uint32_t  apts_last;
    int       aframemax;
    int       vframemax;
    int       syncf_max;
    uint32_t *sttsv_buf;
    uint32_t *stssv_buf;
    uint32_t *stszv_buf;
    uint32_t *stcov_buf;
    uint32_t *sttsa_buf;
    uint32_t *stsza_buf;
    uint32_t *stcoa_buf;
    #define FLAG_VIDEO_H265_ENCODE (1 << 0)
    #define FLAG_AVC1_HEV1_WRITTEN (1 << 1)
    uint32_t  flags;
} MP4FILE;

typedef struct {
    uint32_t  avcc_size;
    uint32_t  avcc_type;
    uint8_t   avcc_config_ver;
    uint8_t   avcc_avc_profile;
    uint8_t   avcc_profile_compat;
    uint8_t   avcc_avc_level;
    uint8_t   avcc_nalulen;
    uint8_t   avcc_sps_num;
    uint16_t  avcc_sps_len;
    uint8_t   avcc_pps_num;
    uint16_t  avcc_pps_len;
} AVCCBOX;

typedef struct {
    uint32_t  hvcc_size;
    uint32_t  hvcc_type;
    uint8_t   configurationVersion;
    uint8_t   general_profile_idc   : 5;
    uint8_t   general_tier_flag     : 1;
    uint8_t   general_profile_space : 2;
    uint32_t  general_profile_compatibility_flags;
    uint32_t  general_constraint_indicator_flags0;
    uint16_t  general_constraint_indicator_flags1;
    uint8_t   general_level_idc;
    uint16_t  min_spatial_segmentation_idc;
    uint8_t   parallelismType;
    uint8_t   chromaFormat;
    uint8_t   bitDepthLumaMinus8;
    uint8_t   bitDepthChromaMinus8;
    uint16_t  avgFrameRate;
    uint8_t   lengthSizeMinusOne: 2;
    uint8_t   temporalIdNested  : 1;
    uint8_t   numTemporalLayers : 3;
    uint8_t   constantFrameRate : 2;
    uint8_t   numOfArrays;
} HVCCBOX;
#pragma pack()

static uint8_t getbyte(uint8_t *buf1, int len1, uint8_t *buf2, int len2, int i)
{
    return (i < len1) ? buf1[i] : buf2[i - len1];
}

static void getdata(uint8_t *buf1, int len1, uint8_t *buf2, int len2, int i, uint8_t *data, int size)
{
    int n;
    if (i < len1) {
        n = (len1 - i) < size ? (len1 - i) : size;
        memcpy(data, buf1 + i, n);
        i += n, data += n, size -= n;
    }
    if (i < len1 + len2) {
        n = (len1 + len2 - i) < size ? (len1 + len2 - i) : size;
        memcpy(data, buf2 + i - len1, n);
    }
}

static void writedata(uint8_t *buf1, int len1, uint8_t *buf2, int len2, int i, int size, FILE *fp)
{
    int n;
    if (i < len1) {
        n = (len1 - i) < size ? (len1 - i) : size;
        fwrite(buf1 + i, n, 1, fp);
        i += n, size -= n;
    }
    if (i < len1 + len2) {
        n = (len1 + len2 - i) < size ? (len1 + len2 - i) : size;
        fwrite(buf2 + i - len1, n, 1, fp);
    }
}

static int h26x_parse_nalu_header(uint8_t *data1, int len1, uint8_t *data2, int len2, int idx, int *hsize)
{
    int len = len1 + len2, counter, i;
    for (counter = 0, i = idx; i < len; i++) {
        uint8_t byte = getbyte(data1, len1, data2, len2, i);
        if (byte == 0) counter++;
        else if (counter >= 2 && byte == 0x01) {
            *hsize = counter + 1;
            return i + 1;
        } else {
            counter = 0;
        }
    }
    return -1;
}

static void mp4muxer_write_avc1_box(MP4FILE *mp4, uint8_t *spsbuf, int spslen, uint8_t *ppsbuf, int ppslen)
{
    AVCCBOX avccbox = {0};

    avccbox.avcc_size          = htonl(STSDV_RESERVED_SIZE);
    avccbox.avcc_type          = MP4_FOURCC('a', 'v', 'c', 'C');
    avccbox.avcc_config_ver    = 1;
    avccbox.avcc_avc_profile   = spslen > 1 ? spsbuf[1] : 0;
    avccbox.avcc_profile_compat= spslen > 2 ? spsbuf[2] : 0;
    avccbox.avcc_avc_level     = spslen > 3 ? spsbuf[3] : 0;
    avccbox.avcc_nalulen       = 0xFF;
    avccbox.avcc_sps_num       = (0x7 << 5) | (1 << 0);
    avccbox.avcc_sps_len       = (uint16_t)(htonl(spslen) >> 16);
    avccbox.avcc_pps_num       = 1;
    avccbox.avcc_pps_len       = (uint16_t)(htonl(ppslen) >> 16);

    mp4->stsdv_ahvc1_type          = MP4_FOURCC('a', 'v', 'c', '1');
    mp4->stsdv_ahvc1_data_refidx   = (uint16_t)(htonl(1 ) >> 16);
    mp4->stsdv_ahvc1_width         = (uint16_t)(htonl(mp4->vw) >> 16);
    mp4->stsdv_ahvc1_height        = (uint16_t)(htonl(mp4->vh) >> 16);
    mp4->stsdv_ahvc1_horiz_res     = htonl(0x480000);
    mp4->stsdv_ahvc1_vert_res      = htonl(0x480000);
    mp4->stsdv_ahvc1_frame_count   = (uint16_t)(htonl(1 ) >> 16);
    mp4->stsdv_ahvc1_depth         = (uint16_t)(htonl(24) >> 16);
    mp4->stsdv_ahvc1_predefined    = 0xFFFF;

    fseek(mp4->fp, offsetof(MP4FILE, stsdv_ahvc1_size), SEEK_SET);
    fwrite(&mp4->stsdv_ahvc1_size, offsetof(MP4FILE, stsdv_ahvcc_reserved) - offsetof(MP4FILE, stsdv_ahvc1_size), 1, mp4->fp);
    fwrite(&avccbox, offsetof(AVCCBOX, avcc_pps_num), 1, mp4->fp);
    fwrite(spsbuf, spslen, 1, mp4->fp);
    fwrite(&avccbox.avcc_pps_num, sizeof(avccbox.avcc_pps_num) + sizeof(avccbox.avcc_pps_len), 1, mp4->fp);
    fwrite(ppsbuf, ppslen, 1, mp4->fp);
    fseek(mp4->fp, 0, SEEK_END);
}

static void mp4muxer_write_hev1_box(MP4FILE *mp4, uint8_t *vpsbuf, int vpslen, uint8_t *spsbuf, int spslen, uint8_t *ppsbuf, int ppslen)
{
    HVCCBOX hvccbox = {0};

    hvccbox.hvcc_size            = htonl(STSDV_RESERVED_SIZE);
    hvccbox.hvcc_type            = MP4_FOURCC('h', 'v', 'c', 'C');
    hvccbox.configurationVersion = 1;
//  hvccbox.general_profile_space= 0;
//  hvccbox.general_tier_flag    = 0;
    hvccbox.general_profile_idc  = 1;
    hvccbox.general_profile_compatibility_flags = 0x60;
    hvccbox.general_constraint_indicator_flags0 = 0x90;
    hvccbox.general_constraint_indicator_flags1 = 0x00;
    hvccbox.general_level_idc    = 63;
    hvccbox.min_spatial_segmentation_idc = 0xf0;
    hvccbox.parallelismType      = 0 | 0xfc;
    hvccbox.chromaFormat         = 1 | 0xfc;
    hvccbox.bitDepthLumaMinus8   = 0 | 0xf8;
    hvccbox.bitDepthChromaMinus8 = 0 | 0xf8;
    hvccbox.avgFrameRate         = (uint16_t)(htonl(mp4->frate) >> 16);
//  hvccbox.constantFrameRate    = 0;
    hvccbox.numTemporalLayers    = 1;
    hvccbox.temporalIdNested     = 1;
    hvccbox.lengthSizeMinusOne   = 3;
    hvccbox.numOfArrays          = 3;

    mp4->stsdv_ahvc1_type        = MP4_FOURCC('h', 'v', 'c', '1');
    mp4->stsdv_ahvc1_data_refidx = (uint16_t)(htonl(1 ) >> 16);
    mp4->stsdv_ahvc1_width       = (uint16_t)(htonl(mp4->vw) >> 16);
    mp4->stsdv_ahvc1_height      = (uint16_t)(htonl(mp4->vh) >> 16);
    mp4->stsdv_ahvc1_horiz_res   = htonl(0x480000);
    mp4->stsdv_ahvc1_vert_res    = htonl(0x480000);
    mp4->stsdv_ahvc1_frame_count = (uint16_t)(htonl(1 ) >> 16);
    mp4->stsdv_ahvc1_depth       = (uint16_t)(htonl(24) >> 16);
    mp4->stsdv_ahvc1_predefined  = 0xFFFF;

    fseek(mp4->fp, offsetof(MP4FILE, stsdv_ahvc1_size), SEEK_SET);
    fwrite(&mp4->stsdv_ahvc1_size, offsetof(MP4FILE, stsdv_ahvcc_reserved) - offsetof(MP4FILE, stsdv_ahvc1_size), 1, mp4->fp);
    fwrite(&hvccbox, sizeof(hvccbox), 1, mp4->fp);

    fputc((0 << 7) | 32, mp4->fp);
    fputc(0x00, mp4->fp);
    fputc(0x01, mp4->fp);
    fputc((vpslen >> 8) & 0xFF, mp4->fp);
    fputc((vpslen >> 0) & 0xFF, mp4->fp);
    fwrite(vpsbuf, vpslen, 1, mp4->fp);

    fputc((0 << 7) | 33, mp4->fp);
    fputc(0x00, mp4->fp);
    fputc(0x01, mp4->fp);
    fputc((spslen >> 8) & 0xFF, mp4->fp);
    fputc((spslen >> 0) & 0xFF, mp4->fp);
    fwrite(spsbuf, spslen, 1, mp4->fp);

    fputc((0 << 7) | 34, mp4->fp);
    fputc(0x00, mp4->fp);
    fputc(0x01, mp4->fp);
    fputc((ppslen >> 8) & 0xFF, mp4->fp);
    fputc((ppslen >> 0) & 0xFF, mp4->fp);
    fwrite(ppsbuf, ppslen, 1, mp4->fp);
    fseek(mp4->fp, 0, SEEK_END);
}

static void write_fixed_trackv_data(MP4FILE *mp4)
{
    if (ENABLE_RECALCULATE_DURATION) { // re-calculate and re-write duration
        mp4->mvhd_duration = htonl((ntohl(mp4->stszv_count) + mp4->frate - 1) / mp4->frate * 1000);
        fseek(mp4->fp, offsetof(MP4FILE, mvhd_duration), SEEK_SET);
        fwrite(&mp4->mvhd_duration, sizeof(uint32_t) * 1, 1, mp4->fp);
    }
#if VIDEO_TIMESCALE_BY_FRAME_RATE
    if (1) {
        fseek(mp4->fp, mp4->sttsv_off + 12, SEEK_SET);
        fwrite(&mp4->sttsv_count , sizeof(uint32_t) * 1, 1, mp4->fp);
        fwrite(&mp4->sttsv_buf[0], sizeof(uint32_t) * 2, 1, mp4->fp);
    }
#else
    if (mp4->sttsv_buf && mp4->sttsv_cur < (int)ntohl(mp4->sttsv_count)) {
        fseek(mp4->fp, mp4->sttsv_off + 12, SEEK_SET);
        fwrite(&mp4->sttsv_count, sizeof(uint32_t), 1, mp4->fp);
        fseek(mp4->fp, mp4->sttsv_cur * sizeof(uint32_t) * 2, SEEK_CUR);
        fwrite(&mp4->sttsv_buf[mp4->sttsv_cur], (ntohl(mp4->sttsv_count) - mp4->sttsv_cur) * sizeof(uint32_t) * 2, 1, mp4->fp);
        mp4->sttsv_cur = ntohl(mp4->sttsv_count);
    }
#endif
    if (mp4->stssv_buf && mp4->stssv_cur < (int)ntohl(mp4->stssv_count)) {
        fseek(mp4->fp, mp4->stssv_off + 12, SEEK_SET);
        fwrite(&mp4->stssv_count, sizeof(uint32_t), 1, mp4->fp);
        fseek(mp4->fp, mp4->stssv_cur * sizeof(uint32_t), SEEK_CUR);
        fwrite(&mp4->stssv_buf[mp4->stssv_cur], (ntohl(mp4->stssv_count) - mp4->stssv_cur) * sizeof(uint32_t), 1, mp4->fp);
        mp4->stssv_cur = ntohl(mp4->stssv_count);
    }
    if (mp4->stszv_buf && mp4->stszv_cur < (int)ntohl(mp4->stszv_count)) {
        fseek(mp4->fp, mp4->stszv_off + 16, SEEK_SET);
        fwrite(&mp4->stszv_count, sizeof(uint32_t), 1, mp4->fp);
        fseek(mp4->fp, mp4->stszv_cur * sizeof(uint32_t), SEEK_CUR);
        fwrite(&mp4->stszv_buf[mp4->stszv_cur], (ntohl(mp4->stszv_count) - mp4->stszv_cur) * sizeof(uint32_t), 1, mp4->fp);
        mp4->stszv_cur = ntohl(mp4->stszv_count);
    }
    if (mp4->stcov_buf && mp4->stcov_cur < (int)ntohl(mp4->stcov_count)) {
        fseek(mp4->fp, mp4->stcov_off + 12, SEEK_SET);
        fwrite(&mp4->stcov_count, sizeof(uint32_t), 1, mp4->fp);
        fseek(mp4->fp, mp4->stcov_cur * sizeof(uint32_t), SEEK_CUR);
        fwrite(&mp4->stcov_buf[mp4->stcov_cur], (ntohl(mp4->stcov_count) - mp4->stcov_cur) * sizeof(uint32_t), 1, mp4->fp);
        mp4->stcov_cur = ntohl(mp4->stcov_count);
    }
    fseek(mp4->fp, ntohl(mp4->ftyp_size) + (int)ntohl(mp4->moov_size), SEEK_SET);
    fwrite(&mp4->mdat_size, sizeof(uint32_t), 1, mp4->fp);
    fseek(mp4->fp, 0, SEEK_END);
}

static void write_fixed_tracka_data(MP4FILE *mp4)
{
#if AUDIO_TIMESCALE_BY_SAMPLE_RATE
    if (1) {
        fseek(mp4->fp, mp4->sttsa_off + 12, SEEK_SET);
        fwrite(&mp4->sttsa_count , sizeof(uint32_t) * 1, 1, mp4->fp);
        fwrite(&mp4->sttsa_buf[0], sizeof(uint32_t) * 2, 1, mp4->fp);
    }
#else
    if (mp4->sttsa_buf && mp4->sttsa_cur < (int)ntohl(mp4->sttsa_count)) {
        fseek(mp4->fp, mp4->sttsa_off + 12, SEEK_SET);
        fwrite(&mp4->sttsa_count, sizeof(uint32_t), 1, mp4->fp);
        fseek(mp4->fp, mp4->sttsa_cur * sizeof(uint32_t) * 2, SEEK_CUR);
        fwrite(&mp4->sttsa_buf[mp4->sttsa_cur], (ntohl(mp4->sttsa_count) - mp4->sttsa_cur) * sizeof(uint32_t) * 2, 1, mp4->fp);
        mp4->sttsa_cur = ntohl(mp4->sttsa_count);
    }
#endif
    if (mp4->stsza_buf && mp4->stsza_cur < (int)ntohl(mp4->stsza_count)) {
        fseek(mp4->fp, mp4->stsza_off + 16, SEEK_SET);
        fwrite(&mp4->stsza_count, sizeof(uint32_t), 1, mp4->fp);
        fseek(mp4->fp, mp4->stsza_cur * sizeof(uint32_t), SEEK_CUR);
        fwrite(&mp4->stsza_buf[mp4->stsza_cur], (ntohl(mp4->stsza_count) - mp4->stsza_cur) * sizeof(uint32_t), 1, mp4->fp);
        mp4->stsza_cur = ntohl(mp4->stsza_count);
    }
    if (mp4->stcoa_buf && mp4->stcoa_cur < (int)ntohl(mp4->stcoa_count)) {
        fseek(mp4->fp, mp4->stcoa_off + 12, SEEK_SET);
        fwrite(&mp4->stcoa_count, sizeof(uint32_t), 1, mp4->fp);
        fseek(mp4->fp, mp4->stcoa_cur * sizeof(uint32_t), SEEK_CUR);
        fwrite(&mp4->stcoa_buf[mp4->stcoa_cur], (ntohl(mp4->stcoa_count) - mp4->stcoa_cur) * sizeof(uint32_t), 1, mp4->fp);
        mp4->stcoa_cur = ntohl(mp4->stcoa_count);
    }
    fseek(mp4->fp, ntohl(mp4->ftyp_size) + (int)ntohl(mp4->moov_size), SEEK_SET);
    fwrite(&mp4->mdat_size, sizeof(uint32_t), 1, mp4->fp);
    fseek(mp4->fp, 0, SEEK_END);
}

void* mp4muxer_init(char *file, int duration, int w, int h, int frate, int gop, int h265, int chnum, int samprate, int sampbits, int sampnum, unsigned char *aacspecinfo)
{
    MP4FILE *mp4 = calloc(1, sizeof(MP4FILE));
    if (!mp4) return NULL;

    mp4->fp      = fopen(file, "wb");
    mp4->vw      = w;
    mp4->vh      = h;
    mp4->frate   = frate;
    mp4->samprate= samprate;
    mp4->sampnum = sampnum;
    mp4->flags  |= h265 ? FLAG_VIDEO_H265_ENCODE : 0;
    if (!mp4->fp) {
        free(mp4);
        return NULL;
    }

    mp4->ftyp_size           = htonl(offsetof(MP4FILE, moov_size ) - offsetof(MP4FILE, ftyp_size));
    mp4->ftyp_type           = MP4_FOURCC('f', 't', 'y', 'p');
    mp4->ftyp_brand          = MP4_FOURCC('i', 's', 'o', 'm');
    mp4->ftyp_version        = htonl(512);
    mp4->ftyp_compat1        = MP4_FOURCC('i', 's', 'o', 'm');
    mp4->ftyp_compat2        = MP4_FOURCC('i', 's', 'o', '2');
    mp4->ftyp_compat3        = MP4_FOURCC('m', 'p', '4', '1');

    mp4->moov_size           = offsetof(MP4FILE, trakv_size) - offsetof(MP4FILE, moov_size);
    mp4->moov_type           = MP4_FOURCC('m', 'o', 'o', 'v');
    mp4->mvhd_size           = htonl(offsetof(MP4FILE, trakv_size) - offsetof(MP4FILE, mvhd_size));
    mp4->mvhd_type           = MP4_FOURCC('m', 'v', 'h', 'd');
    mp4->mvhd_create_time    = htonl(time(NULL) + 2082844800ul);
    mp4->mvhd_modify_time    = mp4->mvhd_create_time;
    mp4->mvhd_timescale      = htonl(1000      );
    mp4->mvhd_duration       = htonl(duration  );
    mp4->mvhd_playrate       = htonl(0x00010000);
    mp4->mvhd_volume         = (uint16_t)(htonl(0x0100) >> 16);
    mp4->mvhd_matrix[0]      = htonl(0x00010000);
    mp4->mvhd_matrix[4]      = htonl(0x00010000);
    mp4->mvhd_matrix[8]      = htonl(0x40000000);
    mp4->mvhd_next_trackid   = htonl(3         );

    // video track
    mp4->trakv_size          = offsetof(MP4FILE, mdiav_size) - offsetof(MP4FILE, trakv_size);
    mp4->trakv_type          = MP4_FOURCC('t', 'r', 'a', 'k');
    mp4->tkhdv_size          = htonl(offsetof(MP4FILE, mdiav_size) - offsetof(MP4FILE, tkhdv_size));
    mp4->tkhdv_type          = MP4_FOURCC('t', 'k', 'h', 'd');
    mp4->tkhdv_flags[2]      = 0xF;
    mp4->tkhdv_trackid       = htonl(1         );
    mp4->tkhdv_duration      = htonl(duration  );
    mp4->tkhdv_matrix[0]     = htonl(0x00010000);
    mp4->tkhdv_matrix[4]     = htonl(0x00010000);
    mp4->tkhdv_matrix[8]     = htonl(0x40000000);
    mp4->tkhdv_width         = htonl(w << 16   );
    mp4->tkhdv_height        = htonl(h << 16   );

    mp4->mdiav_size          = offsetof(MP4FILE, minfv_size) - offsetof(MP4FILE, mdiav_size);
    mp4->mdiav_type          = MP4_FOURCC('m', 'd', 'i', 'a');
    mp4->mdhdv_size          = htonl(offsetof(MP4FILE, hdlrv_size) - offsetof(MP4FILE, mdhdv_size));
    mp4->mdhdv_type          = MP4_FOURCC('m', 'd', 'h', 'd');
#if VIDEO_TIMESCALE_BY_FRAME_RATE
    mp4->mdhdv_timescale     = htonl(frate);
    mp4->mdhdv_duration      = htonl(duration * frate / 1000);
#else
    mp4->mdhdv_timescale     = htonl(1000 );
    mp4->mdhdv_duration      = htonl(duration);
#endif
    mp4->hdlrv_size          = htonl(offsetof(MP4FILE, minfv_size) - offsetof(MP4FILE, hdlrv_size));
    mp4->hdlrv_type          = MP4_FOURCC('h', 'd', 'l', 'r');
    mp4->hdlrv_handler_type  = MP4_FOURCC('v', 'i', 'd', 'e');
    strcpy((char*)mp4->hdlrv_name, "VideoHandler");

    mp4->minfv_size          = offsetof(MP4FILE, stblv_size) - offsetof(MP4FILE, minfv_size);
    mp4->minfv_type          = MP4_FOURCC('m', 'i', 'n', 'f');
    mp4->vmhdv_size          = htonl(offsetof(MP4FILE, dinfv_size) - offsetof(MP4FILE, vmhdv_size));
    mp4->vmhdv_type          = MP4_FOURCC('v', 'm', 'h', 'd');
    mp4->vmhdv_flags[2]      = 1;

    mp4->dinfv_size          = htonl(offsetof(MP4FILE, stblv_size) - offsetof(MP4FILE, dinfv_size));
    mp4->dinfv_type          = MP4_FOURCC('d', 'i', 'n', 'f');
    mp4->drefv_size          = htonl(offsetof(MP4FILE, stblv_size) - offsetof(MP4FILE, drefv_size));
    mp4->drefv_type          = MP4_FOURCC('d', 'r', 'e', 'f');
    mp4->drefv_entry_count   = htonl(1);
    mp4->urlv_size           = htonl(offsetof(MP4FILE, stblv_size) - offsetof(MP4FILE, urlv_size ));
    mp4->urlv_type           = MP4_FOURCC('u', 'r', 'l', ' ');
    mp4->urlv_flags[2]       = 1;

    mp4->stblv_size          = offsetof(MP4FILE, stsdv_size) - offsetof(MP4FILE, stblv_size);
    mp4->stblv_type          = MP4_FOURCC('s', 't', 'b', 'l');
    mp4->stsdv_size          = offsetof(MP4FILE, stsdv_ahvc1_size) - offsetof(MP4FILE, stsdv_size);
    mp4->stsdv_type          = MP4_FOURCC('s', 't', 's', 'd');
    mp4->stsdv_entry_count   = htonl(1);

    mp4->stsdv_ahvc1_size    = offsetof(MP4FILE, sttsv_size) - offsetof(MP4FILE, stsdv_ahvc1_size);
    mp4->stsdv_ahvc1_type    = MP4_FOURCC('u', 'k', 'n', 'w');

    mp4->vframemax           = (int)((int64_t)duration * frate / 1000 + frate / 2);
    mp4->syncf_max           = mp4->vframemax / gop;

#if VIDEO_TIMESCALE_BY_FRAME_RATE
    mp4->sttsv_size          = 16 + 1 * sizeof(uint32_t) * 2;
#else
    mp4->sttsv_size          = 16 + mp4->vframemax * sizeof(uint32_t) * 2;
#endif
    mp4->sttsv_type          = MP4_FOURCC('s', 't', 't', 's');

    mp4->stssv_size          = 16 + mp4->syncf_max * sizeof(uint32_t) * 1;
    mp4->stssv_type          = MP4_FOURCC('s', 't', 's', 's');

    mp4->stscv_size          = 16 + sizeof(uint32_t) * 3;
    mp4->stscv_type          = MP4_FOURCC('s', 't', 's', 'c');
    mp4->stscv_count         = htonl(1);
    mp4->stscv_first_chunk   = htonl(1);
    mp4->stscv_samp_per_chunk= htonl(1);
    mp4->stscv_samp_desc_id  = htonl(1);

    mp4->stszv_size          = 20 + mp4->vframemax * sizeof(uint32_t) * 1;
    mp4->stszv_type          = MP4_FOURCC('s', 't', 's', 'z');
    mp4->stszv_sample_size   = 0;

    mp4->stcov_size          = 16 + mp4->vframemax * sizeof(uint32_t) * 1;
    mp4->stcov_type          = MP4_FOURCC('s', 't', 'c', 'o');

    mp4->stsdv_size         += mp4->stsdv_ahvc1_size;
    mp4->stblv_size         += mp4->stsdv_size + mp4->sttsv_size + mp4->stssv_size + mp4->stscv_size + mp4->stszv_size + mp4->stcov_size;
    mp4->minfv_size         += mp4->stblv_size;
    mp4->mdiav_size         += mp4->minfv_size;
    mp4->trakv_size         += mp4->mdiav_size;
    mp4->moov_size          += mp4->trakv_size;

    mp4->sttsv_off           = offsetof(MP4FILE, sttsv_size);
    mp4->stssv_off           = mp4->sttsv_off + mp4->sttsv_size;
    mp4->stszv_off           = mp4->stssv_off + mp4->stssv_size + mp4->stscv_size;
    mp4->stcov_off           = mp4->stszv_off + mp4->stszv_size;

    mp4->sttsv_buf           = calloc(1, mp4->sttsv_size - 16);
    mp4->stssv_buf           = calloc(1, mp4->stssv_size - 16);
    mp4->stszv_buf           = calloc(1, mp4->stszv_size - 20);
    mp4->stcov_buf           = calloc(1, mp4->stcov_size - 16);

    mp4->sttsv_size          = htonl(mp4->sttsv_size);
    mp4->stssv_size          = htonl(mp4->stssv_size);
    mp4->stscv_size          = htonl(mp4->stscv_size);
    mp4->stszv_size          = htonl(mp4->stszv_size);
    mp4->stcov_size          = htonl(mp4->stcov_size);

    mp4->stsdv_ahvc1_size    = htonl(mp4->stsdv_ahvc1_size);
    mp4->stsdv_size          = htonl(mp4->stsdv_size);
    mp4->stblv_size          = htonl(mp4->stblv_size);
    mp4->minfv_size          = htonl(mp4->minfv_size);
    mp4->mdiav_size          = htonl(mp4->mdiav_size);
    mp4->trakv_size          = htonl(mp4->trakv_size);

    // audio track
    mp4->traka_size          = offsetof(MP4FILE, mdiaa_size) - offsetof(MP4FILE, traka_size);
    mp4->traka_type          = MP4_FOURCC('t', 'r', 'a', 'k');
    mp4->tkhda_size          = htonl(offsetof(MP4FILE, mdiaa_size) - offsetof(MP4FILE, tkhda_size));
    mp4->tkhda_type          = MP4_FOURCC('t', 'k', 'h', 'd');
    mp4->tkhda_flags[2]      = 0xF;
    mp4->tkhda_trackid       = htonl(2       );
    mp4->tkhda_duration      = htonl(duration);
    mp4->tkhda_volume        = 0x0100;

    mp4->mdiaa_size          = offsetof(MP4FILE, minfa_size) - offsetof(MP4FILE, mdiaa_size);
    mp4->mdiaa_type          = MP4_FOURCC('m', 'd', 'i', 'a');
    mp4->mdhda_size          = htonl(offsetof(MP4FILE, hdlra_size) - offsetof(MP4FILE, mdhda_size));
    mp4->mdhda_type          = MP4_FOURCC('m', 'd', 'h', 'd');
#if AUDIO_TIMESCALE_BY_SAMPLE_RATE
    mp4->mdhda_timescale     = htonl(samprate);
    mp4->mdhda_duration      = htonl(duration * samprate / 1000);
#else
    mp4->mdhda_timescale     = htonl(1000    );
    mp4->mdhda_duration      = htonl(duration);
#endif
    mp4->hdlra_size          = htonl(offsetof(MP4FILE, minfa_size) - offsetof(MP4FILE, hdlra_size));
    mp4->hdlra_type          = MP4_FOURCC('h', 'd', 'l', 'r');
    mp4->hdlra_handler_type  = MP4_FOURCC('s', 'o', 'u', 'n');;
    strcpy((char*)mp4->hdlra_name, "SoundHandler");

    mp4->minfa_size          = offsetof(MP4FILE, stbla_size) - offsetof(MP4FILE, minfa_size);
    mp4->minfa_type          = MP4_FOURCC('m', 'i', 'n', 'f');
    mp4->smhda_size          = htonl(offsetof(MP4FILE, dinfa_size) - offsetof(MP4FILE, smhda_size));
    mp4->smhda_type          = MP4_FOURCC('s', 'm', 'h', 'd');

    mp4->dinfa_size          = htonl(offsetof(MP4FILE, stbla_size) - offsetof(MP4FILE, dinfa_size));
    mp4->dinfa_type          = MP4_FOURCC('d', 'i', 'n', 'f');
    mp4->drefa_size          = htonl(offsetof(MP4FILE, stbla_size) - offsetof(MP4FILE, drefa_size));
    mp4->drefa_type          = MP4_FOURCC('d', 'r', 'e', 'f');
    mp4->drefa_entry_count   = htonl(1);
    mp4->urla_size           = htonl(offsetof(MP4FILE, stbla_size) - offsetof(MP4FILE, urla_size ));
    mp4->urla_type           = MP4_FOURCC('u', 'r', 'l', ' ');
    mp4->urla_flags[2]       = 1;

    mp4->stbla_size          = offsetof(MP4FILE, stsda_size) - offsetof(MP4FILE, stbla_size);
    mp4->stbla_type          = MP4_FOURCC('s', 't', 'b', 'l');
    mp4->stsda_size          = offsetof(MP4FILE, mp4a_size ) - offsetof(MP4FILE, stsda_size);
    mp4->stsda_type          = MP4_FOURCC('s', 't', 's', 'd');
    mp4->stsda_entry_count   = htonl(1);

    mp4->mp4a_size           = offsetof(MP4FILE, esds_size) - offsetof(MP4FILE, mp4a_size);
    mp4->mp4a_type           = MP4_FOURCC('m', 'p', '4', 'a');
    mp4->mp4a_data_refidx    = (uint16_t)(htonl(1       ) >> 16);
    mp4->mp4a_channel_num    = (uint16_t)(htonl(chnum   ) >> 16);
    mp4->mp4a_sample_size    = (uint16_t)(htonl(sampbits) >> 16);
    mp4->mp4a_sample_rate    = (uint16_t)(htonl(samprate << 16));

    mp4->esds_size           = offsetof(MP4FILE, sttsa_size) - offsetof(MP4FILE, esds_size);
    mp4->esds_type           = MP4_FOURCC('e', 's', 'd', 's');

    mp4->esds_esdesc_tag     = 0x03;
    mp4->esds_esdesc_len     = ((25 << 8) | 0x80);
    mp4->esds_esdesc_id      = 0x0200;
    mp4->esds_esdesc_flags   = 0x00;
    mp4->esds_deccfg_tag     = 0x04;
    mp4->esds_deccfg_len     = 17;
    mp4->esds_deccfg_object  = 0x40;
    mp4->esds_deccfg_stream  = 0x15;
    mp4->esds_decspec_tag    = 0x05;
    mp4->esds_decspec_len    = 2;
    mp4->esds_decspec_info   = aacspecinfo ? (aacspecinfo[1] << 8) | (aacspecinfo[0] << 0) : 0;
    mp4->esds_slcfg_tag      = 0x06;
    mp4->esds_slcfg_len      = 1;
    mp4->esds_slcfg_reserved = 0x02;

    mp4->aframemax           = (int)((int64_t)duration * samprate / 1000 / sampnum + samprate / sampnum / 2);
#if AUDIO_TIMESCALE_BY_SAMPLE_RATE
    mp4->sttsa_size          = 16 + 1 * sizeof(uint32_t) * 2;
#else
    mp4->sttsa_size          = 16 + mp4->aframemax * sizeof(uint32_t) * 2;
#endif
    mp4->sttsa_type          = MP4_FOURCC('s', 't', 't', 's');

    mp4->stsca_size          = 16 + sizeof(uint32_t) * 3;
    mp4->stsca_type          = MP4_FOURCC('s', 't', 's', 'c');
    mp4->stsca_count         = htonl(1);
    mp4->stsca_first_chunk   = htonl(1);
    mp4->stsca_samp_per_chunk= htonl(1);
    mp4->stsca_samp_desc_id  = htonl(1);

    mp4->stsza_size          = 20 + mp4->aframemax * sizeof(uint32_t) * 1;
    mp4->stsza_type          = MP4_FOURCC('s', 't', 's', 'z');
    mp4->stsza_sample_size   = 0;

    mp4->stcoa_size          = 16 + mp4->aframemax * sizeof(uint32_t) * 1;
    mp4->stcoa_type          = MP4_FOURCC('s', 't', 'c', 'o');

    mp4->mp4a_size          += mp4->esds_size;
    mp4->stsda_size         += mp4->mp4a_size;
    mp4->stbla_size         += mp4->stsda_size + mp4->sttsa_size + mp4->stsca_size + mp4->stsza_size + mp4->stcoa_size;
    mp4->minfa_size         += mp4->stbla_size;
    mp4->mdiaa_size         += mp4->minfa_size;
    mp4->traka_size         += mp4->mdiaa_size;
    mp4->moov_size          += mp4->traka_size;

    mp4->sttsa_off           = offsetof(MP4FILE, trakv_size) + ntohl(mp4->trakv_size) + offsetof(MP4FILE, sttsa_size) - offsetof(MP4FILE, traka_size);
    mp4->stsza_off           = mp4->sttsa_off + mp4->sttsa_size + mp4->stsca_size;
    mp4->stcoa_off           = mp4->stsza_off + mp4->stsza_size;

    mp4->sttsa_buf           = calloc(1, mp4->sttsa_size - 16);
    mp4->stsza_buf           = calloc(1, mp4->stsza_size - 20);
    mp4->stcoa_buf           = calloc(1, mp4->stcoa_size - 16);

    mp4->sttsa_size          = htonl(mp4->sttsa_size);
    mp4->stsca_size          = htonl(mp4->stsca_size);
    mp4->stsza_size          = htonl(mp4->stsza_size);
    mp4->stcoa_size          = htonl(mp4->stcoa_size);

    mp4->esds_size           = htonl(mp4->esds_size );
    mp4->mp4a_size           = htonl(mp4->mp4a_size );
    mp4->stsda_size          = htonl(mp4->stsda_size);
    mp4->stbla_size          = htonl(mp4->stbla_size);
    mp4->minfa_size          = htonl(mp4->minfa_size);
    mp4->mdiaa_size          = htonl(mp4->mdiaa_size);
    mp4->traka_size          = htonl(mp4->traka_size);

    mp4->moov_size           = htonl(mp4->moov_size );
    mp4->mdat_size           = htonl(8);
    mp4->mdat_type           = MP4_FOURCC('m', 'd', 'a', 't');

    fwrite(mp4, offsetof(MP4FILE, sttsv_size), 1, mp4->fp);
    fwrite(&mp4->sttsv_size, 16, 1, mp4->fp); fseek(mp4->fp, ntohl(mp4->sttsv_size) - 16, SEEK_CUR);
    fwrite(&mp4->stssv_size, 16, 1, mp4->fp); fseek(mp4->fp, ntohl(mp4->stssv_size) - 16, SEEK_CUR);
    fwrite(&mp4->stscv_size, ntohl(mp4->stscv_size), 1, mp4->fp);
    fwrite(&mp4->stszv_size, 20, 1, mp4->fp); fseek(mp4->fp, ntohl(mp4->stszv_size) - 20, SEEK_CUR);
    fwrite(&mp4->stcov_size, 16, 1, mp4->fp); fseek(mp4->fp, ntohl(mp4->stcov_size) - 16, SEEK_CUR);
    fwrite(&mp4->traka_size, offsetof(MP4FILE, sttsa_size) - offsetof(MP4FILE, traka_size), 1, mp4->fp);
    fwrite(&mp4->sttsa_size, 16, 1, mp4->fp); fseek(mp4->fp, ntohl(mp4->sttsa_size) - 16, SEEK_CUR);
    fwrite(&mp4->stsca_size, ntohl(mp4->stsca_size), 1, mp4->fp);
    fwrite(&mp4->stsza_size, 20, 1, mp4->fp); fseek(mp4->fp, ntohl(mp4->stsza_size) - 20, SEEK_CUR);
    fwrite(&mp4->stcoa_size, 16, 1, mp4->fp); fseek(mp4->fp, ntohl(mp4->stcoa_size) - 16, SEEK_CUR);
    fwrite(&mp4->mdat_size , ntohl(mp4->mdat_size), 1, mp4->fp);

    mp4->chunk_off = ntohl(mp4->ftyp_size) + ntohl(mp4->moov_size) + ntohl(mp4->mdat_size);
    return mp4;
}

void mp4muxer_exit(void *ctx)
{
    MP4FILE *mp4 = (MP4FILE*)ctx;
    if (mp4) {
        write_fixed_trackv_data(mp4);
        write_fixed_tracka_data(mp4);
        fclose(mp4->fp);
        if (mp4->sttsv_buf) free(mp4->sttsv_buf);
        if (mp4->stssv_buf) free(mp4->stssv_buf);
        if (mp4->stszv_buf) free(mp4->stszv_buf);
        if (mp4->stcov_buf) free(mp4->stcov_buf);
        if (mp4->sttsa_buf) free(mp4->sttsa_buf);
        if (mp4->stsza_buf) free(mp4->stsza_buf);
        if (mp4->stcoa_buf) free(mp4->stcoa_buf);
        free(mp4);
    }
}

void mp4muxer_video(void *ctx, unsigned char *buf1, int len1, unsigned char *buf2, int len2, int key, unsigned pts)
{
    MP4FILE *mp4 = (MP4FILE*)ctx;
    uint8_t  vpsbuf[256], spsbuf[256], ppsbuf[256];
    int      vpslen = 0,  spslen = 0,  ppslen = 0;
    int      nalu_idx, nalu_len, nalu_type, hsize;
    int      len = len1 + len2, i = 0;
    uint32_t framesize = 0, u32tempvalue;
    if (!ctx) return;

    for (i = h26x_parse_nalu_header(buf1, len1, buf2, len2, i, &hsize); i >= 0 && i < len; ) {
        nalu_idx = i;
        i = h26x_parse_nalu_header(buf1, len1, buf2, len2, i, &hsize);
        nalu_len = (i == -1) ? (len - nalu_idx) : (i - hsize - nalu_idx);

        if (mp4->flags & FLAG_VIDEO_H265_ENCODE) {
            nalu_type = (getbyte(buf1, len1, buf2, len2, nalu_idx) & 0x7F) >> 1;
            key = nalu_type == 19 || nalu_type == 20;
            if (!(mp4->flags & FLAG_AVC1_HEV1_WRITTEN)) {
                switch (nalu_type) {
                case 32: vpslen = nalu_len < sizeof(vpsbuf) ? nalu_len : sizeof(vpsbuf); getdata(buf1, len1, buf2, len2, nalu_idx, vpsbuf, vpslen); break;
                case 33: spslen = nalu_len < sizeof(spsbuf) ? nalu_len : sizeof(spsbuf); getdata(buf1, len1, buf2, len2, nalu_idx, spsbuf, spslen); break;
                case 34: ppslen = nalu_len < sizeof(ppsbuf) ? nalu_len : sizeof(ppsbuf); getdata(buf1, len1, buf2, len2, nalu_idx, ppsbuf, ppslen); break;
                }
                if (vpslen && spslen && ppslen) {
                    mp4muxer_write_hev1_box(mp4, vpsbuf, vpslen, spsbuf, spslen, ppsbuf, ppslen);
                    mp4->flags |= FLAG_AVC1_HEV1_WRITTEN;
                }
            }
        } else {
            nalu_type = (getbyte(buf1, len1, buf2, len2, nalu_idx) & 0x1F) >> 0;
            key = nalu_type == 5;
            if (!(mp4->flags & FLAG_AVC1_HEV1_WRITTEN)) {
                switch (nalu_type) {
                case 7: spslen = nalu_len < sizeof(spsbuf) ? nalu_len : sizeof(spsbuf); getdata(buf1, len1, buf2, len2, nalu_idx, spsbuf, spslen); break;
                case 8: ppslen = nalu_len < sizeof(ppsbuf) ? nalu_len : sizeof(ppsbuf); getdata(buf1, len1, buf2, len2, nalu_idx, ppsbuf, ppslen); break;
                }
                if (spslen && ppslen) {
                    mp4muxer_write_avc1_box(mp4, spsbuf, spslen, ppsbuf, ppslen);
                    mp4->flags |= FLAG_AVC1_HEV1_WRITTEN;
                }
            }
        }
        if (1 || ((mp4->flags & FLAG_VIDEO_H265_ENCODE) && nalu_type >= 0 && nalu_type <= 21) || (!(mp4->flags & FLAG_VIDEO_H265_ENCODE) && nalu_type >= 1 && nalu_type <= 5)) {
            u32tempvalue = htonl(nalu_len);
            framesize   += sizeof(uint32_t) + nalu_len;
            fwrite(&u32tempvalue, sizeof(u32tempvalue), 1, mp4->fp);
            writedata(buf1, len1, buf2, len2, nalu_idx, nalu_len, mp4->fp);
        }
    }

    if (mp4->stszv_buf && (int)ntohl(mp4->stszv_count) < mp4->vframemax) {
        mp4->stszv_buf[ntohl(mp4->stszv_count)] = htonl(framesize);
        mp4->stszv_count = htonl(ntohl(mp4->stszv_count) + 1);
    }
    if (mp4->stssv_buf && (int)ntohl(mp4->stssv_count) < mp4->syncf_max && key) {
        mp4->stssv_buf[ntohl(mp4->stssv_count)] = mp4->stszv_count;
        mp4->stssv_count = htonl(ntohl(mp4->stssv_count) + 1);
    }
#if VIDEO_TIMESCALE_BY_FRAME_RATE
    if (mp4->sttsv_buf) {
        mp4->sttsv_buf[0] = mp4->stszv_count;
        mp4->sttsv_buf[1] = htonl(1);
        mp4->sttsv_count  = htonl(1);
    }
#else
    if (mp4->sttsv_buf && (int)ntohl(mp4->sttsv_count) < mp4->vframemax) {
        mp4->sttsv_buf[ntohl(mp4->sttsv_count) * 2 + 0] = htonl(1);
        mp4->sttsv_buf[ntohl(mp4->sttsv_count) * 2 + 1] = htonl(mp4->vpts_last ? pts - mp4->vpts_last : 1000 / mp4->frate);
        mp4->sttsv_count = htonl(ntohl(mp4->sttsv_count) + 1);
        mp4->vpts_last   = pts;
    }
#endif
    if (mp4->stcov_buf && (int)ntohl(mp4->stcov_count) < mp4->vframemax) {
        mp4->stcov_buf[ntohl(mp4->stcov_count)] = htonl(mp4->chunk_off);
        mp4->stcov_count = htonl(ntohl(mp4->stcov_count) + 1);
    }
    mp4->mdat_size  = htonl(ntohl(mp4->mdat_size) + framesize);
    mp4->chunk_off += framesize;
#if 1
    if (ntohl(mp4->stszv_count) % (mp4->frate * 5) == 0) {
        write_fixed_trackv_data(mp4);
        write_fixed_tracka_data(mp4);
    }
#endif
}

void mp4muxer_audio(void *ctx, unsigned char *buf1, int len1, unsigned char *buf2, int len2, int key, unsigned pts)
{
    MP4FILE *mp4 = (MP4FILE*)ctx;
    int      len = len1 + len2;
    if (!ctx) return;

    if (mp4->stsza_buf && (int)ntohl(mp4->stsza_count) < mp4->aframemax) {
        mp4->stsza_buf[ntohl(mp4->stsza_count)] = htonl(len);
        mp4->stsza_count = htonl(ntohl(mp4->stsza_count) + 1);
    }

#if AUDIO_TIMESCALE_BY_SAMPLE_RATE
    if (mp4->sttsa_buf) {
        mp4->sttsa_buf[0] = mp4->stsza_count;
        mp4->sttsa_buf[1] = htonl(mp4->sampnum);
        mp4->sttsa_count  = htonl(1);
    }
#else
    if (mp4->sttsa_buf && (int)ntohl(mp4->sttsa_count) < mp4->aframemax) {
        mp4->sttsa_buf[ntohl(mp4->sttsa_count) * 2 + 0] = htonl(1);
        mp4->sttsa_buf[ntohl(mp4->sttsa_count) * 2 + 1] = htonl(mp4->apts_last ? pts - mp4->apts_last : 1000 * mp4->sampnum / mp4->samprate);
        mp4->sttsa_count = htonl(ntohl(mp4->sttsa_count) + 1);
        mp4->apts_last = pts;
    }
#endif

    if (mp4->stcoa_buf && (int)ntohl(mp4->stcoa_count) < mp4->aframemax) {
        mp4->stcoa_buf[ntohl(mp4->stcoa_count)] = htonl(mp4->chunk_off);
        mp4->stcoa_count = htonl(ntohl(mp4->stcoa_count) + 1);
    }

    mp4->mdat_size = htonl(ntohl(mp4->mdat_size) + len);
    mp4->chunk_off+= len;
    if (len1) fwrite(buf1, len1, 1, mp4->fp);
    if (len2) fwrite(buf2, len2, 1, mp4->fp);
}


