#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(){
	FILE* src = fopen("./testpic.png", "rb");
	FILE* dest = fopen("./testpic2.png", "ab");
	unsigned char buf[1024];
	int eof = 0;
	while(eof == 0){
		fread(buf, 1, 1024, src);
		if(feof(src)){
			eof = 1;
		}
		else if(ferror(src)){
			printf("File (src) error\n");
			eof = 2;
		}
		else{
			//Nothing here. Just a placeholder
		}
		fwrite(buf, 1, 1024, dest);
		if(ferror(dest)){
			printf("File (dest) error\n");
			eof = 2;
		}
	}
	return 0;
}
