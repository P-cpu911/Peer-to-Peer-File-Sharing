#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(){
    FILE* src = fopen("./testpic.png", "rb");
    if (!src) {
        perror("fopen src");
        return 1;
    }

    FILE* dest = fopen("./testpic2.png", "wb"); 
    if (!dest) {
        perror("fopen dest");
        fclose(src);
        return 1;
    }

    unsigned char buf[1024];
    size_t bytesRead;

    while ((bytesRead = fread(buf, 1, sizeof(buf), src)) > 0) {
        fwrite(buf, 1, bytesRead, dest); 
    }

    if (ferror(src)) {
        printf("Read error\n");
    }

    if (ferror(dest)) {
        printf("Write error\n");
    }

    fclose(src);
    fclose(dest);

    printf("File copied successfully!\n");
    return 0;
}
