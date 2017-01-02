/*

	mvlf.c - Move logfiles (with date format conversion)

	Björn Westerberg Nauclér (mail@bnaucler.se) 2016
	License: MIT (do whatever you want)

*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#define VER 0.2

#define HCENT 2000
#define LCENT 1900
#define YBR 60

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

#define ISPAT "tmY"
#define OSPAT "Y-n-t"

#define YLLEN 4
#define YSLEN 2
#define MSLEN 3
#define MNLEN 2
#define DSLEN 3
#define TLEN 2

#define MBCH 256
#define SBCH 32
#define PDCH 16

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

char stpat[4][6] = {
	"tmY", "Y-n-t", "t/n/y", "t/n/Y"
};

typedef struct ymdt {
	int y;
	int m;
	int d;
	int t;
} ymdt;

int usage(char *cmd, char *err, int ret, int verb) {

	if (err[0] && verb > -1) fprintf(stderr, "Error: %s\n", err);

	printf("Move Logfiles v%.1f\n", VER);
	printf("Usage: %s [-acdhioprtv] [dir]\n", cmd);
	printf("	-a: Autodetect input pattern (experimental)\n");
	printf("	-c: Capitalize output suffix initials\n");
	printf("	-d: Output directory\n");
	printf("	-h: Show this text\n");
	printf("	-i: Input pattern (default: %s)\n", ISPAT);
	printf("	-o: Output pattern (default: %s)\n", OSPAT);
	printf("	-p: Input prefix\n");
	printf("	-r: Output prefix\n");
	printf("	-t: Test run\n");
	printf("	-v: Increase verbosity level\n");
	printf("	-q: Decrease verbosity level\n");

	exit(ret);
}

// Verify pattern vaildity
int vpat(const char *p) {

	unsigned int a = 0;
	int len = strlen(p);
	int hasy = 0, hasm = 0, hasd = 0, hast = 0;

	for(a = 0; a < len; a++) {
		if((p[a] == YL || p[a] == YS) && !hasy) hasy++;
		else if((p[a] == ML || p[a] == MS || p[a] == MN) && !hasm) hasm++;
		else if((p[a] == DL || p[a] == DS) && !hasd) hasd++;
		else if(p[a] == DT && !hast) hast++;
		else if(!isalpha(p[a]) && !isdigit(p[a])) continue;
		else return -1;
	}

	return 0;
}

// Return month number
int mnum(const char *txm) {

	unsigned int a = 0;
	int alen = sizeof(ms) / sizeof(ms[0]);

	for (a = 0; a < alen; a++) {
		if (strcasecmp(txm, ms[a]) == 0) return a + 1;
		if (strcasecmp(txm, ml[a]) == 0) return a + 1;
	}

	return -1;
}

// Return weekday number
int dnum(const char *txd) {

	unsigned int a = 0;
	int alen = sizeof(ds) / sizeof(ds[0]);

	for (a = 0; a < alen; a++) {
		if (strcasecmp(txd, ds[a]) == 0) return a + 1;
		if (strcasecmp(txd, dl[a]) == 0) return a + 1;
	}

	return -1;
}

// Find long format month name
int flm(const char *suf) {

	unsigned int a = 0;
	int alen = sizeof(ml) / sizeof(ml[0]);

	for(a = 0; a < alen; a++) { if(strcasestr(suf, ml[a])) return a + 1; }

	return -1;
}

// Find long format weekday name
int fld(const char *suf) {

	unsigned int a = 0;
	int alen = sizeof(dl) / sizeof(dl[0]);

	for(a = 0; a < alen; a++) { if(strcasestr(suf, dl[a])) return a + 1; }

	return -1;
}

// Make new output name
char *mkoname(const char *opat, const char *opref, char *oname,
		ymdt *date, const int cap) {

	int oplen = strlen(opat);

	char *buf = calloc(SBCH, sizeof(char));

	unsigned int a = 0;

	memset(oname, 0, MBCH);
	strcpy(oname, opref);

	for (a = 0; a < oplen; a++) {
		if(opat[a] == YL) {
			snprintf(buf, SBCH, "%04d", date->y);

		} else if(opat[a] == YS) {
			if(date->y < HCENT) date->y -= LCENT;
			else date->y -= HCENT;
			snprintf(buf, SBCH, "%02d", date->y);

		} else if(opat[a] == ML) {
			strcpy(buf, ml[(date->m-1)]);

		} else if(opat[a] == MN) {
			snprintf(buf, SBCH, "%02d", date->m);

		} else if(opat[a] == MS) {
			strcpy(buf, ms[(date->m-1)]);

		} else if(opat[a] == DL) {
			strcpy(buf, dl[(date->d-1)]);

		} else if(opat[a] == DS) {
			strcpy(buf, ds[(date->d-1)]);

		} else if(opat[a] == DT) {
			snprintf(buf, SBCH, "%02d", date->t);

		} else {
			snprintf(buf, SBCH, "%c", opat[a]);
		}

		if (cap) buf[0] = toupper(buf[0]);

		strcat(oname, buf);
		memset(buf, 0, SBCH);
	}

	return oname;
}

int rstymdt(ymdt *date) {

	date->y = 0;
	date->m = 0;
	date->d = 0;
	date->t = 0;

	return 0;
}

// Read time data from pattern
ymdt *rpat(const char *isuf, const char *ipat, ymdt *date) {

	char *buf = calloc(SBCH, sizeof(char));

	int iplen = strlen(ipat);

	unsigned int a = 0, b = 0, c = 0;
	rstymdt(date);

	// Gather data from input string
	for (a = 0; a < iplen; a++) {
		if(ipat[a] == YL) {
			for(b = 0; b < YLLEN; b++) { buf[b] = isuf[(b+c)]; }
			date->y = atoi(buf);
			c += YLLEN;

		} else if(ipat[a] == YS) {
			for(b = 0; b < YSLEN; b++) { buf[b] = isuf[(b+c)]; }
			date->y = atoi(buf);
			if(date->y > YBR) date->y += LCENT;
			else date->y += HCENT;
			c += YSLEN;

		} else if(ipat[a] == ML) {
			date->m = flm(isuf);
			c += strlen(ml[(date->m - 1)]);

		} else if(ipat[a] == MS) {

			for(b = 0; b < MSLEN; b++) { buf[b] = isuf[(b+c)]; }
			date->m = mnum(buf);
			c += MSLEN;

		} else if(ipat[a] == MN) {
			for(b = 0; b < MNLEN; b++) { buf[b] = isuf[(b+c)]; }
			date->m = atoi(buf);
			c += MNLEN;

		} else if(ipat[a] == DL) {
			date->d = fld(isuf);
			c += strlen(dl[(date->d - 1)]);

		} else if(ipat[a] == DS) {
			for(b = 0; b < DSLEN; b++) { buf[b] = isuf[(b+c)]; }
			date->d = dnum(buf);
			c += DSLEN;

		} else if(ipat[a] == DT) {
			for(b = 0; b < TLEN; b++) { buf[b] = isuf[(b+c)]; }
			date->t = atoi(buf);
			c += TLEN;

		} else {
			c++;
		}

		memset(buf, 0, SBCH);
	}

	return date;
}

// Check for data needed to produce output
int chkdate(ymdt *date, const char *opat) {

	int oplen = strlen(opat);

	unsigned int a = 0;

	for (a = 0; a < oplen; a++) {

		if ((date->y < LCENT || date->y > HCENT + 99) &&
			(opat[a] == YL || opat[a] == YS))
				return 1;

		if ((date->m < 1 || date->m > 12) &&
			(opat[a] == ML || opat[a] == MS || opat[a] == MN))
				return 1;

		if ((date->d < 1 || date->d > 7) &&
			(opat[a] == DL || opat[a] == DS))
				return 1;

		if ((date->t < 1 || date->t > 31) && (opat[a] == DT))
				return 1;

	}

	return 0;
}

// Autodetect pattern
char *adpat(const char *isuf, ymdt *date, const char *opat, const int cap) {

	char *pat = calloc(PDCH + 1, sizeof(char));

	int numpat = sizeof(stpat) / sizeof(stpat[0]);

	unsigned int a = 0;

	for (a = 0; a < numpat; a++) {
		strcpy(pat, stpat[a]);
		rpat(isuf, pat, date);
		if (!chkdate(date, opat)) return pat;
	}

	return ERRSTR;
}

int main(int argc, char *argv[]) {

	DIR *id, *od;
	struct dirent *dir;

	char *ipath = calloc(MBCH, sizeof(char));
	char *opath = calloc(MBCH, sizeof(char));

	char *iname = calloc(MBCH, sizeof(char));
	char *oname = calloc(MBCH, sizeof(char));

	char *ipat = calloc(PDCH + 1, sizeof(char));
	char *opat = calloc(PDCH + 1, sizeof(char));

	char *ipref = calloc(MBCH, sizeof(char));
	char *opref = calloc(MBCH, sizeof(char));

	char *buf = calloc(MBCH, sizeof(char));

	unsigned int a = 0;
	int aut = 0, cap = 0, testr = 0, verb = 0;
	int optc;

	struct ymdt date;

	while((optc = getopt(argc, argv, "acd:hi:o:p:qr:tv")) != -1) {
		switch (optc) {

			case 'a':
				aut++;
				break;

			case 'c':
				cap++;
				break;

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
		strcpy(ipath, argv[optind]);
	} else {
		ipath = getcwd(ipath, MBCH);
	}

	// Path formatting
	if (ipath[strlen(ipath) - 1] != DDIV) ipath[strlen(ipath)] = DDIV;
	id = opendir(ipath);

	// Standard pattern settings (based on eggdrop..)
	if (!ipat[0] && !aut) strcpy(ipat, ISPAT);
	if (!opat[0]) strcpy(opat, OSPAT);

	// Validate patterns
	if (vpat(ipat) < 0) usage(argv[0], "Invalid input pattern", 1, verb);
	if (vpat(opat) < 0) usage(argv[0], "Invalid output pattern", 1, verb);
	if (strncmp(ipref, opref, MBCH) == 0 &&
			strncmp(ipat, opat, PDCH) == 0)
			usage(argv[0], "Input and output is exact match", 1, verb);

	// Check for specified output dir
	if (!opath[0]) {
		strcpy(opath, ipath);
	} else {
		if (opath[strlen(opath) - 1] != DDIV) opath[strlen(opath)] = DDIV;
		od = opendir(opath);
		if (!od) usage(argv[0], "Could not open output directory", 1, verb);
		closedir(od);
	}

	// Check if output prefix has been spcified
	if (!opref[0]) strcpy(opref, ipref);

	if (!id) usage(argv[0], "Could not read directory", 1, verb);

	int iplen = strlen(ipref);

	while((dir = readdir(id)) != NULL) {

		for (a = 0; a < iplen; a++) {
			if (dir->d_name[a] != ipref[a]) break;
			if (a == iplen - 1) {

				snprintf(iname, MBCH, "%s%s", ipath, dir->d_name);

				if (aut) strcpy(ipat, adpat(dir->d_name + iplen,
						&date, opat, cap));
				else rpat(dir->d_name + iplen, ipat, &date);

				if (chkdate(&date, opat)) {
					if (verb > -1)
						fprintf(stderr, "Error: could not rename %s\n", iname);

				} else {
					strcpy(buf, mkoname(opat, opref, oname, &date, cap));
					snprintf(oname, MBCH, "%s%s", opath, buf);
					if (testr || verb) printf("%s -> %s\n", iname, oname);
					if (!testr) rename(iname, oname);
				}

				memset(buf, 0, MBCH);
			}
		}
	}
	closedir(id);

	return 0;
}
