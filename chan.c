/* chan.c: Source code for a word-scrambling program.
   Written by Andrew Plotkin <erkyrath@netcom.com>
   This program is in the public domain. */

#include <stdio.h>

extern char *malloc(), *strcpy();
extern long random(), atol();
extern int atoi();

#define PROG_NAME "chan"

typedef struct _wordbranch {
    char *word;
    struct _wordbranch *left, *right;
} wordbranch;

typedef char **tuple;

typedef struct _reslist {
    char *word;
    long quant;
    struct _reslist *next;
} reslist;

typedef struct _tuplebranch {
    tuple key;
    long entries;
    reslist *results;
    struct _tuplebranch *left, *right;
} tuplebranch;

#define WNULL ((wordbranch *)NULL)
#define TNULL ((tuplebranch *)NULL)
#define RNULL ((reslist *)NULL)
#define TUPNULL ((tuple)NULL)

int chain_dep;
wordbranch *wordroot = WNULL;
tuplebranch *tupleroot = TNULL;

tuple new_tuple()
{
    tuple temp = (tuple)malloc((unsigned)(chain_dep*sizeof(char *)));
    if (temp==TUPNULL) {
        cerror("malloc error\n");
    };
    return temp;
}

tuple cpy_tuple(tup)
tuple tup;
{
    register int ix;
    tuple res = new_tuple();

    for (ix=0; ix<chain_dep; ix++) 
        res[ix] = tup[ix];
    return res;
}

rotate_tuple(old, key)
tuple old;
char *key;
{
    register int ix;
    for (ix=0; ix<chain_dep-1; ix++)
        old[ix] = old[ix+1];
    old[chain_dep-1] = key;
}

int cmp_tuple(t1, t2)
tuple t1, t2;
{
    register int ix;
    for (ix=0; ix<chain_dep; ix++) {
        if (t1[ix] != t2[ix]) {
            return ((t1[ix] < t2[ix]) ? (-1) : (1));
        }
    };
    return 0;
}

tuplebranch *new_tuplebranch(tup)
tuple tup;
{
    tuplebranch *res = (tuplebranch *)malloc(sizeof(tuplebranch));
    if (res==TNULL) {
        cerror("malloc error\n");
    };
    res->key = cpy_tuple(tup);
    res->right = TNULL;
    res->left = TNULL;
    res->entries = 0;
    res->results = RNULL;
    return res;
}

display_tuplebranch(pt)
tuplebranch *pt;
{   
    register int ix;
    reslist *rp;

    for (ix=0; ix<chain_dep; ix++) {
        printf("%s ", pt->key[ix]);
    };
    printf("[%d] {", pt->entries);
    for (rp=pt->results; rp!=RNULL; rp=rp->next) {
        printf("(%d)", rp->quant);
        printf("%s, ", rp->word);
    };
    printf("}\n");
}

reslist *new_reslist(wd)
char *wd;
{
    reslist *res = (reslist *)malloc(sizeof(reslist));
    if (res==RNULL) {
        cerror("malloc error\n");
    };
    res->word = wd;
    res->quant = 1;
    res->next = RNULL;
    return res;
}

wordbranch *new_wordbranch(wd)
char *wd;
{
    int len = strlen(wd);
    wordbranch *ptr = (wordbranch *)malloc(sizeof(wordbranch));
    if (ptr==WNULL) {
        cerror("malloc error\n");
    };
    ptr->left = WNULL;
    ptr->right = WNULL;
    ptr->word = malloc((unsigned)(len+1));
    if (ptr->word==NULL) {
        cerror("malloc error\n");
    };
    (void)strcpy(ptr->word, wd);
    return ptr;
}

dump_wordtree(pt, dep)
wordbranch *pt;
int dep;
{
    register int ix;
    if (pt!=WNULL) {
        dump_wordtree(pt->left, dep+1);
        for (ix=0; ix<dep; ix++) putchar('-');
        puts(pt->word);
        dump_wordtree(pt->right, dep+1);
    }
}

dump_tupletree(pt, dep)
tuplebranch *pt;
int dep;
{
    if (pt != TNULL) {
        dump_tupletree(pt->left, dep+1);
        display_tuplebranch(pt);
        dump_tupletree(pt->right, dep+1);
    }
}

char *add_word(wd)
char *wd;
{
    int res;
    wordbranch **ptr = &wordroot;

    while (*ptr!=WNULL) {
        res = strcmp(wd, (*ptr)->word);
        if (res==0) return (*ptr)->word;
        if (res<0) ptr = &((*ptr)->left);
        else ptr = &((*ptr)->right);
    };
    (*ptr) = new_wordbranch(wd);
    return (*ptr)->word;
}

add_tuplekey(tup, wd)
tuple tup;
char *wd;
{
    int res, found;
    tuplebranch **ptr = &tupleroot;
    reslist **rp;

    found = 0;
    while ((*ptr)!=TNULL && !found) {
        res = cmp_tuple(tup, (*ptr)->key);
        if (res==0) found=1;
        else {
            if (res<0) ptr = &((*ptr)->left);
            else ptr = &((*ptr)->right);
        }
    };
    if (*ptr==TNULL) (*ptr) = new_tuplebranch(tup);
    (*ptr)->entries += 1;
    
    rp = &((*ptr)->results);
    while (*rp != RNULL) {
        if ((*rp)->word == wd) {
            (*rp)->quant += 1;
            return;
        }
        rp = &((*rp)->next);
    };
    *rp = new_reslist(wd);
}

char *read_word(infl)
FILE *infl;
{
    static int lastwhite = (-2); /* broke the last word with this; */
    int length=0;
    int done;
    int tc;
    static char buffer[4096];
    char *final;
    
    if (lastwhite == (-1)) return NULL;
    if (lastwhite != (-2)) buffer[length++] = lastwhite;
    done=0;
    while (!done) {
        tc = getc(infl);
        if (tc==EOF) return NULL;
        buffer[length++] = tc;
        if (tc==' ' || tc=='\t' || tc=='\n') {;}
        else done=1;
    };
    done=0;
    while (!done) {
        tc = getc(infl);
        if (tc==EOF || tc==' ' || tc=='\t' || tc=='\n')
            done=1;
        else
            buffer[length++] = tc;
    };
    lastwhite = tc;
    buffer[length] = '\0';
    final = add_word(buffer);
    return final;
}

char *fetch(tup)
tuple tup;
{
    int res, found;
    tuplebranch *ptr = tupleroot;
    reslist *rp;

    found = 0;
    while (ptr!=TNULL && !found) {
        res = cmp_tuple(tup, ptr->key);
        if (res==0) found=1;
        else {
            if (res<0) ptr = (ptr->left);
            else ptr = (ptr->right);
        }
    };
    if (ptr==TNULL) {
        cerror("unknown tuple found.\n");
    };
    res = (random() % ptr->entries);
    
    rp = ptr->results;
    while (rp != RNULL)        {
        if (res < rp->quant) return rp->word;
        res -= rp->quant;
        rp = rp->next;
    };
    cerror("searchpoint too large.\n");
}

read_print(num_words, infl)
long num_words;
FILE *infl;
{
    register int ix;
    char *newd;
    int done=0;
    tuple start_t, work_t;

    start_t = new_tuple();

    for (ix=0; ix<chain_dep; ix++) {
        newd=read_word(infl);
        if (newd==NULL) {
            cerror("not enough words!\n");
        };
        start_t[ix] = newd;
    };
    work_t = cpy_tuple(start_t);
    while (!done) {
        newd=read_word(infl);
        if (newd==NULL) done=1;
        else {
            add_tuplekey(work_t, newd);
            rotate_tuple(work_t, newd);
        }
    };
    for (ix=0; ix<chain_dep; ix++) {
        add_tuplekey(work_t, start_t[ix]);
        rotate_tuple(work_t, start_t[ix]);
    }

    fprintf(stderr, "chan: beginning output.\n");

    for (ix=0; ix<chain_dep; ix++) {
        fputs(start_t[ix], stdout);
    }

    while (num_words!=0) {
        newd = fetch(start_t);
        fputs(newd, stdout);
        rotate_tuple(start_t, newd);
        if (num_words>0) num_words--;
    }
    fputs("\n", stdout);
}

main(argc, argv)
int argc;
char *argv[];
{
    int ax=1;
    FILE *infl = stdin;
    long num_words = 0;
    chain_dep = 2; /* two words make a third */

    srandom(getpid());

    while (ax<argc) {
        if (!strcmp(argv[ax], "-d")) {
            ax++;
            if (ax==argc) cerror("-d option requires a number");
            chain_dep = atoi(argv[ax]);
        };
        if (!strcmp(argv[ax], "-n")) {
            ax++;
            if (ax==argc) cerror("-n option requires a number");
            num_words = atol(argv[ax]);
        };
        ax++;
    };

    if (num_words && num_words<=chain_dep) 
        cerror("number of words must be greater than depth");

    if (num_words==0) num_words = (-1);
    read_print(num_words, infl);
}

cerror(msg)
char *msg;
{
    fprintf(stderr, "%s: %s\n", PROG_NAME, msg);
    exit(-1);
}
