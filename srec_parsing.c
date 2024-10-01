#pragma pack (1)

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "srec_parsing.h"

#define MAX_LENGTH (1 + 2 + 255)
#define ADDR_SZ  	4
#define INVALID  	0xFF

#define STATE_HDR   0x02
#define STATE_DATA 	0x03
#define STATE_CNT   0x04
#define STATE_ADDR  0x05
#define STATE_NONE  0x06

#define STYLE_S19 	0x02
#define STYLE_S28 	0x03
#define STYLE_S37 	0x04

static uint8_t end_proc = false;
static uint8_t buffer[MAX_LENGTH] = {0};
static file_t _file = {0};
static file_t* file = &_file;

static uint8_t hexCharToValue(char c);
static uint8_t hexStringToValue(char *buff,uint16_t strlen) ;
static uint8_t parse_line(uint8_t* data);
static uint8_t check_sum(uint8_t* data,uint8_t len);
static uint8_t check_record_type(uint8_t type,uint32_t* count);
static void example_write_flash_data(uint8_t* data,uint16_t len);


 file_t* srec_parsing_file(char* name)
 {
 	//check name path
 	FILE* fd = fopen(name,"r");
 	if(fd == NULL){
 		return NULL;
 	}
 	//get line number
	while(fgetc(fd) != EOF){
		fgets((char*)buffer,MAX_LENGTH,fd);
		file->count++;
	}
	rewind(fd);
	//alloc
	file->lines = (uint8_t*)calloc(file->count,sizeof(uint8_t));
	if(file->lines == NULL){
		fclose(fd);
		return NULL;
	}
	//parse line
	uint8_t err = 0;
	for(uint32_t i = 0; i < file->count; i++){
		fgets((char*)buffer,MAX_LENGTH,fd);
		err = parse_line(buffer);
		*(file->lines+i) = err;
		if(err != CODE_OK){
			fclose(fd);
			return file;
		}
	}
	if(!end_proc){
		return NULL;
	}
	end_proc = false;
	fclose(fd);
	return file;
 }


void srec_release_file(file_t* arg)
{
	if(arg){
		if(arg->lines){
			free(arg->lines);
			arg->lines = NULL;
		}
	}
}


static uint8_t parse_line(uint8_t* data)
{
	uint32_t data_lines = 0;
	//check start of record
	if(data[0] != 'S'){
		return CODE_SYNX;
	}
	//check record type, style s19 s28 s37 and its order
	uint8_t state = check_record_type(data[1],&data_lines);
	if(!state){
		return CODE_TYPE; 
	}
	//convert str to value and check string valid 
	uint16_t len = hexStringToValue((char*)&data[2],strlen((char*)&data[2]));
	if(!len){
		return CODE_SYNX;
	}
	//check byte count, len include both byte count and checksum so decrease 1 to remove bytecount
	if(data[2] != len-1){
		return CODE_BYTECNT;
	}
	//check sum
	//len from data[2] to data[len-2]
	//remove checksum -1, 2 byte 'S' and type +2  
	if(data[len-1+2] != check_sum(&data[2],len-1)){
		return CODE_CKSUM;
	}
	//Write data to flash or use data to do something else
	if((state == STATE_DATA)){ //not header, not count, not start address, just data field 
		example_write_flash_data(&data[3],len-2);
	}
	//after data field, then verify data count s5,s6 
	if(state == STATE_CNT){
		uint32_t count = 0;
		for(uint8_t i = 0;i < len-2;i++){
			count |= data[3+i] << 8*(len-3-i);
		}
		if(count != data_lines){
			return CODE_LINECNT;
		}
	}
	return CODE_OK;
}


static void example_write_flash_data(uint8_t* data,uint16_t len)
{
	for(uint16_t i = 0;i < len;i++){
		printf("%02X",data[i]);
	}
	printf("\n");
} 


static uint8_t hexCharToValue(char c) 
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'A' && c <= 'F') return 10 + c - 'A';
	if (c >= 'a' && c <= 'f') return 10 + c - 'a';
	return INVALID; // Not a valid hexadecimal character
}

static uint8_t hexStringToValue(char *buff,uint16_t str_len) 
{
	if(!(str_len%2) & (buff[str_len-1] != '\n')) //wrong string input
		return 0;
	uint8_t len = (str_len-1)/2;
	uint8_t* temp = (uint8_t*)calloc(len,1);
	if(!temp) return 0;
	uint8_t low = 0,high = 0;
	for(uint16_t i = 0;i < len;i++){
		high = hexCharToValue(*(buff+i*2))<<4;
		low  = hexCharToValue(*(buff+i*2+1));
		if((high == INVALID)|(low == INVALID)){
			return 0;
		}
		*(temp+i) |= high|low;
	}
	memcpy((uint8_t*)buff,temp,len);
	free(temp);
	return len;
}


static uint8_t check_record_type(uint8_t type,uint32_t* count)
{
	uint8_t temp = 0;
	static uint8_t parse_status = STATE_NONE;//for check order file
	static uint8_t pre_type = 0;  //for check style s19 s28 s37
	static uint32_t data_cnt = 0; //for check count (s5,s6 type)
	switch(type){
		case '0':
			//case 0 use for check order, style and count  
			data_cnt = 0;
			pre_type = 0;
			if((parse_status>=STATE_HDR)&(parse_status<STATE_ADDR))
				return 0;
			parse_status = STATE_HDR;
			return parse_status;
		case '5':
		case '6':	
			//case 5,6 use for check order, count 
			if(parse_status != STATE_DATA)
				return 0;
			parse_status = STATE_CNT; //update state
			*count = data_cnt; 		 //set count
			return parse_status;
		case '1':
			//case 1 use for check order, style(when exit switch) and count increase
			if(parse_status > STATE_DATA)
				return 0;
			data_cnt++; //increase data count
			parse_status = STATE_DATA;
			temp = STYLE_S19; //set style
			break;
		case '9':
			//case 9 use for check order, style(when exit switch) and count return
			if((parse_status!=STATE_DATA)&(parse_status!=STATE_CNT))
				return 0;
			temp = STYLE_S19;
			*count = data_cnt;
			parse_status = STATE_ADDR;
			end_proc = true; //end file processing, have all type necessary 
			break;
		case '2':
			//case 2 use for check order, style(when exit switch) and count increase
			if(parse_status > STATE_DATA)
				return 0;
			data_cnt++;
			parse_status = STATE_DATA;
			temp = STYLE_S28;
			break;
		case '8':
			//case 8 use for check order, style(when exit switch) and count return
			if((parse_status!=STATE_DATA)&(parse_status!=STATE_CNT))
				return 0;
			temp = STYLE_S28;
			*count = data_cnt; 
			parse_status = STATE_ADDR;
			end_proc = true;
			break;
		case '3':
			//case 3 use for check order, style(when exit switch) and count increase
			if(parse_status > STATE_DATA)
				return 0;
			data_cnt++;
			parse_status = STATE_DATA;
			temp = STYLE_S37;
			break;
		case '7':	
			//case 7 use for check order, style(when exit switch) and count return
			if((parse_status!=STATE_DATA)&(parse_status!=STATE_CNT))
				return 0;
			temp = STYLE_S37;
			*count = data_cnt;
			parse_status = STATE_ADDR;
			end_proc = true;
			break;
		default:
			pre_type = 0;
			data_cnt = 0;
			parse_status = STATE_NONE;
			return 0;
	}
	if(!pre_type){
		pre_type = temp;
	}
	return (pre_type != temp) ? 0 : parse_status;
}



static uint8_t check_sum(uint8_t* data,uint8_t len)
{
	uint8_t sum = 0;
	for(uint8_t i = 0;i < len;i++){
		sum += data[i];
	}
	return (0xFF-sum);
}









