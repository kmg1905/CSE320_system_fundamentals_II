#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "../include/task.h"
#include "../include/debug.h"

/* Functions to free parsed objects. */

static void free_word(char *wp) {
    free(wp);
}

static void free_word_list(WORD_LIST *wlp) {
    free_word(wlp->first);
    if(wlp->rest != NULL)
    free_word_list(wlp->rest);
    free(wlp);
}

static void free_command(COMMAND *cp) {
    free_word_list(cp->words);
    free(cp);
}

static void free_command_list(COMMAND_LIST *clp) {
    free_command(clp->first);
    if(clp->rest != NULL)
    free_command_list(clp->rest);
    free(clp);
}

static void free_pipeline(PIPELINE *pp) {
    free_command_list(pp->commands);
    free(pp);
}

static void free_pipeline_list(PIPELINE_LIST *plp) {
    free_pipeline(plp->first);
    if(plp->rest != NULL)
    free_pipeline_list(plp->rest);
    free(plp);
}

void free_task(TASK *tp) {
    free_pipeline_list(tp->pipelines);
    free(tp);
}

/* Unparsing functions. */

static void unparse_word(char *wp, FILE *out) {
    fprintf(out, "%s", wp);
}

static void unparse_word_list(WORD_LIST *wlp, FILE *out) {
    unparse_word(wlp->first, out);
    if(wlp->rest != NULL) {
    fprintf(out, " ");
    unparse_word_list(wlp->rest, out);
    }
}

static void unparse_command(COMMAND *cp, FILE *out) {
    unparse_word_list(cp->words, out);
}

static void unparse_command_list(COMMAND_LIST *clp, FILE *out) {
    unparse_command(clp->first, out);
    if(clp->rest != NULL) {
    fprintf(out, " | ");
    unparse_command_list(clp->rest, out);
    }
}

static void unparse_pipeline(PIPELINE *pp, FILE *out) {
    unparse_command_list(pp->commands, out);
    if(pp->input_path != NULL)
    fprintf(out, " < %s", pp->input_path);
    if(pp->output_path != NULL)
    fprintf(out, " > %s", pp->output_path);
}

static void unparse_pipeline_list(PIPELINE_LIST *plp, FILE *out) {
    unparse_pipeline(plp->first, out);
    if(plp->rest != NULL) {
    fprintf(out, " ; ");
    unparse_pipeline_list(plp->rest, out);
    }
}

void unparse_task(TASK *tp, FILE *out) {
    unparse_pipeline_list(tp->pipelines, out);
}

/* Parsing functions. */

static void skip_spaces(char **linep) {
    while(isspace(**linep))
    (*linep)++;
}

#define ISDELIM(c) ((c) == ';' || (c) == '|' || (c) == '<' || (c) == '>')

static char *parse_word(char **linep) {
    char *wp = *linep;
    while(**linep != '\0' && !isspace(**linep) && !ISDELIM(**linep))
    (*linep)++;
    if(wp == *linep)
    return NULL;
    char c = **linep;
    **linep = '\0';
    wp = strdup(wp);
    **linep = c;
    if(isspace(**linep))
    (*linep)++;
#ifdef DEBUG
    fprintf(stderr, "parse_word => %s [remaining: '%s' ]", wp, *linep);
    fprintf(stderr, "\n");
#endif
    return wp;
}

static WORD_LIST *parse_word_list(char **linep) {
    skip_spaces(linep);
    char *wp = parse_word(linep);
    if(wp == NULL)
    return NULL;
    WORD_LIST *wlp = malloc(sizeof(WORD_LIST));
    wlp->first = wp;
    if(**linep != '\0' && !ISDELIM(**linep))
    wlp->rest = parse_word_list(linep);
    else
    wlp->rest = NULL;
    return wlp;
}

static COMMAND *parse_command(char **linep) {
    WORD_LIST *wlp = parse_word_list(linep);
    if(wlp == NULL)
    return NULL;
    COMMAND *cp = malloc(sizeof(COMMAND));
    cp->words = wlp;
#ifdef DEBUG
    fprintf(stderr, "parse_command => ");
    unparse_command(cp, stderr);
    fprintf(stderr, " [remaining: '%s' ]\n", *linep);
#endif
    return cp;
}

static COMMAND_LIST *parse_command_list(char **linep) {
    skip_spaces(linep);
    COMMAND *cp = parse_command(linep);
    if(cp == NULL)
    return NULL;
    COMMAND_LIST *clp = malloc(sizeof(COMMAND_LIST));
    clp->first = cp;
    skip_spaces(linep);
    if(**linep == '|') {
    (*linep)++;
    clp->rest = parse_command_list(linep);
    } else {
    clp->rest = NULL;
    }
    return clp;
}

static char *parse_path(char **linep) {
    skip_spaces(linep);
    char *pp = *linep;
    while(**linep != '\0' && !isspace(**linep) && !ISDELIM(**linep))
    (*linep)++;
    char c = **linep;
    **linep = '\0';
    pp = strdup(pp);
    **linep = c;
    return pp;
}

static void parse_redirection(PIPELINE *pp, char **linep) {
    char *path;
    pp->input_path = NULL;
    pp->output_path = NULL;
    skip_spaces(linep);
    char c = **linep;
    if(c == '<' || c == '>') {
    (*linep)++;
    path = parse_path(linep);
    if(path == NULL)
        return;
    }
    if(c == '<') {
    if(pp->input_path != NULL)
        debug("Ambiguous input redirection");
    else
        pp->input_path = path;
    } else if(c == '>') {
    if(pp->output_path != NULL)
        debug("Ambiguous output redirection");
    else
        pp->output_path = path;
    }
}

static PIPELINE *parse_pipeline(char **linep) {
    COMMAND_LIST *clp = parse_command_list(linep);
    if(clp == NULL)
    return NULL;
    PIPELINE *pp = malloc(sizeof(PIPELINE));
    pp->commands = clp;
    parse_redirection(pp, linep);
#ifdef DEBUG
    fprintf(stderr, "parse_pipeline => ");
    unparse_pipeline(pp, stderr);
    fprintf(stderr, " [remaining: '%s' ]\n", *linep);
#endif
    return pp;
}

static PIPELINE_LIST *parse_pipeline_list(char **linep) {
    skip_spaces(linep);
    PIPELINE *pp = parse_pipeline(linep);
    if(pp == NULL)
    return NULL;
    PIPELINE_LIST *plp = malloc(sizeof(PIPELINE_LIST));
    plp->first = pp;
    skip_spaces(linep);
    if(**linep == ';') {
    (*linep)++;
    plp->rest = parse_pipeline_list(linep);
    } else {
    plp->rest = NULL;
    }
    return plp;
}

TASK *parse_task(char **linep) {
    PIPELINE_LIST *plp = parse_pipeline_list(linep);
    skip_spaces(linep);
    if(plp == NULL)
    return NULL;
    if(**linep != '\0') {
    free_pipeline_list(plp);
    return NULL;
    }
    TASK *tp = malloc(sizeof(TASK));
    tp->pipelines = plp;
#ifdef DEBUG
    fprintf(stderr, "parse_task => ");
    unparse_task(tp, stderr);
    fprintf(stderr, " [remaining: '%s' ]\n", *linep);
#endif
    return tp;
}
