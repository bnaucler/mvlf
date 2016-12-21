#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#define OPREF "ljusdal.log."
#define NPREF "ljusdal.log."

#define DLEN 2
#define MLEN 3
#define YLEN 4

#define FNCH 128
#define SBCH 32

int dtnum(char *txmon) {

	if (strcasecmp(txmon, "jan") == 0) return 1;
	else if (strcasecmp(txmon, "feb") == 0) return 2;
	else if (strcasecmp(txmon, "mar") == 0) return 3;
	else if (strcasecmp(txmon, "apr") == 0) return 4;
	else if (strcasecmp(txmon, "may") == 0) return 5;
	else if (strcasecmp(txmon, "jun") == 0) return 6;
	else if (strcasecmp(txmon, "jul") == 0) return 7;
	else if (strcasecmp(txmon, "aug") == 0) return 8;
	else if (strcasecmp(txmon, "sep") == 0) return 9;
	else if (strcasecmp(txmon, "oct") == 0) return 10;
	else if (strcasecmp(txmon, "nov") == 0) return 11;
	else if (strcasecmp(txmon, "dec") == 0) return 12;
	else return 0;
}

char *mknn(char *oname, char *nname) {

	int oplen = strlen(OPREF);
	char *buf = calloc(SBCH, sizeof(char));

	if (strlen(oname) != oplen + DLEN + MLEN + YLEN) return "ERROR";

	memset(nname, 0, FNCH);

	int a = 0;
	int y = 0, m = 2, d = 0;

	for (a = 0; a < DLEN; a++) { buf[a] = oname[(oplen + a)]; }
	d = atoi(buf);
	memset(buf, 0, SBCH);

	for (a = 0; a < MLEN; a++) { buf[a] = oname[(oplen + DLEN + a)]; }
	m = dtnum(buf);
	memset(buf, 0, SBCH);

	for (a = 0; a < YLEN; a++) { buf[a] = oname[(oplen + DLEN + MLEN + a)]; }
	y = atoi(buf);

	if (d == 0 || m == 0 || y == 0) snprintf(nname, FNCH, "ERROR");
	else snprintf(nname, FNCH, "%s%04d%02d%02d", NPREF, y, m, d);

	free(buf);
	return nname;
}

int main(int argc, char *argv[]) {

	if (argc != 2) return 1;

	DIR *d;
	struct dirent *dir;
	d = opendir(argv[1]);

	unsigned int a = 0;
	int plen = strlen(OPREF);
	char *nname = calloc(FNCH, sizeof(char));

	if (!d) {
		return 1;
	} else {
		while((dir = readdir(d)) != NULL) {

			for (a = 0; a < plen; a++) {
				if (dir->d_name[a] != OPREF[a]) break;
				if (a == plen - 1) {
					nname = mknn(dir->d_name, nname);
					rename(dir->d_name, nname);
				}
			}
		}
		closedir(d);
	}

	return 0;
}
