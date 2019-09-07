/* 
 * File:   main.c
 * Author: rkendy
 *
 * Created on 29 de Abril de 2013, 09:26
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <libvirt/libvirt.h>
#include <string.h>

#include "acessoCondorQueue.h"
#include "acessoLibVirt.h"

#define SLEEP_TIME_SEM_ACAO 60
#define SLEEP_TIME_COM_ATIVACAO_DE_DOMINIO 120
#define SLEEP_TIME_COM_DESATIVACAO_DE_DOMINIO 120

#define GERENCIA_MAQUINAS_VIRTUAIS 0



/*
 * 
 */
int main(int argc, char** argv) {

    abreLog();
    int diaLog = getDiaLog();
    logMensagem("Iniciando escalonador ...\n");
    logDefs();
    
    
    int qtdJobsAguardandoExec;
    int qtdAtendida;
    int sleepTime = SLEEP_TIME_SEM_ACAO;
    int codigoStatus;
    node ultimoJobHistorico;
    
    listaBase listaAguardandoExecucao = inicializaLista(listaAguardandoExecucao);
    listaBase listaEmExecucao = inicializaLista(listaEmExecucao);
    
    while(1) {
        // Verifica se o servidor que controla a fila do condor estah ativo:
        codigoStatus = verificaServidorVirtualEssencial(VM_SERVIDOR_ESSENCIAL);
        if(codigoStatus == VM_SERVIDOR_ESSENCIAL_STATUS_ATIVO ||
                codigoStatus == VM_SERVIDOR_ESSENCIAL_STATUS_DESCONHECIDO) {
        
            /**
             * Verifica fila do condor, e monta 2 listas.
             * Uma com os jobs aguardando recursos e outra de jobs em execucao.
             * A primeira lista eh utilizada para levantar VMs.
             * A segunda lista eh utilizada para baixar VMs.
             */
            condor_queue(&listaAguardandoExecucao, &listaEmExecucao);
            
            
            /**
             * Verifica se existem jobs aguardando tempo suficiente (MAX_COUNT_READER)
             * para que sejam ativados novos recursos.
             * Apenas 1 recurso eh ativado, independente do numero de jobs aguardando.
             * Se houver ativacao, o processo tem periodo de sleep diferente.
             */
            qtdJobsAguardandoExec = qtdJobsAguardandoExecucao(listaAguardandoExecucao, MAX_COUNT_READER_JOB_WAITING);
            
            logMensagem("Quantidade de Jobs aguardando execucao: %d\n", listaAguardandoExecucao.count);
            logMensagem("Quantidade de Jobs a serem ativados: %d\n", qtdJobsAguardandoExec);
            logMensagem("Quantidade de Jobs em execucao: %d\n", listaEmExecucao.count);
            
            if(qtdJobsAguardandoExec > 0) {
                // Ativa apenas um recurso:
                if(GERENCIA_MAQUINAS_VIRTUAIS == 1) {
                    qtdAtendida = disponibilizaDominios(1);
                    if(qtdAtendida > 0) {
                        sleepTime = SLEEP_TIME_COM_ATIVACAO_DE_DOMINIO;
                    }
                }
            }
            else {
                // ToDo: desativar mesmo tendo jobs em execucao???
                /**
                 * Verifica se existem jobs em execucao.
                 * Se a lista for vazia, verifica o ultimo job executado.
                 */
                if(listaEmExecucao.count == 0 && listaAguardandoExecucao.count == 0) {

                    condor_history(&ultimoJobHistorico);

                    if(GERENCIA_MAQUINAS_VIRTUAIS == 1) {
                        if(ultimoJobHistorico.readCount > MAX_COUNT_READER_NO_RUNNING_JOBS) {
                            int quantidade = 1;
                            // Tenta pausar maquinas em execucao:
                            int qtdInterrompido = interrompeDominios(quantidade, VIR_CONNECT_LIST_DOMAINS_RUNNING);
                            // Se foi interrompido, zera o contador.
                            if(qtdInterrompido > 0) {
                                ultimoJobHistorico.readCount = 0;
                            }

                            // Caso nao consiga interromper na chamada acima, busca por VMs em pausa e faz shutdown:
                            if(qtdInterrompido < quantidade) {
                                // Tenta desligar maquinas pausadas:
                                quantidade = interrompeDominios(quantidade, VIR_CONNECT_LIST_DOMAINS_PAUSED);
                                if(quantidade > 0) {
                                    sleepTime = SLEEP_TIME_COM_DESATIVACAO_DE_DOMINIO;
                                }
                            }
                        }
                    }
                }
                else {
                    // Nao faz nada...
                    // ToDo: consultar quais maquinas estao sendo utilizadas e tentar
                    // suspender as outras que nao estao sendo utilizadas (condor_status)?
                }
            }
        }
        else {
            sleepTime = SLEEP_TIME_SEM_ACAO;
        }
        
        sleep(sleepTime);
        // Verifica se o dia mudou. Se mudou, entao fecha o log atual
        // e gera novo arquivo referente ao novo dia.
        int novoDiaLog = getDiaAtual();
        if(diaLog != novoDiaLog) {
            logMensagem("Fechando arquivo de log");
            fechaLog();
            abreLog();
            diaLog = getDiaLog();
        }
    }
    fechaLog();
    return (EXIT_SUCCESS);
}

/**
 * Loga as principais definicoes.
 */
void logDefs() {
    logMensagem("==== Definicoes Tempo Sleep:\n");
    logMensagem("SLEEP_TIME_SEM_ACAO: %d\n", SLEEP_TIME_SEM_ACAO);
    logMensagem("SLEEP_TIME_COM_ATIVACAO_DE_DOMINIO: %d\n", SLEEP_TIME_COM_ATIVACAO_DE_DOMINIO);
    logMensagem("SLEEP_TIME_COM_DESATIVACAO_DE_DOMINIO: %d\n", SLEEP_TIME_COM_DESATIVACAO_DE_DOMINIO);
    
    logMensagem("==== Definicoes Contador:\n");
    logMensagem("MAX_COUNT_READER_JOB_WAITING: %d\n", MAX_COUNT_READER_JOB_WAITING);
    logMensagem("MAX_COUNT_READER_NO_RUNNING_JOBS: %d\n", MAX_COUNT_READER_NO_RUNNING_JOBS);
    
    logMensagem("==== Definicoes VMs e Contador Minimo:\n");
    logMensagem("VM_NOME_INICIO_PADRAO: %s\n", VM_NOME_INICIO_PADRAO);
    logMensagem("VM_SERVIDOR_ESSENCIAL: %s\n", VM_SERVIDOR_ESSENCIAL);
    logMensagem("PAUSED_VM_COUNT_MIN: %d\n", PAUSED_VM_COUNT_MIN);
}