/*

    mvlf.c - Move logfiles (with date format conversion)

    Björn Westerberg Nauclér (mail@bnaucler.se) 2016-2017
    https://github.com/bnaucler/mvlf
    License: MIT (do whatever you want)

*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
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

#define BBCH 512
#define MBCH 256
#define SBCH 32
#define PDCH 16
#define PTCH (MBCH - PDCH)

static const char ms[12][4] = {
    "jan", "feb", "mar", "apr",
    "may", "jun", "jul", "aug",
    "sep", "oct", "nov", "dec"
};

static const char ml[12][10] = {
    "january", "february", "march", "april",
    "may", "june", "july", "august",
    "september", "october", "november", "december"
};

static const char ds[7][4] = {
    "mon", "tue", "wed", "thu",
    "fri", "sat", "sun"
};

static const char dl[7][10] = {
    "monday", "tuesday", "wednesday", "thursday",
    "friday", "saturday", "sunday"
};

static const char stpat[4][6] = {
    "tmY", "Y/n/t", "t/n/y", "t/n/Y"
};

typedef struct ymdt {
    int y;
    int m;
    int d;
    int t;
} ymdt;

static int usage(char *cmd, char *err, int ret, int verb) {

    if(err[0] && verb > -1) fprintf(stderr, "Error: %s\n", err);

    printf("Move Logfiles v%.1f\n"
           "Usage: %s [-achioqtv] [dir/prefix] [dir/prefix]\n"
           "    -a: Autodetect input pattern (experimental)\n"
           "    -c: Capitalize output suffix initials\n"
           "    -h: Show this text\n"
           "    -i: Input pattern (default: %s)\n"
           "    -o: Output pattern (default: %s)\n"
           "    -q: Decrease verbosity level\n"
           "    -t: Test run\n"
           "    -v: Increase verbosity level\n",
        VER, cmd, ISPAT, OSPAT);

    exit(ret);
}

// Verify pattern vaildity
static int vpat(const char *p) {

    unsigned int a = 0;
    int len = strlen(p);
    int hasy = 0, hasm = 0, hasd = 0, hast = 0;

    for(a = 0; a < len; a++) {
        if((p[a] == YL || p[a] == YS) && !hasy) hasy++;
        else if((p[a] == ML || p[a] == MS || p[a] == MN) && !hasm) hasm++;
        else if((p[a] == DL || p[a] == DS) && !hasd) hasd++;
        else if(p[a] == DT && !hast) hast++;
        else if(!isalnum(p[a])) continue;
        else return 1;
    }

    return 0;
}

// Return month number
static int mnum(const char *txm) {

    unsigned int a = 0;
    int alen = sizeof(ms) / sizeof(ms[0]);

    for(a = 0; a < alen; a++) {
        if(!strcasecmp(txm, ms[a]) || !strcasecmp(txm, ml[a])) return a + 1;
    }

    return -1;
}

// Return weekday number
static int dnum(const char *txd) {

    unsigned int a = 0;
    int alen = sizeof(ds) / sizeof(ds[0]);

    for(a = 0; a < alen; a++) {
        if(!strcasecmp(txd, ds[a]) || !strcasecmp(txd, dl[a])) return a + 1;
    }

    return -1;
}

// Find long format month name
static int flm(const char *suf) {

    unsigned int a = 0;
    int alen = sizeof(ml) / sizeof(ml[0]);

    for(a = 0; a < alen; a++) { if(strcasestr(suf, ml[a])) return a + 1; }

    return -1;
}

// Find long format weekday name
static int fld(const char *suf) {

    unsigned int a = 0;
    int alen = sizeof(dl) / sizeof(dl[0]);

    for(a = 0; a < alen; a++) { if(strcasestr(suf, dl[a])) return a + 1; }

    return -1;
}

// Make new output name
static char *mkoname(const char *opat, const char *opref,
        ymdt *date, const int cap) {

    char *sbuf = calloc(SBCH, sizeof(char));
    char *bbuf = calloc(MBCH, sizeof(char));

    int oplen = strlen(opat);
    unsigned int a = 0;

    strncpy(bbuf, opref, MBCH);

    for(a = 0; a < oplen; a++) {
        switch(opat[a]) {

            case(YL):
                snprintf(sbuf, SBCH, "%04d", date->y);
            break;

            case(YS):
                if(date->y < HCENT) date->y -= LCENT;
                else date->y -= HCENT;
                snprintf(sbuf, SBCH, "%02d", date->y);
            break;

            case(ML):
                strncpy(sbuf, ml[(date->m-1)], SBCH);
            break;

            case(MN):
                snprintf(sbuf, SBCH, "%02d", date->m);
            break;

            case(MS):
                strncpy(sbuf, ms[(date->m-1)], SBCH);
            break;

            case(DL):
                strncpy(sbuf, dl[(date->d-1)], SBCH);
            break;

            case(DS):
                strncpy(sbuf, ds[(date->d-1)], SBCH);
            break;

            case(DT):
                snprintf(sbuf, SBCH, "%02d", date->t);
            break;

            default:
                snprintf(sbuf, SBCH, "%c", opat[a]);
            break;
        }

        if(cap) sbuf[0] = toupper(sbuf[0]);

        strcat(bbuf, sbuf);
    }

    free(sbuf);
    return bbuf;
}

// Read time data from pattern
static ymdt *rpat(const char *isuf, const char *ipat, ymdt *date) {

    char *buf = calloc(SBCH, sizeof(char));

    int iplen = strlen(ipat);
    unsigned int a = 0, b = 0, c = 0;

    memset(date, 0, sizeof(*date)); // redundant?

    for(a = 0; a < iplen; a++) {
        switch(ipat[a]) {
            case(YL):
                for(b = 0; b < YLLEN; b++) { buf[b] = isuf[(b+c)]; }
                date->y = atoi(buf);
                c += YLLEN;
            break;

            case(YS):
                for(b = 0; b < YSLEN; b++) { buf[b] = isuf[(b+c)]; }
                date->y = atoi(buf);
                if(date->y > YBR) date->y += LCENT;
                else date->y += HCENT;
                c += YSLEN;
            break;

            case(ML):
                date->m = flm(isuf);
                c += strlen(ml[(date->m - 1)]);
            break;

            case(MS):
                for(b = 0; b < MSLEN; b++) { buf[b] = isuf[(b+c)]; }
                date->m = mnum(buf);
                c += MSLEN;
            break;

            case(MN):
                for(b = 0; b < MNLEN; b++) { buf[b] = isuf[(b+c)]; }
                date->m = atoi(buf);
                c += MNLEN;
            break;

            case(DL):
                date->d = fld(isuf);
                c += strlen(dl[(date->d - 1)]);
            break;

            case(DS):
                for(b = 0; b < DSLEN; b++) { buf[b] = isuf[(b+c)]; }
                date->d = dnum(buf);
                c += DSLEN;
            break;

            case(DT):
                for(b = 0; b < TLEN; b++) { buf[b] = isuf[(b+c)]; }
                date->t = atoi(buf);
                c += TLEN;
            break;

            default:
                c++;
            break;
        }

        memset(buf, 0, SBCH);
    }

    free(buf);
    return date;
}

// Check for data needed to produce output
static int chkdate(ymdt *date, const char *opat) {

    int oplen = strlen(opat);
    unsigned int a = 0;

    for(a = 0; a < oplen; a++) {

        if((date->y < LCENT || date->y > HCENT + 99) &&
            (opat[a] == YL || opat[a] == YS))
                return 1;

        if((date->m < 1 || date->m > 12) &&
            (opat[a] == ML || opat[a] == MS || opat[a] == MN))
                return 1;

        if((date->d < 1 || date->d > 7) &&
            (opat[a] == DL || opat[a] == DS))
                return 1;

        if((date->t < 1 || date->t > 31) && (opat[a] == DT))
                return 1;

    }

    return 0;
}

// Autodetect pattern
static char *adpat(const char *isuf, ymdt *date, const char *opat, const int cap) {

    char *pat = calloc(PDCH, sizeof(char));

    int numpat = sizeof(stpat) / sizeof(stpat[0]);
    unsigned int a = 0;

    for(a = 0; a < numpat; a++) {
        strncpy(pat, stpat[a], PDCH);
        rpat(isuf, pat, date);
        if(!chkdate(date, opat)) return pat;
    }

    return ERRSTR;
}

int main(int argc, char *argv[]) {

    DIR *id, *od;
    struct dirent *dir;

    char ipath[BBCH];
    char opath[BBCH];

    char iname[BBCH];
    char oname[BBCH];

    char ipat[PDCH];
    char opat[PDCH];

    char ipref[PTCH];
    char opref[PTCH];

    int optc;
    int aut = 0, cap = 0, testr = 0, verb = 0;
    unsigned int a = 0;

    struct ymdt date;

    while((optc = getopt(argc, argv, "achi:o:qtv")) != -1) {
        switch(optc) {

            case 'a':
                aut++;
                break;

            case 'c':
                cap++;
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
    if(argc > optind + 2) {
        usage(argv[0], "Too many arguments", 1, verb);
    } else if(argc > optind) {
        strncpy(ipath, argv[optind], BBCH);
        if(argc == optind + 2) strncpy(opath, argv[optind + 1], BBCH);
    } else {
        usage(argv[0], "Nothing to do", 0, verb);
    }

    // Get file names from paths
    if(!opath[0]) strncpy(opath, ipath, BBCH);

    strncpy(ipath, dirname(ipath), BBCH);
    strncpy(opath, dirname(opath), BBCH);
    strncpy(ipref, basename(ipath), PTCH);
    strncpy(opref, basename(opath), PTCH);

    if(strncmp(ipath, opath, BBCH)) {
        od = opendir(opath);
        if(!od) usage(argv[0], "Could not open output directory", 1, verb);
        closedir(od);
    }

    // Standard pattern settings (based on eggdrop..)
    if(!ipat[0] && !aut) strncpy(ipat, ISPAT, PDCH);
    if(!opat[0]) strncpy(opat, OSPAT, PDCH);

    // Validate patterns
    if(vpat(ipat)) usage(argv[0], "Invalid input pattern", 1, verb);
    if(vpat(opat)) usage(argv[0], "Invalid output pattern", 1, verb);
    if(strncmp(ipath, opath, BBCH) == 0 &&
            strncmp(ipref, opref, MBCH - PDCH) == 0 &&
            strncmp(ipat, opat, PDCH) == 0)
            usage(argv[0], "Input and output are exact match", 1, verb);

    // Check if output prefix has been spcified
    if(!opref[0]) strncpy(opref, ipref, PTCH);

    // Open input dir
    id = opendir(ipath);
    if(!id) usage(argv[0], "Could not read directory", 1, verb);

    int iplen = strlen(ipref);

    while((dir = readdir(id)) != NULL) {

        for(a = 0; a < iplen; a++) {
            if(dir->d_name[a] != ipref[a]) break;
            if(a == iplen - 1) {

                snprintf(iname, BBCH, "%s%c%s", ipath, DDIV, dir->d_name);

                if(aut) strncpy(ipat, adpat(dir->d_name + iplen,
                        &date, opat, cap), PDCH);
                else rpat(dir->d_name + iplen, ipat, &date);

                if(chkdate(&date, opat)) {
                    if(verb > -1)
                        fprintf(stderr, "Error: could not rename %s\n", iname);

                } else {
                    snprintf(oname, BBCH, "%s%c%s", opath, DDIV,
                            mkoname(opat, opref, &date, cap));
                    if(testr || verb) printf("%s -> %s\n", iname, oname);
                    if(!testr) rename(iname, oname);
                }
            }
        }
    }
    closedir(id);

    return 0;
}
