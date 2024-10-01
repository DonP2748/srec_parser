#ifndef _SREC_PARSING_H_
#define _SREC_PARSING_H_


#define CODE_OK 			0x01
#define CODE_SYNX 			0x02
#define CODE_TYPE 			0x03
#define CODE_BYTECNT		0x04
#define CODE_CKSUM			0x05
#define CODE_LINECNT  		0x06

typedef struct file_t{
	uint8_t* lines; //error code
	uint32_t count; //line number
}file_t;


file_t* srec_parsing_file(char* name);
void srec_release_file(file_t* arg);

 #endif//_SREC_PARSING_H_
