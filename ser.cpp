#include <stdint.h>
#include <byteswap.h>

char name[14] = {0x4c, 0x55, 0x43, 0x41, 0x4d, 0x2d, 0x52, 0x45, 0x43, 0x4f, 0x52, 0x44, 0x45, 0x52};


void write_header(FILE *f, int width, int height, int cnt)
{
	uint32_t tmp;	

	fseek(f, 0, SEEK_SET);
//0
	fwrite(name, 14, 1, f);
//14	
	tmp = 0;
	fwrite(&tmp, 1, sizeof(tmp), f); //camera type
//18
	tmp = 0;
	fwrite(&tmp, 1, sizeof(tmp), f); //mono
//22
 	tmp = 0;
	fwrite(&tmp, 1, sizeof(tmp), f); //endianess	
//26	
	tmp = (width);
	fwrite(&tmp, 1, sizeof(tmp), f); //width
//30	
	tmp = (height);
	fwrite(&tmp, 1, sizeof(tmp), f); //height

	tmp = 16;
	fwrite(&tmp, 1, sizeof(tmp), f); //depth

//38

	tmp = cnt; //frame count
	fwrite(&tmp, 1, sizeof(tmp), f); //count
	
	tmp = 0;

	for (int i = 0; i < (120/4); i++) 
		fwrite(&tmp, 1, sizeof(tmp), f);	//useless strings

//158

	for (int i = 0; i < (16/4); i++)
		fwrite(&tmp, 1, sizeof(tmp), f);	//dates

}


