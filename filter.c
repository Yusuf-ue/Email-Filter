#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct _word_filter word_filter;
typedef struct _domain_filter domain_filter;
typedef struct _length_filter length_filter;
typedef struct _report report;
typedef struct _email email;

struct _word_filter{
    char word[20];
    int score;
    int count;
};

struct _domain_filter{
    char domain[20];
    int score;
    int count;
};

struct _length_filter{
    int length;
    int score;
    int count;
};

struct _report{
    word_filter w_f[20];
    domain_filter d_f[20];
    length_filter l_f;
    int wf_count;
    int df_count;
    int lf_count;
    int score;
};

struct _email{
    char sender[40];
    char recipient[40];
    char body[2000];
    report r;
};

void print_report(report r){
    printf("Report: \n");
    printf("|----------------------------------------");

    printf("\tWortfilter: \n");
    printf("|----------------------------------------");
    for(int i = 0; i < r.wf_count; i++){
	printf("\t\t|%d.| %s | Häufigkeit in der EMail: %d\n", i+1, r.w_f[i].word, r.w_f[i].count);
    }
    printf("|----------------------------------------");
    printf("\t\tAnzahl der Wortfilter: %d\n", r.wf_count);
    printf("|----------------------------------------");

    printf("\tDomainfilter: \n");
    for(int i = 0; i < r.df_count; i++){
	printf("\t\t|%d.| %s | Häufigkeit in der EMail: %d\n", i+1, r.d_f[i].domain, r.d_f[i].count);
    }
    printf("|----------------------------------------");
    printf("\tAnzahl der Domainfilter: %d\n", r.df_count);
    printf("|----------------------------------------");

    printf("\tLängenfilter: \n");
    for(int i = 0; i < r.lf_count; i++){
	printf("\t\t%d.| %d\n", i+1, r.l_f.length);
    }
    printf("|----------------------------------------");
    printf("\tAnzahl der Längenfilter: %d\n", r.lf_count);
    printf("|----------------------------------------");

    printf("--> Bewertung: %d\n", r.score);
    printf("|----------------------------------------");

    return;
}

void print_email(email em, report r){
    printf("E-Mail: \n");
    printf("|----------------------------------------");

    printf("Sender: \t%s\n", em.sender);
    printf("|----------------------------------------");

    printf("Recipient: \t%s\n", em.recipient);
    printf("|----------------------------------------");

    printf("Inhalt: \n");
    printf("\n");
    printf("%s \n", em.body);
    printf("|----------------------------------------");

    print_report(r);

    return;
}

int read_email(char* filename, email em){
    int success = 0;
    
    char *header_sender = "[[sender:]]";
    char *header_recipient = "[[recipient:]]";
    char *header_body = "[[body:]]";
    char *header = NULL;
    char *line = NULL;

    FILE* fp = fopen(filename, "r");

    fscanf(fp, "%s\n", header);
    if(strncmp(header_sender, header, 12) == 0){
	fscanf(fp, "%s\n", em.sender);
    }else{
	success = 1;
	return success;
    }

    fscanf(fp, "%s\n", header);
    if(strncmp(header_recipient, header, 15) == 0){
	fscanf(fp, "%s\n", em.recipient);
    }else{
	success = 1;
	return success;
    }

    fscanf(fp, "%s\n", header);
    if(strncmp(header_body, header, 10) == 0){
	while(fscanf(fp, "%s\n", line) != EOF){
	    int n = strlen(line);
	    strncat(em.body, line, n);
	}
    }else{
	success = 1;
	return success;
    }

    return success;
}

int read_config(char* filename, word_filter wf[], domain_filter df[], length_filter lf, report r){
    int success = 0;

    char *typ = NULL;
    char *filter = NULL;
    int *score = NULL;
    int wf_count = 0;
    int df_count = 0;
    int lf_count = 0;

    FILE* fp = fopen(filename, "r");

    while(fscanf(fp, "%c %s %d", typ, filter, score) != EOF){
	if(strncmp(typ, "w", 1) == 0){
	    strncpy(wf[wf_count].word, filter, 20);
	    wf[wf_count].score = *score;
	    wf_count = wf_count + 1;   
	}else if(strncmp(typ, "d", 1) == 0){
	    strncpy(df[df_count].domain, filter, 20);
	    df[df_count].score = *score;
	    df_count = df_count + 1;
	}else if(strncmp(typ, "l", 1) == 0){
	    if(lf_count > 0){
		success = 1;
		return success;
	    }
	    lf.length = atoi(filter);
	    lf.score = *score;
	    lf_count = lf_count + 1;
	}else{
	    success = 1;
	    return success;
       
	}
    }

    r.wf_count = wf_count;
    r.df_count = df_count;
    r.lf_count = lf_count;
    r.score = 0;

    return success;
}

void scoring(email em, report r, word_filter wf[], domain_filter df[], length_filter lf){
    int len = strlen(em.body);
    if(len > lf.length){
	r.score = r.score + lf.score;
    }

    char *saveptr = NULL;
    char *domain = NULL;
    char *name = NULL;
    name = strtok_s(em.recipient, "@", &saveptr);
    domain = strtok_s(NULL, "@", &saveptr);
    for(int i = 0; i < r.df_count; i++){
	if(strncmp(domain, df[i].domain, 20) == 0){
	    r.score = r.score + df[i].score;
	    df[i].count = df[i].count + 1;
	}
    }

    for(int i = 0; i < r.wf_count; i++){
	char *ptr = strstr(em.body, wf[i].word);
	while(ptr != NULL){
	    r.score = r.score + wf[i].score;
	    wf[i].count = wf[i].count + 1;
	    int str_i = ptr - em.body;
	    ptr = strstr(&em.body[str_i], wf[i].word);
	}
    }
}

int main(int argc, char** args){
    char *email_datei = NULL;
    char *config_datei = NULL;
    char temp;

    printf("\nBitte geben Sie den Namen der Konfigurationsdatei ein. [z.B. config.txt]\n");
    scanf("%s", &config_datei, &temp);  //war davor ohne & bei zweiter stelle
    printf("checkpoint 1\n");           //debugging print
    printf("\nBitte geben Sie den Namen der EMail-Datei ein. [z.B. email.txt]\n");
    scanf("%s%c", &email_datei, &temp);  //war davor ohne & bei zweiter stelle

    word_filter wf[20];
    domain_filter df[20];
    length_filter lf;
    email em;
    report r;
    printf("checkpoint 2\n");
    read_email(email_datei, em);
    printf("checkpoint 3\n");
    read_config(config_datei, wf, df, lf, r);
    scoring(em, r, wf, df, lf);
    print_report(r);
    print_email(em, r);

    return 0;
    
}
