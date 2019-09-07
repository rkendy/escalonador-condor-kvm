#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "acessoCondorQueue.h"


/**
 * Inicia lista.
 * @param lista
 */
listaBase inicializaLista(listaBase lista) {
    lista.count = 0;
    lista.primeiro = NULL;
    lista.ultimo = NULL;
    return lista;
}


/**
 * Retorna verdadeiro se status for referente ao aguardando recurso.
 * @param status
 * @return 
 */
int isStatusAguardandoExecucao(int status) {
    return (status == JOB_STATUS_IDLE);
}

/**
 * Retorna verdadeiro se status for referente ao em execucao.
 * @param status
 * @return 
 */
int isStatusEmExecucao(int status) {
    return (status == JOB_STATUS_RUNNING);
}

/**
 * 
 * @param lista
 */
void printLista(listaBase lista) {
    int i;
    node *proximoPonteiro;
    proximoPonteiro = lista.primeiro;
    for(i=0 ; i<lista.count ; i++) {
        logMensagemPuro("Count: %d\t", proximoPonteiro->readCount);
        proximoPonteiro = proximoPonteiro->proximo;
    }
    logMensagemPuro("\n");
}


/**
 * 
 * @param lista
 */
void printListaPonteiro(listaBase lista) {
    node *proximoPonteiro;
    proximoPonteiro = lista.primeiro;
    while(proximoPonteiro != NULL) {
        logMensagemPuro("\tContador: %d\n", proximoPonteiro->readCount);
        proximoPonteiro = proximoPonteiro->proximo;
    }
}


/**
 * Copia as informacoes do job no No criado.
 * @param origem
 * @param destino
 */
void copiaInfo(condorJobType origem, condorJobType *destino) {
    strcpy(destino->globalId, origem.globalId);
    strcpy(destino->owner, origem.owner);
    destino->status = origem.status;
    destino->submitted = origem.submitted;
}

/**
 * Remove no da lista.
 * @param lista
 * @param ponteiroNo
 */
void removeJobFromList(listaBase *lista, node *ponteiroNo, node *anterior) {
    
    if(lista->primeiro == ponteiroNo) {
        // Primeiro da lista sendo removido:
        lista->primeiro = ponteiroNo->proximo;
    }
    else {
        // Atualiza o proximo do anterior:
        anterior->proximo = ponteiroNo->proximo;
    }
    // Verifica se o no a ser removido eh o ultimo:
    if(lista->ultimo == ponteiroNo) {
        lista->ultimo = anterior;
    }
    free(ponteiroNo);
    lista->count--;
}


/**
 * Procura job de id na lista de jobs ja criados.
 * @param lista
 * @param id
 * @return 
 */
node* findById(listaBase *lista, char *id) {
    node *ponteiroNo;
    ponteiroNo = lista->primeiro;
    while(ponteiroNo != NULL) {
        if(strcmp(ponteiroNo->info.globalId, id) == 0) {
            // Job ja existe na lista
            return ponteiroNo;
        }
        ponteiroNo = ponteiroNo->proximo;
    }
    return ponteiroNo;
}

/**
 * Insere novo job (No) na lista.
 * @param lista
 * @param info
 */
void insereLista(listaBase *lista, condorJobType info) {
    node *temp;
    
    if(lista->primeiro == NULL) {
        // Lista vazia:
        lista->primeiro = malloc(1*sizeof(node));
        lista->ultimo = lista->primeiro;
    }
    else {
        // Lista nao vazia:
        temp = malloc(1*sizeof(node));
        lista->ultimo->proximo = temp;
        lista->ultimo = temp;
    }
    
    lista->ultimo->proximo = NULL;      // Insere sempre no final. Assim o proximo eh sempre NULL
    lista->ultimo->readCount = 1;       // Como eh novo, inicia com contador de leitura como 1
    lista->ultimo->toErase = 0;         // Nao deve ser removido por enquanto
    lista->ultimo->initialRead = getSystemTimestamp();  // Timestamp inicial
    lista->count++;                     // Incrementa contador de elementos da lista.
    
    // Copia as informacoes do job para o elemento da lista:
    copiaInfo(info, &lista->ultimo->info);
}


/**
 * Verifica se jobInfo eh novo ou se ja existe na lista.
 * Se for novo (seu id nao encontrado na lista), insere na lista.
 * Se ja existe, verifica se os status sao iguais. 
 * Se sao iguais, aumenta do job (indicador que precisa de VM).
 * Se sao diferentes, remove da lista.
 * @param lista
 * @param jobInfo
 */
void verificaNovoItem(listaBase *lista, condorJobType jobInfo) {
    node *ponteiroNo;
    ponteiroNo = findById(lista, jobInfo.globalId);
    if(ponteiroNo != NULL) {
        ponteiroNo->readCount++;        // Incrementa contador de leitura do job
        ponteiroNo->toErase = 0;        // Seta para nao remover o job da lista
    }
    else {
        // Item eh novo. Insere na lista:
        insereLista(lista, jobInfo);
    }    
}



/**
 * Recebe linha de saida do comando "condor_history" e popula condorJob.
 * No caso, popula apenas o globalId.
 * @param st
 * @param condorJob
 */
void readIdFromCondorHistoryOutput(char *st, condorJobType *condorJob) {
    char *ch;
    ch = strtok(st, CONDOR_QUEUE_SPLIT_CHAR);
    strcpy(condorJob->globalId, ch);
}

/**
 * Recebe linha de saida do comando "condor_q" e separa as informacoes.
 * Ignora linha com status diferente de "5" (ou 1)??
 * @param st
 */
int readInfoJobFromCondorOutput(char *st, condorJobType *condorJob) {
    
    char *ch;
    ch = strtok(st, CONDOR_QUEUE_SPLIT_CHAR);
    char *globalId = ch;
    
    ch = strtok(NULL, CONDOR_QUEUE_SPLIT_CHAR);
    char *command = ch;
    
    ch = strtok(NULL, CONDOR_QUEUE_SPLIT_CHAR);
    char *owner = ch;
    
    ch = strtok(NULL, CONDOR_QUEUE_SPLIT_CHAR);
    long submitted = atoi(ch);
    
    ch = strtok(NULL, CONDOR_QUEUE_SPLIT_CHAR);
    int status = atoi(ch);
    
    if(isStatusAguardandoExecucao(status) || isStatusEmExecucao(status)) {
        strcpy(condorJob->globalId, globalId);
        strcpy(condorJob->owner, owner);
        condorJob->status = status;
        condorJob->submitted = submitted;
        return 1;
    }
    return 0;
}



/**
 * Marca todos os elementos da lista para serem removidos.
 * @param lista
 */
void marcaElementosAntigos(listaBase *lista) {
    node *ponteiro;
    ponteiro = lista->primeiro;
    while(ponteiro != NULL) {
        ponteiro->toErase = 1;
        ponteiro = ponteiro->proximo;
    }
}


/**
 * Remove elementos marcados da lista.
 * @param lista
 */
void removeElementosAntigos(listaBase *lista) {
    node *ponteiroNo;
    node *proximo, *anterior;
    ponteiroNo = lista->primeiro;
    anterior = NULL;
    while(ponteiroNo != NULL) {
        proximo = ponteiroNo->proximo;
        if(ponteiroNo->toErase == 1) {
            removeJobFromList(lista, ponteiroNo, anterior);
        }
        else {
            anterior = ponteiroNo;
        }
        ponteiroNo = proximo;
    }
}




/**
 * Le lista do condor e cria lista com os jobs que estao em status pendente.
 * @param lista
 */
void condor_queue(listaBase *listaAguardando, listaBase *listaEmExecucao) {

    FILE *pipein_fp;
    char readbuf[MAX_OUTPUT_LINE_SIZE];

    /* Create one way pipe line with call to popen() */
    if (( pipein_fp = popen(CONDOR_QUEUE_COMMAND, "r")) == NULL) {
        logMensagem("Erro ao executar comando condor_q\n");
        perror("popen");
    }
    
    /* Seta todos os elementos para serem removidos */
    marcaElementosAntigos(listaAguardando);
    marcaElementosAntigos(listaEmExecucao);
    
    /* Processa as informacoes retornadas pelo comando condor_q */
    condorJobType item;
    while(fgets(readbuf, MAX_OUTPUT_LINE_SIZE, pipein_fp)) {
        if(readInfoJobFromCondorOutput(readbuf, &item)) {
            if(isStatusAguardandoExecucao(item.status)) {
                verificaNovoItem(listaAguardando, item);
            }
            else if(isStatusEmExecucao(item.status))  {
                verificaNovoItem(listaEmExecucao, item);
            }
        }
    }
    
    // Remover os outros itens da lista:
    removeElementosAntigos(listaAguardando);
    removeElementosAntigos(listaEmExecucao);
    
    /* Close the pipes */
    pclose(pipein_fp);
}




/**
 * Retorna a quantidade de recursos necessarios, ou seja
 * quantas VMs devem ser ativadas.
 * Um recurso eh necessario quando o job estah aguardando execucao e seu
 * contador eh maior que maxCount.
 * @param lista
 * @param maxCount
 * @return 
 */
int qtdJobsAguardandoExecucao(listaBase lista, int maxCount) {
    node* ponteiro;
    int contadorRecursos;
    contadorRecursos = 0;
    ponteiro = lista.primeiro;
    while(ponteiro != NULL) {
        if(ponteiro->readCount > maxCount) {
            contadorRecursos++;
        }
        ponteiro = ponteiro->proximo;
    }
    return contadorRecursos;
}



/**
 * Verifica a lista de historico e retorna o job mais recente, ou seja, o job
 * executado mais recente.
 * @param ultimoNoExecutado
 */
void condor_history(node *ultimoNoExecutado) {
    FILE *pipein_fp;
    char readbuf[MAX_OUTPUT_LINE_SIZE];

    /* Create one way pipe line with call to popen() */
    if (( pipein_fp = popen(CONDOR_HISTORY_COMMAND, "r")) == NULL) {
        logMensagem("Erro ao executar comando condor_q\n");
        perror("popen");
    }
    
   /* Processa as informacoes retornadas pelo comando condor_q */
    condorJobType item;
    while(fgets(readbuf, MAX_OUTPUT_LINE_SIZE, pipein_fp)) {
        readIdFromCondorHistoryOutput(readbuf, &item);
        
        if(strcmp(ultimoNoExecutado->info.globalId, item.globalId) == 0) {
            // Ultimo Job eh igual. Aumenta seu contador:
            ultimoNoExecutado->readCount++;
        }
        else {
            // Ultimo Job eh diferente. Considera o job lido como ultimo e reseta contador:
            // Copia os valores lidos em "item" para "ultimoNoExecutado.info":
            copiaInfo(item, &ultimoNoExecutado->info);
            ultimoNoExecutado->readCount = 0;
        }
        
        logMensagem("ID do ultimo job do historico[contador]: %s[Contador: %d]\n", item.globalId, ultimoNoExecutado->readCount);
    }
    
    /* Close the pipes */
    pclose(pipein_fp);    
    
}


void teste() {
    condorJobType item;
    strcpy(item.globalId, "yyy");
    strcpy(item.owner, "zzz");
    
    item.status = 0;
    item.submitted = 0;
    
    listaBase lista = inicializaLista(lista);
    marcaElementosAntigos(&lista);
    removeElementosAntigos(&lista);
    
    marcaElementosAntigos(&lista);
    verificaNovoItem(&lista, item);
    removeElementosAntigos(&lista);
    
    marcaElementosAntigos(&lista);
    removeElementosAntigos(&lista);
}

void condor_queue_teste(listaBase *listaAguardando, listaBase *listaEmExecucao, condorJobType item) {
    marcaElementosAntigos(listaAguardando);
    marcaElementosAntigos(listaEmExecucao);
    
    if(isStatusAguardandoExecucao(item.status)) {
        verificaNovoItem(listaAguardando, item);
    }
    else if(isStatusEmExecucao(item.status))  {
        verificaNovoItem(listaEmExecucao, item);
    }
    removeElementosAntigos(listaAguardando);
    removeElementosAntigos(listaEmExecucao);
}
