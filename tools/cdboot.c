/*
 * A tool for creating bootable CD (ISO-9660) image.
 */


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "cdboot.h"
#include "util.h"


/*
 * Date & Time functions
 */

void null_tm(date_time_t *tm_p)
{
  memset(tm_p->year_d, '\000', 4);
  memset(tm_p->month_d, '\000', 2);
  memset(tm_p->day_d, '\000', 2);
  memset(tm_p->hour_d, '\000', 2);
  memset(tm_p->minute_d, '\000', 2);
  memset(tm_p->second_d, '\000', 2);
  memset(tm_p->hsec_d, '\000', 2);
  memset(tm_p->qoff_d, '\000', 2);
}

void null_stm(sht_date_time_t *stm_p)
{
  stm_p->year = '\000';
  stm_p->month = '\000';
  stm_p->day = '\000';
  stm_p->hour = '\000';
  stm_p->minute = '\000';
  stm_p->second = '\000';
  stm_p->qoff = '\000';
}

/*
 * Sector functions.
 */
 
void null_sec(raw_sector_t *sec)
{
  memset(sec, '\000', ISO_SEC_LEN);
}

static inline void append_sec(FILE *file, raw_sector_t sec)
{
  int cnt;
  cnt = fwrite(sec, ISO_SEC_LEN, 1, file);
  if (cnt != 1)
    die("Error writing file.");
}

void write_ini_sec(FILE *file)
{
  raw_sector_t sec;
  null_sec(&sec);
  for (int i = 0; i < ISO_UNUSED_INI_SECS; i++)
    append_sec(file, sec);
}


/*
 * Dir descriptors
 */
void create_rootdir_desc(rootdir_desc_t *rootdir_p,
                       int loc_ext,
                       int dat_len)
{
  rootdir_p->len_dr = 34;
  rootdir_p->ext_attr = (char)0;
  both_byte_order32(rootdir_p->loc_ext, loc_ext);
  both_byte_order32(rootdir_p->dat_len, dat_len);
  null_stm(&rootdir_p->rec_stm);
  rootdir_p->file_flags = FFLG_DIR;
  rootdir_p->unit_size = (char)0;
  rootdir_p->ilea_gap_size = (char)0;
  both_byte_order16(rootdir_p->vol_seq_no, (short int)1); /* 1 assumed. */
  rootdir_p->len_fi = 1;
  rootdir_p->fi[0] = '\000';
}

static inline void rootdir_desc2sec(raw_sector_t *sec, rootdir_desc_t *dir, int boffset)
{
  memcpy((char*)sec + boffset, dir, sizeof(rootdir_desc_t));
}


/*
 * Vol. descriptors.
 */
void create_basic_pri_vdesc(pri_vdesc_t *sec_p,
                            int vol_space_size, /* A number of logical blocks */
                            int path_tab_size,
                            int occ_l_path_tab,
                            int occ_m_path_tab,
                            int root_loc_ext, /* for root dir entry... */
                            int root_dat_len) /* (see above) */
{
  sec_p->vdesc_type = 1;
  strncpy(sec_p->std_id, "CD001", 5);
  sec_p->ver_no = 1;
  sec_p->reserved0 = 0;
  strncpy(sec_p->syst_id, "CDBoot                          ", 32);
  strncpy(sec_p->vol_id, "CDBoot_Vol1                     ", 32);
  memset(&sec_p->reserved1[0], '\000', 8);
  both_byte_order32(sec_p->vol_space_size, vol_space_size);
  memset(&sec_p->reserved2[0], '\000', 32);
  both_byte_order16(sec_p->vol_set_size, (short int)1); /* 1 assumed */
  both_byte_order16(sec_p->vol_seq_num, (short int)1); /* 1 assumed */
  both_byte_order16(sec_p->log_blk_size, PRIVDESC_LOG_BLK);
  both_byte_order32(sec_p->path_tab_size, path_tab_size);
  lsb_byte_order32(sec_p->occ_l_path_tab, occ_l_path_tab);
  memset(sec_p->opt_l_path_tab, '\000', 4);
  lsb_byte_order32(sec_p->occ_m_path_tab, occ_m_path_tab);
  memset(sec_p->opt_m_path_tab, '\000', 4);
  create_rootdir_desc(&sec_p->rootdir_desc, root_loc_ext, root_dat_len);
  memset(sec_p->vol_set_id, '\000', 128);
  strncpy(sec_p->vol_set_id, "CDBoot_ID1                      ", 32);
  memset(sec_p->publ_id, ' ', 128);
  memset(sec_p->dat_prep_id, ' ', 128);
  memset(sec_p->app_id, '\000', 128);
  memset(sec_p->cprght_file_id, ' ', 37);
  memset(sec_p->abstr_file_id, ' ', 37);
  memset(sec_p->bibl_file_id, ' ', 37);
  null_tm(&sec_p->vol_creat_tm);
  null_tm(&sec_p->vol_mod_tm);
  null_tm(&sec_p->vol_exp_tm);
  null_tm(&sec_p->vol_eff_tm);
  sec_p->file_st_ver = 1;
  sec_p->reserved3 = 0;
  memset(sec_p->app_use, '\000', 512);
  memset(sec_p->reserved4, '\000', 653);
}

void create_boot_rec_vdesc(boot_rec_vdesc_t *sec_p,
                           int fsec_dir)
{
  static const char *eltoro = "EL TORITO SPECIFICATION";
  
  sec_p->vdesc_type = 0;
  strncpy(sec_p->std_id, "CD001", 5);
  sec_p->ver_no = 1;
  memset(sec_p->boot_syst_id, '\000', 32);
  strncpy(sec_p->boot_syst_id, eltoro, strlen(eltoro));
  memset(sec_p->boot_id, '\000', 32);
  lsb_byte_order32(sec_p->fsec_dir, fsec_dir); /* LSB order */
  memset(sec_p->reserved0, '\000', 1973);
}

void create_vdesc_set_term(vdesc_set_term_t *sec_p)
{
  sec_p->vdesc_type = -1;
  strncpy(sec_p->std_id, "CD001", 5);
  sec_p->ver_no = 1;
  memset(sec_p->reserved0, '\000', 2041);
}

static inline void boot_rec_vdesc2sec(raw_sector_t *sec, boot_rec_vdesc_t *boot_rec_vdesc)
{
  memcpy(sec, boot_rec_vdesc, ISO_SEC_LEN);
}

static inline void pri_vdesc2sec(raw_sector_t *sec, pri_vdesc_t *pri_vdesc)
{
  memcpy(sec, pri_vdesc, ISO_SEC_LEN);
}

static inline void vdesc_set_term2sec(raw_sector_t *sec, vdesc_set_term_t *vdesc_set_term)
{
  memcpy(sec, vdesc_set_term, ISO_SEC_LEN);
}


/*
 * Entries.
 */
void init_valid_ent(valid_ent_t *ent_p)
{
  short int chksum = 0x0000;
  short int *i_p = (short int*)ent_p;
  int i;
  int max_i = sizeof(valid_ent_t) / sizeof(short int);
  ent_p->head_id = 0x01;
  ent_p->platform = PLATFORM_X86;
  memset(ent_p->reserved0, '\000', 2);
  memset(ent_p->manf_id, '\0', sizeof(ent_p->manf_id));
  strncpy(ent_p->manf_id, "CDBoot", 6);
  lsb_byte_order16(ent_p->chksum, 0x0000);
  ent_p->mag0 = 0x55;
  ent_p->mag1 = 0xaa;
  for(i = 0; i < max_i; i++)
  {
    chksum -= *i_p;
    i_p++;
  }
  lsb_byte_order16(ent_p->chksum, chksum);
}

void init_inidef_ent(inidef_ent_t *ent_p,
                     char media_type,
                     short int load_seg,
                     char sys_type,
                     short int sec_count,
                     int load_rba)
{
  ent_p->boot_ind = 0x88; /* 0x88="bootable", 0x00="non bootable" */
  ent_p->media_type = media_type;
  lsb_byte_order16(ent_p->load_seg, load_seg);
  ent_p->sys_type = sys_type; /* A copy of byte 5 from the Partition Table in boot image. */
  ent_p->res0 = '\000';
  lsb_byte_order16(ent_p->sec_count, sec_count);
  lsb_byte_order32(ent_p->load_rba, load_rba);
  memset(ent_p->res1, '\000', 20);
}

static inline void valid_ent2sec(raw_sector_t *sec_p, valid_ent_t *ent_p, int offset)
{
  memcpy((char*)sec_p + sizeof(valid_ent_t) * offset, ent_p, sizeof(valid_ent_t));
}

static inline void inidef_ent2sec(raw_sector_t *sec_p, inidef_ent_t *ent_p, int offset)
{
  memcpy((char*)sec_p + sizeof(inidef_ent_t) * offset, ent_p, sizeof(inidef_ent_t));
}

/*
 * Boot catalog
 */
void write_boot_catalog(FILE *destf,
                        char media_type,
                        char system_type,
                        short int sec_count,
                        int load_rba)
{
  valid_ent_t vent;
  inidef_ent_t ient;
  raw_sector_t sec;
  
  null_sec(&sec);
  
  init_valid_ent(&vent);
  valid_ent2sec(&sec, &vent, 0);
  
  init_inidef_ent(&ient, media_type & 0x0f, 0, system_type, sec_count, load_rba); /* seg. 07C0:0000 assumed */
  inidef_ent2sec(&sec, &ient, 1);

  append_sec(destf, sec);  
}



/*
 * File functions.
 */
 
void open_src_file(const char *fname, FILE **file_pp)
{
  *file_pp = fopen(fname, "r");
  if (*file_pp == NULL)
    die("Error #%d opening file", errno);
}

void open_dst_file(const char *fname, FILE **file_pp)
{
  *file_pp = fopen(fname, "a+");
  if (*file_pp == NULL)
    die("Error #%d opening file", errno);
}

long int get_file_length(FILE *f)
{
  long int curpos;
  long int flen;
  int res;
  curpos = ftell(f);
  res = fseek(f, 0, SEEK_END);
  if (res == -1)
    die("Error determining file length.");
  flen = ftell(f);
  res = fseek(f, curpos, SEEK_SET);
  if (res == -1)
    die("Error determining file length.");
  return flen;
}

/*
 * Set of Volume Descriptors.
 */
void write_vol_descs(FILE *destf,
                     int path_tab_size,
                     int occ_l_path_tab,
                     int occ_m_path_tab,
                     int vol_space_size,
                     int root_loc_ext,
                     int root_dat_len)
{
  pri_vdesc_t pri_vdesc_sec;
  boot_rec_vdesc_t boot_rec_vdesc_sec;
  vdesc_set_term_t vdesc_set_term_sec;
  raw_sector_t rsec;
  
  create_basic_pri_vdesc(&pri_vdesc_sec, vol_space_size, path_tab_size, occ_l_path_tab, occ_m_path_tab, root_loc_ext, root_dat_len);
  pri_vdesc2sec(&rsec, &pri_vdesc_sec);
  append_sec(destf, rsec);
  
  create_boot_rec_vdesc(&boot_rec_vdesc_sec, 19); /* preceeding are: 16 unused + 3 vol. descriptors. */
  boot_rec_vdesc2sec(&rsec, &boot_rec_vdesc_sec);
  append_sec(destf, rsec);
  
  create_vdesc_set_term(&vdesc_set_term_sec);
  vdesc_set_term2sec(&rsec, &vdesc_set_term_sec);
  append_sec(destf, rsec);
}


/*
 * Injecting boot image into CD image.
 */
void append_boot_image(FILE *srcf, FILE *destf)
{
  long int flen;
  long int secs;
  int lasts;
  int i;
  int res;
  raw_sector_t sec;
  
  flen = get_file_length(srcf);
  secs = flen / ISO_SEC_LEN;
  lasts = flen % ISO_SEC_LEN;
  
  clearerr(srcf);
  clearerr(destf);
  for (i = 0; i < secs; i++)
  {
    res = fread(&sec, ISO_SEC_LEN, 1, srcf);
    if (res < 1)
      die("Error reading source boot image file.\n");
    res = fwrite(&sec, ISO_SEC_LEN, 1, destf);
    if (res < 1)
      die("Error writing ISO image file.\n");
  }
  if (lasts > 0)
  {
    null_sec(&sec);
    res = fread(&sec, lasts, 1, srcf);
    if (res < 1)
      die("Error reading source boot image file.\n");
    res = fwrite(&sec, ISO_SEC_LEN, 1, destf);
    if (res < 1)
      die("Error writing ISO image file.\n");
  }
}


/*
 * Path Table Record
 */

void create_path_tab_rec(int byte_order, path_tab_rec_t *rec_p,
                         int loc_ext,
                         short int updir_no,
                         const char *dir_id)
{
  rec_p->len_di = LEN_DI; /* As it should always be LEN_DI... */
  rec_p->xtattr_len = 0; /* 0 assumed = no extended attribute record. */
  switch (byte_order)
  {
    case LSB_ORDER:
      lsb_byte_order32(rec_p->loc_ext, loc_ext);
      lsb_byte_order16(rec_p->updir_no, updir_no);
      break;
    case MSB_ORDER:
      msb_byte_order32(rec_p->loc_ext, loc_ext);
      msb_byte_order16(rec_p->updir_no, updir_no);
      break;
    default:
      memset(rec_p->loc_ext, '\000', 4);
      memset(rec_p->updir_no, '\000', 2);
      break;
  }
  memset(rec_p->dir_id, '\000', sizeof(rec_p->dir_id));
  strncpy(rec_p->dir_id, dir_id, LEN_DI);
}

void create_root_path_tab_rec(int byte_order, root_path_tab_rec_t *rec_p,
                              int loc_ext)
{
  rec_p->len_di = 1; /* Root dir ID should always be of 1-byte size. */
  rec_p->xtattr_len = 0; /* 0 assumed = no extended attribute record. */
  switch (byte_order)
  {
    case LSB_ORDER:
      lsb_byte_order32(rec_p->loc_ext, loc_ext);
      lsb_byte_order16(rec_p->updir_no, (short int)1); /* This should always point to the root dir. */
      break;
    case MSB_ORDER:
      msb_byte_order32(rec_p->loc_ext, loc_ext);
      msb_byte_order16(rec_p->updir_no, (short int)1); /* This should always point to the root dir. */
      break;
    default:
      memset(rec_p->loc_ext, '\000', 4);
      memset(rec_p->updir_no, '\000', 2);
      break;
  }
  memset(rec_p->dir_id, '\000', 1);
  rec_p->pad = 0;
}

void path_tab_rec2sec(raw_sector_t *sec_p, path_tab_rec_t *rec_p, int boffset)
{
  memcpy((char*)sec_p + boffset, rec_p, sizeof(path_tab_rec_t));
}

void root_path_tab_rec2sec(raw_sector_t *sec_p, root_path_tab_rec_t *rec_p, int boffset)
{
  memcpy((char*)sec_p + boffset, rec_p, sizeof(root_path_tab_rec_t));
}

/*
 * Set of Path Table records.
 */

void write_path_tabs(FILE *destf, int root_loc_ext)
{
  raw_sector_t sec;
  root_path_tab_rec_t rec;
  
  /* Path Table - type L */  
  null_sec(&sec);
  create_root_path_tab_rec(LSB_ORDER, &rec, root_loc_ext);
  root_path_tab_rec2sec(&sec, &rec, 0);
  append_sec(destf, sec);

  /* Path Table - type M */  
  null_sec(&sec);
  create_root_path_tab_rec(MSB_ORDER, &rec, root_loc_ext);
  root_path_tab_rec2sec(&sec, &rec, 0);
  append_sec(destf, sec);  
}

/*
 * Root directory extent
 */
void write_rootdir_ext(FILE *destf,
                        int root_loc_ext)
{
  int root_dat_len = 2 * sizeof(rootdir_desc_t); /* "." and ".." dirs only... */
  raw_sector_t sec;
  rootdir_desc_t dir;
  create_rootdir_desc(&dir, root_loc_ext, root_dat_len);
  rootdir_desc2sec(&sec, &dir, 0);
  dir.fi[0] = (char)0x01; /* Root directory as "..". */
  rootdir_desc2sec(&sec, &dir, sizeof(rootdir_desc_t));
  append_sec(destf, sec);
}

/*
 * CD ISO image structure is:
 *
 *                          [sector length = logical block size = 2048]
 * Sector  0   (empty)
 * Sector  1   (empty)
 * ...
 * Sector 15   (empty)
 * Sector 16 - Primary Volume Descriptor
 * Sector 17 - Boot Record Volume Descriptor
 * Sector 18 - Volume Descriptor Set Terminator
 * Sector 19 - Boot catalog:                        [entry length = 32]
 *               -- Validation Entry
 *               -- Initial/Default Entry
 *               --
 * Sector 20 - Path Table "L"         [assumed dir record length = 128]
 *               -- (1) root directory
 *               -- 
 * Sector 21 - Path Table "M"
 *               -- (1) root directory
 *               -- 
 * Sector 22 - Root dir extent
 *               -- (1) root directory .
 *               -- (2) root directory ..
 *               -- 
 * Sector 23 - (boot image)
 * Sector 24 - (boot image)
 * ...
 *            
 *
 */


/*
 * Main
 */
int main(int args, char *argv[])
{
  FILE *srcf;
  FILE *destf;
  long int flen;
  short int sec_count; // Boot image length in emulated device sectors.
  int iso_sec_count; // Boot image length in CD(ISO) sectors.
  const int path_tab_size = 1 * sizeof(root_path_tab_rec_t);
  
  if (args < 3)
    die("Usage: cdboot <SRC_IMG> <DEST_ISO_IMG> [<options...>]\n");
  
  
  open_src_file(argv[1], &srcf);
  open_dst_file(argv[2], &destf);

  flen = get_file_length(srcf);
  sec_count = flen / SEC_LEN + ((flen % SEC_LEN > 0) ? 1 : 0);
  iso_sec_count = flen / ISO_SEC_LEN + ((flen % ISO_SEC_LEN > 0) ? 1 : 0);
  
  write_ini_sec(destf); /* Sectors 0 - 15 */
  write_vol_descs(destf, path_tab_size, 20, 21, iso_sec_count + 23,
                  22, 2 * sizeof(rootdir_desc_t)); /* Sectors 16 - 18 */
  write_boot_catalog(destf, HRDDSK, 0, sec_count, 23); /* Sector 19 */
  write_path_tabs(destf, 22); /* Sectors 20 - 21 */
  write_rootdir_ext(destf, 22); /* Sector 22 */
  append_boot_image(srcf, destf); /* Sectors 23 - ? */

  fclose(destf);
  fclose(srcf);
  
  
  exit(EXIT_SUCCESS);
}
