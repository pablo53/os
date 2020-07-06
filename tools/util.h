#ifndef _UTIL_H
#define _UTIL_H 1


void die(const char *, ...);
int parse_int(const char *);

void both_byte_order8(char* dst_buf, char val);
void both_byte_order16(char* dst_buf, short int val);
void both_byte_order32(char* dst_buf, int val);
void lsb_byte_order8(char* dst_buf, char val);
void lsb_byte_order16(char* dst_buf, short int val);
void lsb_byte_order32(char* dst_buf, int val);
void msb_byte_order8(char* dst_buf, char val);
void msb_byte_order16(char* dst_buf, short int val);
void msb_byte_order32(char* dst_buf, int val);

#endif
