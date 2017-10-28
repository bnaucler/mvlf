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

#define VER "0.3A"

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
} flag;

static int usage(char *cmd, char *err, int ret, int vfl) {

    if(err[0] && vfl > -1) fprintf(stderr, "Error: %s\n", err);

    printf("mvlf (move logfiles) v%s\n"
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

    char *ptr;

    long lret = strtol(str, &ptr, 10);

    if(ptr == str) return -1;

    if(lret > INT_MAX) lret = INT_MAX;
    else if(lret < INT_MIN) lret = INT_MIN;

    return (int)lret;
}

// Verify pattern validity
static int vpat(const char *p) {

    int hasy = 0, hasm = 0, hasd = 0, hast = 0;

    do {
        if((*p == YL || *p == YS) && !hasy) hasy++;
        else if((*p == ML || *p == MS || *p == MN) && !hasm) hasm++;
        else if((*p == DL || *p == DS) && !hasd) hasd++;
        else if(*p == DT && !hast) hast++;
        else if(!isalnum(*p)) continue;
        else return 1;
    } while(*++p);

    return 0;
}

// Return s index from arr
static int arrind(const char *s, const int mxl, const char arr[][mxl]) {

    int i = 0;

    while(arr[i][0]) { if(strcasestr(s, arr[i++])) return i; }

    return -1;
}

// Make new output name
static char *mkoname(char *bbuf, ymdt *date, flag *f) {

    char sbuf[SBCH];
    char *p = f->opat;

    strncpy(bbuf, f->opref, MBCH);

    while(*p) {
        switch(*p) {

            case(YL):
                snprintf(sbuf, SBCH, "%04d", date->y);
                break;

            case(YS):
                date->y -= date->y < HCENT ? LCENT : HCENT;
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
                snprintf(sbuf, SBCH, "%c", *p);
                break;
        }

        if(f->cfl) sbuf[0] = toupper(sbuf[0]);
        strncat(bbuf, sbuf, MBCH);
        p++;
    }

    return bbuf;
}

// Return matoi of substring
static int matoisubstr(const char **src, const size_t len) {

    char buf[(len + 1)];

    memcpy(buf, *src, len);
    *src += len;
    buf[(len + 1)] = 0;

    return matoi(buf);
}

// Return array index of substring
static int arrindsubstr(const char **src, const int mxl, const char arr[][mxl],
    const size_t len) {

    char buf[(len + 1)];

    memcpy(buf, *src, len);
    *src += len;
    buf[(len + 1)] = 0;

    return arrind(buf, mxl, ms);
}

// Read date from pattern
static void rpat(const char *isuf, const char *ipat, ymdt *date) {

    while(*ipat) {
        switch(*ipat) {

            case(YL):
                date->y = matoisubstr(&isuf, YLLEN);
                break;

            case(YS):
                date->y = matoisubstr(&isuf, YSLEN);
                date->y += date->y > YBR ? LCENT : HCENT;
                break;

            case(ML):
                date->m = arrind(isuf, sizeof(ml[0]), ml);
                isuf += strlen(ml[(date->m - 1)]);
                break;

            case(MS):
                date->m = arrindsubstr(&isuf, sizeof(ms[0]), ms, MSLEN);
                break;

            case(MN):
                date->m = matoisubstr(&isuf, MNLEN);
                break;

            case(DL):
                date->d = arrind(isuf, sizeof(dl[0]), dl);
                isuf += strlen(dl[(date->d - 1)]);
                break;

            case(DS):
                date->d = arrindsubstr(&isuf, sizeof(ds[0]), ds, DSLEN);
                break;

            case(DT):
                date->t = matoisubstr(&isuf, TLEN);
                break;

            default:
                isuf++;
                break;
        }
        ipat++;
    }
}

// Check for data needed to produce output
static int chkdate(const ymdt *date, const char *opat) {

    while(*opat) {
        switch(*opat) {

            case(YL): case(YS):
                if(date->y < LCENT || date->y > HCENT + 99) return 1;
                break;

            case(DL): case(DS):
                if(date->d < 1 || date->d > 7) return 1;
                break;

            case(ML): case(MS): case(MN):
                if(date->m < 1 || date->m > 12) return 1;
                break;

            case(DT):
                if(date->t < 1 || date->t > 31) return 1;
                break;
        }

        opat++;
    }

    return 0;
}

// Autodetect pattern TODO: Speed up
static char *adpat(const char *isuf, ymdt *date, char *pat, const flag *f) {

    int numpat = sizeof(stpat) / sizeof(stpat[0]);
    unsigned int a = 0;

    for(a = 0; a < numpat; a++) {
        strncpy(pat, stpat[a], PDCH);
        rpat(isuf, pat, date);
        if(!chkdate(date, f->opat)) return pat;
        memset(date, 0, sizeof(*date));
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

    while(*p == *s) {
        if(!*++p) return 1;
        s++;
    }

    return 0;
}

// Dump debug data to stdout
static void debugprint(const flag *f) {

    printf("DEBUG ipath: %s, opath: %s, iname: %s, oname: %s, "
           "ipat: %s, opat: %s, ipref: %s, opref: %s\n",
           f->ipath, f->opath, f->iname, f->oname,
           f->ipat, f->opat, f->ipref, f->opref
          );
}

// Execute move operation
static int execute(const char *fn, flag *f) {

    char pat[PDCH], on[MBCH];
    int iplen = strlen(f->ipref);
    ymdt date;

    if(f->afl) strncpy(f->ipat, adpat(fn + iplen, &date, pat, f), PDCH);
    else rpat(fn + iplen, f->ipat, &date);

    if(chkdate(&date, f->opat)) return 1;

    snprintf(f->iname, BBCH, "%s%c%s", f->ipath, DDIV, fn);
    snprintf(f->oname, BBCH, "%s%c%s", f->opath, DDIV, mkoname(on, &date, f));

    // Dump debug data if executed with -vv
    if(f->vfl > 1) debugprint(f);

    if(!f->tfl) rename(f->iname, f->oname);

    return 0;
}

int main(int argc, char **argv) {

    DIR *id, *od;
    struct dirent *dir;

    // Initialize flags
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

    // Check if output prefix has been specified
    if(!f->opref[0]) strncpy(f->opref, f->ipref, PTCH);

    // Open input dir
    id = opendir(f->ipath);
    if(!id) usage(f->cmd, "Could not read directory", 1, f->vfl);

    // Loop through directory and process files
    while((dir = readdir(id)) != NULL) {
        if(!strst(dir->d_name, f->ipref)) {
            if(f->vfl > 0) fprintf(stderr, "Ignoring %s\n", dir->d_name);
            continue;
        }

        if(execute(dir->d_name, f) && f->vfl > -1)
            fprintf(stderr, "Error: could not rename %s\n", f->iname);
        else if(f->tfl || f->vfl)
            printf("%s -> %s\n", f->iname, f->oname);
    }

    // Cleanup
    closedir(id);
    free(f);

    return 0;
}
