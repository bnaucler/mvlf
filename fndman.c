#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#define OPREF "ljusdal.log."
#define NPREF "ljusdal.log."

#define ERRSTR "ERROR"

#define DLEN 2
#define MLEN 3
#define YLEN 4

#define FNCH 128
#define FPCH 256
#define SBCH 32

char m[12][4] = {
	"jan", "feb", "mar", "apr",
	"may", "jun", "jul", "aug",
	"sep", "oct", "nov", "dec"
};

int mnum(char *txmon) {

	int a = 0;
	int alen = sizeof(m) / sizeof(m[0]);

	for (a = 0; a < alen; a++) {
		if (strcasecmp(txmon, m[a]) == 0) return a + 1;
	}

	return 0;
}

char *mknn(char *oname, char *nname) {

	int oplen = strlen(OPREF);
	char *buf = calloc(SBCH, sizeof(char));

	if (strlen(oname) != oplen + DLEN + MLEN + YLEN) return ERRSTR;

	memset(nname, 0, FNCH);

	int a = 0;
	int y = 0, m = 2, d = 0;

	for (a = 0; a < DLEN; a++) { buf[a] = oname[(oplen + a)]; }
	d = atoi(buf);
	memset(buf, 0, SBCH);

	for (a = 0; a < MLEN; a++) { buf[a] = oname[(oplen + DLEN + a)]; }
	m = mnum(buf);
	memset(buf, 0, SBCH);

	for (a = 0; a < YLEN; a++) { buf[a] = oname[(oplen + DLEN + MLEN + a)]; }
	y = atoi(buf);

	if (d == 0 || m == 0 || y == 0) snprintf(nname, FNCH, ERRSTR);
	else snprintf(nname, FNCH, "%s%04d%02d%02d", NPREF, y, m, d);

	free(buf);
	return nname;
}

int usage(char *cmd, char *err, int ret) {

	if (strlen(err) > 0) printf("ERROR: %s\n", err);
	printf("Usage: %s [dir]\n", cmd);

	exit(ret);
}

int main(int argc, char *argv[]) {

	DIR *d;
	struct dirent *dir;

	char *dpath = calloc(FPCH, sizeof(char));
	char *nname = calloc(FNCH, sizeof(char));

	unsigned int a = 0;
	int plen = strlen(OPREF);

	if (argc > 2) usage(argv[0], "", 1);
	else if (argc == 1) dpath = getcwd(dpath, FPCH);
	else {
		if (strlen(argv[1]) > FPCH) {
			usage(argv[0], "Your directory paths are messed up", 1);
		} else {
			strcpy(dpath, argv[1]);
		}
	}

	d = opendir(dpath);
	if (dpath[strlen(dpath) - 1] != '/') dpath[strlen(dpath)] = '/';

	if (!d) {
		usage(argv[0], "Could not read directory", 1);

	} else {
		while((dir = readdir(d)) != NULL) {

			for (a = 0; a < plen; a++) {
				if (dir->d_name[a] != OPREF[a]) break;
				if (a == plen - 1) {
					nname = mknn(dir->d_name, nname);
					if (strcasecmp(nname, ERRSTR) == 0) {
						// printf("Error: could not rename %s\n", dir->d_name);
					} else {
						// rename(dir->d_name, nname);
						printf("%s%s\t%s%s\n", 
								dpath, dir->d_name, dpath, nname);
					}
				}
			}
		}
		closedir(d);
	}

	return 0;
}
