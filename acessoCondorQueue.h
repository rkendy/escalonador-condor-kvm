/* 
 * File:   acessoCondorQueue.h
 * Author: rkendy
 *
 * Created on 29 de Abril de 2013, 09:47
 */

#ifndef ACESSOCONDORQUEUE_H
#define	ACESSOCONDORQUEUE_H

#ifdef	__cplusplus
extern "C" {
#endif

// Local:
#define CONDOR_QUEUE_COMMAND "condor_q -format \"%s|\" GlobalJobId -format \"%s|\" CMD -format \"%s|\" Owner -format \"%d|\" QDate -format \"%d\\n\" JobStatus"
#define CONDOR_HISTORY_COMMAND "condor_history -backwards -match 1 -format \"%s\" GlobalJobId"
    
    
// Producao:
//#define CONDOR_QUEUE_COMMAND "ssh root@omws.ic.uff.br \"condor_q -format \\\"%s|\\\" GlobalJobId -format \\\"%s|\\\" CMD -format \\\"%s|\\\" Owner -format \\\"%d|\\\" QDate -format \\\"%d\\n\\\" JobStatus\""
//#define CONDOR_HISTORY_COMMAND "ssh root@omws.ic.uff.br \"condor_history -backwards -match 1 -format \\\"%s\\\" GlobalJobId\""    
    
    
    
//#define CONDOR_QUEUE_COMMAND "ssh rkendy@kvm01 \"condor_q -format \\\"%s|\\\" GlobalJobId -format \\\"%s|\\\" CMD -format \\\"%s|\\\" Owner -format \\\"%d|\\\" QDate -format \\\"%d\\n\\\" JobStatus\""
//#define CONDOR_QUEUE_COMMAND "ssh rkendy@kvm01 \"condor_q -format \\\"%s|\\\" GlobalJobId\" "
// producao: ssh root@omws.ic.uff.br ...
    


#define MAX_OUTPUT_LINE_SIZE 1028
#define CONDOR_QUEUE_SPLIT_CHAR "|"
#define JOB_STATUS_HELD 5
#define JOB_STATUS_IDLE 1    
#define JOB_STATUS_COMPLETED 4
#define JOB_STATUS_RUNNING 2
    
#define MAX_COUNT_READER_JOB_WAITING 2
#define MAX_COUNT_READER_NO_RUNNING_JOBS 10
    

    
// condor_q -format "%s\n" GlobalJobId
// condor_q -format "%s|" GlobalJobId -format "%f\n" QDate
// condor_q -format "%s|" GlobalJobId -format "%s|" CMD -format "%s|" Owner -format "%f|" QDate -format "%d\n" JobStatus
    
    
// HoldReason: utlizar para distinguir entre hold por erro ou hold por nao possuir recursos suficientes
// LastRemoteHost: seria o host atual?
    
/*
 * Job Status
 * JobStatus in job ClassAds
 * 0	Unexpanded	U
 * 1	Idle	I
 * 2	Running	R
 * 3	Removed	X
 * 4	Completed	C
 * 5	Held	H
 * 6	Submission_err	E
 * 
 * Qual a diferenca entre H e I??
 * Buscar jobs com status "5"
*/


/**
 * Informacoes do job.
 */    
typedef struct {
    char globalId[50];             /* GlobalJobId */
    char owner[50];                /* Owner do job */
    long submitted;             /* Data do envio */
    int status;                 /* Buscar por 1 ou 5? */
} condorJobType;


/**
 * No da lista.
 */
typedef struct node {
    condorJobType info;         /* Info do job */
    int readCount;              /* Contador do job */
    long initialRead;           /* Timestamp da primeira leitura */
    int toErase;                /* Indica se deve ser removido ou nao */
    //int pendente;               /* Indica se houve tentativa de atender o job */
    struct node *proximo;       /* Ponteiro para o proximo da lista */
} node;

typedef struct {
    node *primeiro;
    node *ultimo;
    int count;
} listaBase;

#ifdef	__cplusplus
}
#endif

#endif	/* ACESSOCONDORQUEUE_H */

