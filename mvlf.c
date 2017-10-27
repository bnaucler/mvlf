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
#include <limits.h>
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
    int y, m, d, t;
} ymdt;

typedef struct flag {
    char cmd[SBCH];

    char ipath[BBCH];
    char opath[BBCH];

    char iname[BBCH];
    char oname[BBCH];

    char ipat[PDCH];
    char opat[PDCH];

    char ipref[PTCH];
    char opref[PTCH];

    int afl, cfl, tfl, vfl;
    int iplen;
} flag;

static int usage(char *cmd, char *err, int ret, int vfl) {

    if(err[0] && vfl > -1) fprintf(stderr, "Error: %s\n", err);

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

// Safe atoi
static int matoi(char* str) {

    long lret = strtol(str, NULL, 10);

    if(lret > INT_MAX) lret = INT_MAX;
    else if(lret < INT_MIN) lret = INT_MIN;

    return (int)lret;
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
static char *mkoname(ymdt *date, const flag *f) {

    char sbuf[SBCH];
    char *bbuf = calloc(MBCH, sizeof(char));

    int oplen = strlen(f->opat);
    unsigned int a = 0;

    strncpy(bbuf, f->opref, MBCH);

    for(a = 0; a < oplen; a++) {
        switch(f->opat[a]) {

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
                snprintf(sbuf, SBCH, "%c", f->opat[a]);
            break;
        }

        if(f->cfl) sbuf[0] = toupper(sbuf[0]);
        strcat(bbuf, sbuf);
    }

    return bbuf;
}

// Read date from pattern
static ymdt *rpat(const char *isuf, const char *ipat, ymdt *date) {

    char buf[SBCH];

    int iplen = strlen(ipat);
    unsigned int a = 0, b = 0, c = 0;

    memset(date, 0, sizeof(*date));

    for(a = 0; a < iplen; a++) {
        switch(ipat[a]) {
            case(YL):
                for(b = 0; b < YLLEN; b++) { buf[b] = isuf[(b+c)]; }
                date->y = matoi(buf);
                c += YLLEN;
            break;

            case(YS):
                for(b = 0; b < YSLEN; b++) { buf[b] = isuf[(b+c)]; }
                date->y = matoi(buf);
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
                date->m = matoi(buf);
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
                date->t = matoi(buf);
                c += TLEN;
            break;

            default:
                c++;
            break;
        }

        memset(buf, 0, SBCH);
    }

    return date;
}

// Check for data needed to produce output
static int chkdate(ymdt *date, const char *opat) {

    // int oplen = strlen(opat);
    // unsigned int a = 0;
    // for(a = 0; a < oplen; a++) {

    while(*opat) {
        if((date->y < LCENT || date->y > HCENT + 99) &&
            (*opat == YL || *opat == YS))
                return 1;

        if((date->m < 1 || date->m > 12) &&
            (*opat == ML || *opat == MS || *opat == MN))
                return 1;

        if((date->d < 1 || date->d > 7) &&
            (*opat == DL || *opat == DS))
                return 1;

        if((date->t < 1 || date->t > 31) && (*opat == DT))
                return 1;

        opat++;
    }

    return 0;
}

// Autodetect pattern
static char *adpat(const char *isuf, ymdt *date, const flag *f) {

    char *pat = calloc(PDCH, sizeof(char));

    int numpat = sizeof(stpat) / sizeof(stpat[0]);
    unsigned int a = 0;

    for(a = 0; a < numpat; a++) {
        strncpy(pat, stpat[a], PDCH);
        rpat(isuf, pat, date);
        if(!chkdate(date, f->opat)) return pat;
    }

    return NULL;
}

// Get options; reset argc & argv
static char **opts(int *argc, char **argv, flag *f) {

    int optc;

    while((optc = getopt(*argc, argv, "achi:o:qtv")) != -1) {
        switch(optc) {

            case 'a':
                f->afl++;
                break;

            case 'c':
                f->cfl++;
                break;

            case 'h':
                usage(f->cmd, "", 0, f->vfl);
                break;

            case 'i':
                strncpy(f->ipat, optarg, PDCH);
                break;

            case 'o':
                strncpy(f->opat, optarg, PDCH);
                break;

            case 'q':
                f->vfl--;
                break;

            case 't':
                f->tfl++;
                break;

            case 'v':
                f->vfl++;
                break;

            default:
                usage(f->cmd, "", 1, f->vfl);
                break;
        }
    }

    *argc -= optind;
    return argv += optind;
}

// Create flags and set base configuration
static flag *getflag(const char *cmd) {

    flag *f = calloc(1, sizeof(flag));
    f->afl = f->cfl = f->tfl = f->vfl = 0;
    strncpy(f->cmd, cmd, SBCH);

    return f;
}

// Return 0 if s has prefix p
static int strst(const char *s, const char *p) {

    while(*p++ == *s++);

    if(!*p) return 1;
    else return 0;
}

// Dump debug data to stdout
static void debugprint(const flag *f) {

    printf("DEBUG output: ipath: %s, opath: %s, iname: %s, oname: %s, "
           "ipat: %s, opat: %s, ipref: %s, opref: %s\n",
           f->ipath, f->opath, f->iname, f->oname,
           f->ipat, f->opat, f->ipref, f->opref
          );
}

// Execute move operation
static int execute(const char *fn, flag *f, ymdt *date) {

    snprintf(f->iname, BBCH, "%s%c%s", f->ipath, DDIV, fn);

    if(f->afl) strncpy(f->ipat, adpat(fn + f->iplen, date, f), PDCH);
    else rpat(fn + f->iplen, f->ipat, date);

    if(chkdate(date, f->opat)) return 1; 

    snprintf(f->oname, BBCH, "%s%c%s", f->opath, DDIV, mkoname(date, f));
    if(f->tfl || f->vfl) printf("%s -> %s\n", f->iname, f->oname);
    if(!f->tfl) rename(f->iname, f->oname);

    return 0;
}

int main(int argc, char **argv) {

    DIR *id, *od;
    struct dirent *dir;
    ymdt date;

    flag *f = getflag(argv[0]);
    argv = opts(&argc, argv, f);

    // Input verification
    if(argc > 1) usage(f->cmd, "Too many arguments", 1, f->vfl);
    else if(argc < 0) usage(f->cmd, "Nothing to do", 0, f->vfl);

    // Get paths
    strncpy(f->ipath, argv[0], BBCH);
    if(argc > 1) strncpy(f->opath, argv[1], BBCH);

    // Get folder names and prefixes
    if(!f->opath[0]) strncpy(f->opath, f->ipath, BBCH);

    strncpy(f->ipref, basename(f->ipath), PTCH);
    strncpy(f->ipath, dirname(f->ipath), BBCH);
    strncpy(f->opref, basename(f->opath), PTCH);
    strncpy(f->opath, dirname(f->opath), BBCH);

    f->iplen = strlen(f->ipref);

    // Validate output directory
    if(strncmp(f->ipath, f->opath, BBCH)) {
        od = opendir(f->opath);
        if(!od) usage(f->cmd, "Could not open output directory", 1, f->vfl);
        closedir(od);
    }

    // Standard pattern settings (based on eggdrop..)
    if(!f->ipat[0] && !f->afl) strncpy(f->ipat, ISPAT, PDCH);
    if(!f->opat[0]) strncpy(f->opat, OSPAT, PDCH);

    // Validate patterns
    if(vpat(f->ipat)) usage(f->cmd, "Invalid input pattern", 1, f->vfl);
    if(vpat(f->opat)) usage(f->cmd, "Invalid output pattern", 1, f->vfl);
    if(!strncmp(f->ipath, f->opath, BBCH) &&
       !strncmp(f->ipref, f->opref, PTCH) &&
       !strncmp(f->ipat, f->opat, PDCH))
            usage(f->cmd, "Input and output are exact match", 1, f->vfl);

    // Check if output prefix has been spcified
    if(!f->opref[0]) strncpy(f->opref, f->ipref, PTCH);

    // Open input dir
    id = opendir(f->ipath);
    if(!id) usage(f->cmd, "Could not read directory", 1, f->vfl);

    // Dump debug data if executed with -vv
    if(f->vfl > 1) debugprint(f);

    // Loop through directory and process files
    while((dir = readdir(id)) != NULL) {
        if(!strst(dir->d_name, f->ipref))
            continue;

        if(execute(dir->d_name, f, &date) && f->vfl < 0)
            fprintf(stderr, "Error: could not rename %s\n", f->iname);
    }

    // Cleanup
    closedir(id);
    free(f);

    return 0;
}
