/*

	renlog.c - Rename logfiles (with date format conversion)

	Björn Westerberg Nauclér (mail@bnaucler.se) 2016
	License: MIT

	Pattern syntax:
	Y = Year (1999, 2003...)
	y = Year shorthand (99, 03...)

	M = Month (January, February...)
	m = Month shorthand (Jan, Feb...)
	n = Month number (01, 02...)

	D = Day (Monday, Tuesday..)
	d = Day shorthand (Mon, Tue...)
	t = Date (31, 01...)

	Examples for matching patterns:
	[prefix.]2015-03-09	- Y-n-t
	[prefix.]Mon26Dec16 - dtmy

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <dirent.h>

#define VER 0.1

#define OPREF "ljusdal.log."
#define NPREF "ljusdal.log."

#define YL 'Y'
#define YS 'y'
#define ML 'M'
#define MS 'm'
#define MN 'n'
#define DL 'D'
#define DS 'd'
#define DT 't'

#define DDIV '/'
#define ERRSTR "ERROR"

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
		if (strcasecmp(txmon, ml[a]) == 0) return a + 1;
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

int usage(char *cmd, char *err, int ret, int verb) {

	if (strlen(err) > 0 && verb > -1) fprintf(stderr, "ERROR: %s\n", err);
	printf("REName LOGfiles v%.1f\n", VER);
	printf("Usage: %s [-dhioprtv] [dir]\n", cmd);
	printf("	-d: Output directory\n");
	printf("	-h: Show this text\n");
	printf("	-i: Input pattern\n");
	printf("	-o: Output pattern\n");
	printf("	-p: Input prefix\n");
	printf("	-r: Output prefix\n");
	printf("	-q: Quiet mode\n");
	printf("	-t: Test run\n");
	printf("	-v: Verbose mode\n");

	exit(ret);
}

int vpat(char *p) {

	int len = strlen(p);
	int hasy = 0, hasm = 0, hasd = 0;

	for(int a = 0; a < len; a++) {
		if((p[a] == YL || p[a] == YS) && hasy == 0) hasy++;
		else if((p[a] == ML || p[a] == MS) && hasm == 0) hasm++;
		else if((p[a] == DL || p[a] == DS) && hasd == 0) hasd++;
		else if(p[a] == DT) continue;
		else if (!isalpha(p[a]) && !isdigit(p[a])) continue;
		else return -1;
	}

	return 0;
}

int dtch(char *ipat, char *opat) {

	int ilen = strlen(ipat);
	int olen = strlen(opat);

	int ohas = 0;

	for(int a = 0; a < olen; a++) {
		if(opat[a] == DT) {
			ohas++;
			break;
		}
	}
	
	if (!ohas) return 0;

	for(int a = 0; a < ilen; a++) {
		if(ipat[a] == DT) return 0;
	}

	return 1;
}

int main(int argc, char *argv[]) {

	DIR *d;
	struct dirent *dir;

	// Can we reduce the number of string vars?
	char *ipath = calloc(MBCH, sizeof(char));
	char *opath = calloc(MBCH, sizeof(char));

	// Rename oname and nname for consistency
	char *oname = calloc(MBCH, sizeof(char));
	char *nname = calloc(MBCH, sizeof(char));

	char *buf = calloc(MBCH, sizeof(char));

	char *ipat = calloc(PDCH + 1, sizeof(char));
	char *opat = calloc(PDCH + 1, sizeof(char));

	char *ipref = calloc(MBCH, sizeof(char));
	char *opref = calloc(MBCH, sizeof(char));

	unsigned int a = 0;
	int plen = strlen(OPREF);
	int testr = 0, verb = 0;
	int optc;

	while((optc = getopt(argc, argv, "d:hi:o:p:r:t")) != -1) {
		switch (optc) {

			case 'd':
				strncpy(opath, optarg, PDCH);
				break;

			case 'h':
				usage(argv[0], "", 0, verb);
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

			case 'q':
				verb--;
				break;

			case 't':
				testr++;
				break;

			case 'v':
				verb++;
				break;

			default:
				usage(argv[0], "", 1, verb);
				break;
		}
	}

	// Input verification
	if (argc > optind + 1) {
		usage(argv[0], "Too many arguments", 1, verb);
	} else if (argc > optind) {
		strcpy(ipath, argv[1]);
	} else {
		ipath = getcwd(ipath, MBCH);
	}

	// Path formatting
	if (ipath[strlen(ipath) - 1] != DDIV) ipath[strlen(ipath)] = DDIV;
	d = opendir(ipath);

	// Validate patterns
	if (vpat(ipat) < 0) usage(argv[0], "Invalid input pattern", 1, verb);
	if (vpat(opat) < 0) usage(argv[0], "Invalid output pattern", 1, verb);
	if (strncmp(ipref, opref, MBCH) == 0 && 
			strncmp(ipat, opat, PDCH) == 0)
			usage(argv[0], "Input and output is exact match", 1, verb);
	if (dtch(ipat, opat) != 0)
		usage(argv[0], "Cannot make date numbers from thin air", 1, verb);

	// DEBUG
	printf("OK\n");
	exit(0);

	if (strlen(opath) == 0) strcpy(opath, ipath);
	else if (ipath[strlen(opath) - 1] != DDIV)
		ipath[strlen(opath)] = DDIV;

	if (!d) {
		usage(argv[0], "Could not read directory", 1, verb);

	} else {
		while((dir = readdir(d)) != NULL) {

			for (a = 0; a < plen; a++) {
				if (dir->d_name[a] != OPREF[a]) break;
				if (a == plen - 1) {
					snprintf(oname, MBCH, "%s%s", ipath, dir->d_name);
					snprintf(nname, MBCH, "%s%s", opath, mknn(dir->d_name, buf));
					if (strcasecmp(nname, ERRSTR) == 0 && verb < -1) {
						fprintf(stderr, "Error: could not rename %s\n", oname);
					} else {
						if (testr || verb) printf("%s\t%s\n", oname, nname);
						// else rename(dir->d_name, nname);
					}
				}
			}
		}
		closedir(d);
	}

	return 0;
}
