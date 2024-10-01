#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "srec_parsing.h"


char* get_error_code(uint8_t code)
{
	switch(code){
	case CODE_OK:
		return "TRUE FORM";
	case CODE_SYNX:
		return "SYNTAX ERROR";
	case CODE_TYPE:
		return "TYPE ERROR";
	case CODE_BYTECNT:
		return "BYTE COUNT ERROR";
	case CODE_CKSUM:
		return "CHECKSUM ERROR";
	case CODE_LINECNT:
		return "LINE COUNT ERROR";
	default:
		return NULL;
	}
}


int main(void)
{
	printf("Start File Parsing process\n");
	file_t* file = srec_parsing_file("test.srec");
	if(file){
		for(uint32_t i = 0;i < file->count;i++){
			printf("LINE %d - %s\n",i+1,get_error_code(*(file->lines+i)));
		}
	}
	else
		printf("Cannot Open File or File Corrupted!\n");
	srec_release_file(file);
	return 0;
}




