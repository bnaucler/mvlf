#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#define VER 0.1

#define OPREF "ljusdal.log."
#define NPREF "ljusdal.log."

#define ERRSTR "ERROR"
#define DDIV '/'

#define DLEN 2
#define MLEN 3
#define YLEN 4

#define MBCH 256
#define SBCH 32
#define PDCH 10

char ms[12][4] = {
	"jan", "feb", "mar", "apr",
	"may", "jun", "jul", "aug",
	"sep", "oct", "nov", "dec"
};

char ml[12][10] = {
	"january", "february", "march", "april",
	"may", "june", "july", "august",
	"september", "october", "november", "december"
};

char ds[7][4] = {
	"mon", "tue", "wed", "thu",
	"fri", "sat", "sun"
};

char dl[7][10] = {
	"monday", "tuesday", "wednesday", "thursday",
	"friday", "saturday", "sunday"
};

int mnum(char *txmon) {

	int a = 0;
	int alen = sizeof(ms) / sizeof(ms[0]);

	for (a = 0; a < alen; a++) {
		if (strcasecmp(txmon, ms[a]) == 0) return a + 1;
	}

	return 0;
}

char *mknn(char *oname, char *nname) {

	int oplen = strlen(OPREF);
	char *buf = calloc(SBCH, sizeof(char));

	if (strlen(oname) != oplen + DLEN + MLEN + YLEN) return ERRSTR;

	memset(nname, 0, MBCH);

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

	if (d == 0 || m == 0 || y == 0) snprintf(nname, MBCH, ERRSTR);
	else snprintf(nname, MBCH, "%s%04d%02d%02d", NPREF, y, m, d);

	free(buf);
	return nname;
}

int usage(char *cmd, char *err, int ret) {

	if (strlen(err) > 0) printf("ERROR: %s\n", err);
	printf("REName LOGfiles v%.1f\n", VER);
	printf("Usage: %s [-hiopf] [dir]\n", cmd);
	printf("	-h: Show this text\n");
	printf("	-i: Input pattern\n");
	printf("	-o: Output pattern\n");
	printf("	-p: Input prefix\n");
	printf("	-r: Output prefix\n");

	exit(ret);
}

int main(int argc, char *argv[]) {

	DIR *d;
	struct dirent *dir;

	char *dpath = calloc(MBCH, sizeof(char));
	char *oname = calloc(MBCH, sizeof(char));
	char *nname = calloc(MBCH, sizeof(char));
	char *buf = calloc(MBCH, sizeof(char));

	char *ipat = calloc(PDCH + 1, sizeof(char));
	char *opat = calloc(PDCH + 1, sizeof(char));

	char *ipref = calloc(MBCH, sizeof(char));
	char *opref = calloc(MBCH, sizeof(char));

	unsigned int a = 0;
	int plen = strlen(OPREF);
	int optc;

	while((optc = getopt(argc, argv, "hi:o:p:r:")) != -1) {
		switch (optc) {

			case 'h':
				usage(argv[0], "", 0);
				break;

			case 'i':
				strncpy(ipat, optarg, PDCH);
				break;

			case 'o':
				strncpy(opat, optarg, PDCH);
				break;

			case 'p':
				strncpy(ipref, optarg, MBCH);
				break;

			case 'r':
				strncpy(opref, optarg, MBCH);
				break;

			default:
				usage(argv[0], "", 1);
				break;
		}
	}

	if (argc > optind + 1) {
		usage(argv[0], "Too many arguments", 1);
	} else if (argc > optind) { 
		strcpy(dpath, argv[1]);
	} else {
		dpath = getcwd(dpath, MBCH);
	}

	if (dpath[strlen(dpath) - 1] != DDIV) dpath[strlen(dpath)] = DDIV;
	d = opendir(dpath);

	if (!d) {
		usage(argv[0], "Could not read directory", 1);

	} else {
		while((dir = readdir(d)) != NULL) {

			for (a = 0; a < plen; a++) {
				if (dir->d_name[a] != OPREF[a]) break;
				if (a == plen - 1) {
					snprintf(oname, MBCH, "%s%s", dpath, dir->d_name);
					snprintf(nname, MBCH, "%s%s", dpath, mknn(dir->d_name, buf));
					if (strcasecmp(nname, ERRSTR) == 0) {
						printf("Error: could not rename %s\n", oname);
					} else {
						// rename(dir->d_name, nname);
						printf("%s\t%s\n", oname, nname);
					}
				}
			}
		}
		closedir(d);
	}

	return 0;
}
