#include <stdio.h>
#include <stdlib.h>
      
int main(void){
	FILE *fp;
	char *str;
	int  buff_size;
	   
	buff_size = 1024;            

	if(fp = fopen("./README", "r"))
	{
		str = malloc(buff_size+5);     

		while(fgets( str, buff_size, fp))
			printf( "%s", str);

		fclose(fp);
		free(str);
	}

	return 0;
}

