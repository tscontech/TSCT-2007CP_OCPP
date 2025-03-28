﻿#include "config.h"
#include "bitstream.h"
#include "common_ps.h"

#ifdef PARSING_HE_AAC_V2

#define EXTENSION_ID_PS 2
#define MAX_PS_ENVELOPES 5
#define NO_ALLPASS_LINKS 3

typedef struct
{
    /* bitstream parameters */
    uint8_t enable_iid;
    uint8_t enable_icc;
    uint8_t enable_ext;

    uint8_t iid_mode;
    uint8_t icc_mode;
    uint8_t nr_iid_par;
    uint8_t nr_ipdopd_par;
    uint8_t nr_icc_par;

    uint8_t frame_class;
    uint8_t num_env;

    uint8_t border_position[MAX_PS_ENVELOPES+1];

    uint8_t iid_dt[MAX_PS_ENVELOPES];
    uint8_t icc_dt[MAX_PS_ENVELOPES];

    uint8_t enable_ipdopd;
    uint8_t ipd_mode;
    uint8_t ipd_dt[MAX_PS_ENVELOPES];
    uint8_t opd_dt[MAX_PS_ENVELOPES];

    /* indices */
    int8_t iid_index_prev[34];
    int8_t icc_index_prev[34];
    int8_t ipd_index_prev[17];
    int8_t opd_index_prev[17];
    int8_t iid_index[MAX_PS_ENVELOPES][34];
    int8_t icc_index[MAX_PS_ENVELOPES][34];
    int8_t ipd_index[MAX_PS_ENVELOPES][17];
    int8_t opd_index[MAX_PS_ENVELOPES][17];

    int8_t ipd_index_1[17];
    int8_t opd_index_1[17];
    int8_t ipd_index_2[17];
    int8_t opd_index_2[17];

    /* ps data was correctly read */
    uint8_t ps_data_available;

    /* a header has been read */
    uint8_t header_read;

    /* hybrid filterbank parameters */
    void *hyb;
    uint8_t use34hybrid_bands;
    uint8_t numTimeSlotsRate;

    /**/
    uint8_t num_groups;
    uint8_t num_hybrid_groups;
    uint8_t nr_par_bands;
    uint8_t nr_allpass_bands;
    uint8_t decay_cutoff;

    uint8_t *group_border;
    uint16_t *map_group2bk;

    /* filter delay handling */
    uint8_t saved_delay;
    uint8_t delay_buf_index_ser[NO_ALLPASS_LINKS];
    uint8_t num_sample_delay_ser[NO_ALLPASS_LINKS];
    uint8_t delay_D[64];
    uint8_t delay_buf_index_delay[64];

    complex_t delay_Qmf[14][64]; /* 14 samples delay max, 64 QMF channels */
    complex_t delay_SubQmf[2][32]; /* 2 samples delay max (SubQmf is always allpass filtered) */
    complex_t delay_Qmf_ser[NO_ALLPASS_LINKS][5][64]; /* 5 samples delay max (table 8.34), 64 QMF channels */
    complex_t delay_SubQmf_ser[NO_ALLPASS_LINKS][5][32]; /* 5 samples delay max (table 8.34) */

    /* transients */
    real_t alpha_decay;
    real_t alpha_smooth;

    real_t P_PeakDecayNrg[34];
    real_t P_prev[34];
    real_t P_SmoothPeakDecayDiffNrg_prev[34];

    /* mixing and phase */
    complex_t h11_prev[50];
    complex_t h12_prev[50];
    complex_t h21_prev[50];
    complex_t h22_prev[50];
    uint8_t phase_hist;
    complex_t ipd_prev[20][2];
    complex_t opd_prev[20][2];

} ps_info;

unsigned short ps_data(ps_info *ps, BitStreamInfo *bsi, uint8_t *header);

/* ps_dec.c */
ps_info *ps_init(uint8_t sr_index, uint8_t numTimeSlotsRate);
uint8_t ps_decode(ps_info *ps, qmf_t X_left[38][64], qmf_t X_right[38][64]);
void ps_free(ps_info *ps);
#endif
