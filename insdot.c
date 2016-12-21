#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#define DOTPOS 11
#define FNLEN 128
#define SBCH 64

int main(int argc, char *argv[]) {

	DIR *d;
	struct dirent *dir;
	d = opendir(argv[1]);

	char buf[SBCH];
	unsigned int fnlen = 0;
	unsigned int a = 0;

	if (!d) {
		return 1;
	} else {
		while((dir = readdir(d)) != NULL) {

			fnlen = strlen(dir->d_name);

			if (dir->d_name[DOTPOS] != '.') {
				for(a = 0; a < DOTPOS; a++) buf[a] = dir->d_name[a];
				buf[(DOTPOS)] = '.';
				for(a = DOTPOS + 1; a <= fnlen; a++) {
				buf[a] = dir->d_name[(a-1)];
				}

			rename(dir->d_name, buf);
			memset(buf, 0, SBCH);

			}
		}
		closedir(d);
	}
	return 0;
}
