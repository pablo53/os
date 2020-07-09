#ifndef _CDBOOT_H
#define _CDBOOT_H 1


/*
 * Abstract ISO-CD sector
 */
#define ISO_SEC_LEN 0x800
typedef char raw_sector_t[ISO_SEC_LEN];

/*
 * ISO-9660 related constants.
 */
#define ISO_UNUSED_INI_SECS 0x10

/*
 * Date and Time Format
 */
typedef struct
{
  char year_d[4];
  char month_d[2];
  char day_d[2];
  char hour_d[2];
  char minute_d[2];
  char second_d[2];
  char hsec_d[2];
  char qoff_d[1]; /* Greenwich offset in quarters of hour. */
} date_time_t;

typedef struct
{
  char year;
  char month;
  char day;
  char hour;
  char minute;
  char second;
  char qoff; /* Greenwich offset in quarters of hour. */
} sht_date_time_t;


/*
 * File and Directory Descriptor
 */
#define LEN_DR 128
#define LEN_FI 37

#define FFLG_EXIST 1
#define FFLG_DIR 2
#define FFLG_AFIL 4
#define FFLG_REC 8
#define FFLG_PROT 16
#define FFLG_MULEXT 128

typedef struct
{
  char len_dr; /* Must be LEN_DR. */
  char ext_attr;
  char loc_ext[8];
  char dat_len[8];
  date_time_t rec_tm;
  char file_flags;
  char unit_size;
  char ilea_gap_size;
  char vol_seq_no[4];
  char len_fi; /* Must be LEN_FI. */
  char fi[LEN_FI];
  char reserved0[LEN_DR - LEN_FI - 33]; /* Must be 0's. */
} dir_desc_t;

typedef struct
{
  unsigned char len_dr; /* Must be 34. */
  char ext_attr;
  char loc_ext[8];
  char dat_len[8];
  sht_date_time_t rec_stm;
  char file_flags;
  char unit_size;
  char ilea_gap_size;
  char vol_seq_no[4];
  char len_fi; /* Must be 1. */
  char fi[1];
} rootdir_desc_t;


/*
 * Primary Volume Descriptor
 */
#define PRIVDESC_LOG_BLK 0x0800
typedef struct
{
  char vdesc_type; /* Must be 1. */
  char std_id[5]; /* Must be "CD001" (without terminating '\000'). */
  char ver_no; /* Must be 1. */
  char reserved0; /* Must be '\000'. */
  char syst_id[32]; /* System identifier. */
  char vol_id[32]; /* Volume identifier. */
  char reserved1[8]; /* Must be 0's. */
  char vol_space_size[8]; /* Number of Logical Blocks. */
  char reserved2[32]; /* Must be 0's. */
  char vol_set_size[4];
  char vol_seq_num[4];
  char log_blk_size[4]; /* Must be PRIVDESC_LOG_BLK = 2^(9+n). */
  char path_tab_size[8];
  char occ_l_path_tab[4];
  char opt_l_path_tab[4];
  char occ_m_path_tab[4];
  char opt_m_path_tab[4];
  rootdir_desc_t rootdir_desc;
  char vol_set_id[128];
  char publ_id[128];
  char dat_prep_id[128];
  char app_id[128];
  char cprght_file_id[37];
  char abstr_file_id[37];
  char bibl_file_id[37];
  date_time_t vol_creat_tm;
  date_time_t vol_mod_tm;
  date_time_t vol_exp_tm;
  date_time_t vol_eff_tm;
  char file_st_ver; /* Must be 1. */
  char reserved3; /* Must be 0. */
  char app_use[512];
  char reserved4[653]; /* Must be 0's. */
} pri_vdesc_t;

/*
 * Boot Record Volume Descriptor
 */
typedef struct
{
  char vdesc_type; /* Must be 0. */
  char std_id[5]; /* Must be "CD001" (without terminating '\000'). */
  char ver_no; /* Must be 1. */
  char boot_syst_id[32]; /* Must be "EL TORITO SPECIFICATION" with padding '\000's. */
  char boot_id[32]; /* Must be '\000's. */
  char fsec_dir[4]; /* A pointer to the first sector of the Boot Catalog. */
  char reserved0[1973]; /* Unused. Must be 0's. */
} boot_rec_vdesc_t;


/*
 * Volume Descriptor Set Terminator
 */
typedef struct
{
  char vdesc_type; /* Must be -1. */
  char std_id[5]; /* Must be "CD001" (without terminating '\000'). */
  char ver_no; /* Must be 1. */
  char reserved0[2041]; /* Unused. Must be 0's. */
} vdesc_set_term_t;


/*
 * Validation Entry
 */
#define PLATFORM_X86 0x00
#define PLATFORM_POWER_PC 0x01
#define PLATFORM_MAC 0x02

typedef struct
{
  char head_id; /* Must be 0x01. */
  char platform;
  char reserved0[2]; /* Must be 0's. */
  char manf_id[24];
  char chksum[2];
  unsigned char mag0; /* Must be 0x55. */
  unsigned char mag1; /* Must be 0xAA. */
} valid_ent_t;


/*
 * Initial/Default entry.
 */
 
#define BOOTABLE 0x88
#define NOTBOOTABLE 0x00

#define NOEMUL 0x00
#define DSK12 0x01
#define DSK144 0x02
#define DSK288 0x03
#define HRDDSK 0x04

#define SEC_LEN 512

typedef struct
{
  unsigned char boot_ind;
  char media_type;
  char load_seg[2];
  char sys_type;
  char res0; /* Reserved */
  char sec_count[2];
  char load_rba[4];
  char res1[20]; /* Reserved */
} inidef_ent_t;


/*
 * Path Table
 */
#define LEN_DI 24

typedef struct
{
  unsigned char len_di; /* Should always be LEN_DI. */
  unsigned char xtattr_len;
  char loc_ext[4];
  char updir_no[2];
  char dir_id[LEN_DI];
  /* Padding field is not necessary here. */
} path_tab_rec_t;

typedef struct
{
  unsigned char len_di; /* Should always be 1. */
  unsigned char xtattr_len;
  char loc_ext[4];
  char updir_no[2];
  char dir_id[1]; /* Should always be 0. */
  char pad; /* Padding field must be 0. */
} root_path_tab_rec_t;

#define LSB_ORDER 1
#define MSB_ORDER 2
#define BOTH_ORDER 3


#endif
