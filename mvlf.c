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

#define VER 0.1

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

int usage(char *cmd, char *err, int ret, int verb) {

	if (strlen(err) > 0 && verb > -1) fprintf(stderr, "ERROR: %s\n", err);
	printf("Move Logfiles v%.1f\n", VER);
	printf("Usage: %s [-dhioprtv] [dir]\n", cmd);
	printf("	-d: Output directory\n");
	printf("	-h: Show this text\n");
	printf("	-i: Input pattern (default: tmY)\n");
	printf("	-o: Output pattern (default: Y-n-t)\n");
	printf("	-p: Input prefix\n");
	printf("	-r: Output prefix\n");
	printf("	-t: Test run\n");
	printf("	-v: Increase verbosity level\n");
	printf("	-q: Decrease verbosity level\n");

	exit(ret);
}

int vpat(char *p) {

	unsigned int a = 0;
	int len = strlen(p);
	int hasy = 0, hasm = 0, hasd = 0, hast = 0;

	for(a = 0; a < len; a++) {
		if((p[a] == YL || p[a] == YS) && hasy == 0) hasy++;
		else if((p[a] == ML || p[a] == MS || p[a] == MN) && hasm == 0) hasm++;
		else if((p[a] == DL || p[a] == DS) && hasd == 0) hasd++;
		else if((p[a] == DT && hast == 0) ||
				(!isalpha(p[a]) && !isdigit(p[a]))) continue;
		else return -1;
	}

	return 0;
}

int vpatd(char *ipat, char *opat) {

	unsigned int a = 0;
	int ilen = strlen(ipat);
	int olen = strlen(opat);

	int ihy = 0, ihm = 0, ihd = 0, iht = 0;

	for(a = 0; a < ilen; a++) {
		if(ipat[a] == YL || ipat[a] == YS) ihy++;
		if(ipat[a] == ML || ipat[a] == MS || ipat[a] == MN) ihm++;
		if(ipat[a] == DL || ipat[a] == DS) ihd++;
		if(ipat[a] == DT) iht++;
	}

	for(a = 0; a < olen; a++) {
		if((opat[a] == YL || opat[a] == YS) && !ihy) return 1;
		if((opat[a] == ML || opat[a] == MS || opat[a] == MN) && !ihm) return 1;
		if((opat[a] == DL || opat[a] == DS) && !ihd) return 1;
		if(opat[a] == DT && !iht) return 1;
	}

	return 0;
}

int mnum(char *txm) {

	unsigned int a = 0;
	int alen = sizeof(ms) / sizeof(ms[0]);

	for (a = 0; a < alen; a++) {
		if (strcasecmp(txm, ms[a]) == 0) return a + 1;
		if (strcasecmp(txm, ml[a]) == 0) return a + 1;
	}

	return -1;
}

int dnum(char *txd) {

	unsigned int a = 0;
	int alen = sizeof(ds) / sizeof(ds[0]);

	for (a = 0; a < alen; a++) {
		if (strcasecmp(txd, ds[a]) == 0) return a + 1;
		if (strcasecmp(txd, dl[a]) == 0) return a + 1;
	}

	return -1;
}

int flm(char *suf) {

	unsigned int a = 0;
	int alen = sizeof(ml) / sizeof(ml[0]);
	int mnlen = sizeof(ml[0]);

	char *buf = calloc(mnlen, sizeof(char));

	for(a = 0; a < alen; a++) {
		strcpy(buf, suf);
		if(strcasestr(suf, ml[a])) return a + 1;
		memset(buf, 0, mnlen);
	}

	return -1;
}

int fld(char *suf) {

	unsigned int a = 0;
	int alen = sizeof(dl) / sizeof(dl[0]);
	int mnlen = sizeof(dl[0]);

	char *buf = calloc(mnlen, sizeof(char));

	for(a = 0; a < alen; a++) {
		strcpy(buf, suf);
		if(strcasestr(suf, dl[a])) return a + 1;
		memset(buf, 0, mnlen);
	}

	return -1;
}

char *mknn(char *oiname, char *oname, char *ipref,
	char *opref, char *ipat, char *opat) {

	char *buf = calloc(SBCH, sizeof(char));
	char *iname = calloc(MBCH, sizeof(char));

	int iplen = strlen(ipat);
	int oplen = strlen(opat);

	unsigned int a = 0, b = 0, c = 0;
	int y = 0, m = 0, d = 0, t = 0;

	strcpy(iname, oiname);
	iname += strlen(ipref);

	// Gather data from input string
	for (a = 0; a < iplen; a++) {
		if(ipat[a] == YL) {
			for(b = 0; b < YLLEN; b++) { buf[b] = iname[(b+c)]; }
			y = atoi(buf);
			c += YLLEN;

		} else if(ipat[a] == YS) {
			for(b = 0; b < YSLEN; b++) { buf[b] = iname[(b+c)]; }
			y = atoi(buf);
			if(y > YBR) y += LCENT;
			else y += HCENT;
			c += YSLEN;

		} else if(ipat[a] == ML) {
			m = flm(iname);
			c += strlen(ml[(m - 1)]);

		} else if(ipat[a] == MS) {

			for(b = 0; b < MSLEN; b++) { buf[b] = iname[(b+c)]; }
			m = mnum(buf);
			c += MSLEN;

		} else if(ipat[a] == MN) {
			for(b = 0; b < MNLEN; b++) { buf[b] = iname[(b+c)]; }
			m = atoi(buf);
			c += MNLEN;

		} else if(ipat[a] == DL) {
			d = fld(iname);
			c += strlen(dl[(d - 1)]);

		} else if(ipat[a] == DS) {
			for(b = 0; b < DSLEN; b++) { buf[b] = iname[(b+c)]; }
			d = dnum(buf);
			c += DSLEN;

		} else if(ipat[a] == DT) {
			for(b = 0; b < TLEN; b++) { buf[b] = iname[(b+c)]; }
			t = atoi(buf);
			c += TLEN;

		} else {
			c++;
		}

		memset(buf, 0, SBCH);
	}

	if(m < 0 || m > 12 || d < 0 || t > 31) return ERRSTR;

	// Create output string
	a = 0, b = 0, c = 0;
	memset(oname, 0, MBCH);
	strcpy(oname, opref);

	for (a = 0; a < oplen; a++) {
		if(opat[a] == YL) {
			snprintf(buf, SBCH, "%04d", y);

		} else if(opat[a] == YS) {
			if(y < HCENT) y -= LCENT;
			else y -= HCENT;
			snprintf(buf, SBCH, "%02d", y);

		} else if(opat[a] == ML) {
			strcat(oname, ml[(m-1)]);

		} else if(opat[a] == MN) {
			snprintf(buf, SBCH, "%02d", m);

		} else if(opat[a] == MS) {
			strcat(oname, ms[(m-1)]);

		} else if(opat[a] == DL) {
			strcat(oname, dl[(d-1)]);

		} else if(opat[a] == DS) {
			strcat(oname, ds[(d-1)]);

		} else if(opat[a] == DT) {
			snprintf(buf, SBCH, "%02d", t);

		} else {
			snprintf(buf, SBCH, "%c", opat[a]);

		}

		if(strlen(buf) > 0) strcat(oname, buf);
		memset(buf, 0, SBCH);
	}

	return oname;
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
	char *buf2 = calloc(MBCH, sizeof(char));

	unsigned int a = 0;
	int testr = 0, verb = 0;
	int optc;

	while((optc = getopt(argc, argv, "d:hi:o:p:qr:tv")) != -1) {
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
		strcpy(ipath, argv[optind]);
	} else {
		ipath = getcwd(ipath, MBCH);
	}

	// Path formatting
	if (ipath[strlen(ipath) - 1] != DDIV) ipath[strlen(ipath)] = DDIV;
	id = opendir(ipath);

	// Standard pattern settings (based on eggdrop..)
	if (strlen(ipat) == 0) strcpy(ipat, ISPAT);
	if (strlen(opat) == 0) strcpy(opat, OSPAT);

	// Validate patterns
	if (vpat(ipat) < 0) usage(argv[0], "Invalid input pattern", 1, verb);
	if (vpat(opat) < 0) usage(argv[0], "Invalid output pattern", 1, verb);
	if (strncmp(ipref, opref, MBCH) == 0 &&
			strncmp(ipat, opat, PDCH) == 0)
			usage(argv[0], "Input and output is exact match", 1, verb);
	if (vpatd(ipat, opat))
		usage(argv[0], "Cannot create data from thin air", 1, verb);

	// Check for specified output dir
	if (strlen(opath) == 0) {
		strcpy(opath, ipath);
	} else {
		if (opath[strlen(opath) - 1] != DDIV) opath[strlen(opath)] = DDIV;
		od = opendir(opath);
		if (!od) usage(argv[0], "Could not open output directory", 1, verb);
		closedir(od);
	}

	// Check if output prefix has been spcified
	if (strlen(opref) == 0) strcpy(opref, ipref);

	if (!id) usage(argv[0], "Could not read directory", 1, verb);

	int iplen = strlen(ipref);

	while((dir = readdir(id)) != NULL) {

		for (a = 0; a < iplen; a++) {
			if (dir->d_name[a] != ipref[a]) break;
			if (a == iplen - 1) {
				snprintf(iname, MBCH, "%s%s", ipath, dir->d_name);
				snprintf(buf2, MBCH, "%s", mknn(dir->d_name,
							buf, ipref, opref, ipat, opat));

				if (strcasecmp(buf2, ERRSTR) == 0 && verb > -1) {
					fprintf(stderr, "Error: could not rename %s\n", iname);

				} else {
					snprintf(oname, MBCH, "%s%s", opath, buf2);
					if (testr || verb) printf("%s -> %s\n", iname, oname);
					if (!testr) rename(iname, oname);
				}
			}
		}
	}
	closedir(id);

	return 0;
}
