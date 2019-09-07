#include <stdio.h>
#include <libvirt/libvirt.h>

#include <stdlib.h>
#include <string.h>

#include "acessoLibVirt.h"


/**
 * Realiza conexcao.
 * @param connectionUrl
 * @return 
 */
virConnectPtr conectar(const char* connectionUrl) {
    virConnectPtr conn = NULL; /* the hypervisor connection */
    conn = virConnectOpen(connectionUrl);
    if (conn == NULL) {
        logErro("Nao foi possivel conectar ao hypervisor. URL: %s\n", connectionUrl);
        virConnectClose(conn);
        return NULL;
    }
    return conn;
}



/**
 * Verifica se o nome do dominio inicia com o nome padrao (definido em VM_NOME_INICIO_PADRAO).
 * @param nomeDominio
 * @param inicioPadraoObrigatorio
 * @return 
 */
int isNomeDominioPadrao(const char* nomeDominio) {
    char* resultado = strstr(nomeDominio, VM_NOME_INICIO_PADRAO);
    if(resultado != NULL) {
        if(resultado == nomeDominio) return 1;
        return 0;
    }
    return 0;
}


/**
 * Ativa dominio. Aceita apenas as duas flags:
 * 1) VIR_CONNECT_LIST_DOMAINS_INACTIVE:
 * Para ativar os dominios inativos (desligados)
 * 
 * 2) VIR_CONNECT_LIST_DOMAINS_PAUSED:
 * Para ativar os dominios pausados
 * 
 * @param conn
 * @param count Quantidade de Dominios solicitados
 * @param flag
 */
void ativaDominios(virConnectPtr conn, int* count, unsigned int flag) {
    virDomainPtr* dominios;
    int numDominios;
    int i;
    numDominios = virConnectListAllDomains(conn, &dominios, flag);
    for(i=0 ; i<numDominios && (*count) > 0 ; i++) {
        const char* nomeDominio = virDomainGetName(dominios[i]);
        
        // Verifica nome do dominio para nao levantar VM errada:
        if(!isNomeDominioPadrao(nomeDominio)) continue;
        
        int codRetorno;
        if(flag == VIR_CONNECT_LIST_DOMAINS_PAUSED) {
            codRetorno = virDomainResume(dominios[i]);
        }
        else if(flag == VIR_CONNECT_LIST_DOMAINS_INACTIVE) {
            codRetorno = virDomainCreate(dominios[i]);
        }
        else {
            logMensagem("Codigo invalido...\n");
        }
        if(codRetorno == 0) {
            logMensagem("Dominio %s ativado com sucesso. Flag=%d\n", nomeDominio, flag);
            (*count)--;
        }
        else {
            logMensagem("Erro ao ativar dominio %s. Flag=%d\n", nomeDominio, flag);
        }
    }
    // Libera ponteiros de dominios:
    for(i=0 ; i<numDominios ; i++) {
        virDomainFree(dominios[i]);
    }
    free(dominios);
}


void pauseDOminio() {
    //virDomainManagedSave();
}



/**
 * Disponibiliza dominios (VMs). A quantidade de VMs eh definida
 * no parametro de entrada "quantidade".
 * @param quantidade
 * @return Quantidade atendida
 */
int disponibilizaDominios(int qtdSolicitada) {
    virConnectPtr conn = NULL;
    int contador = qtdSolicitada;
    conn = conectar(LIB_VIRT_ADDRESS);
    if(conn != NULL) {
        // Tenta primeiro com VMs pausadas:
        ativaDominios(conn, &contador, VIR_CONNECT_LIST_DOMAINS_PAUSED);
        
        // Se ainda for preciso mais VMs, ativa as VMs desligadas:
        if(contador > 0) {
            ativaDominios(conn, &contador, VIR_CONNECT_LIST_DOMAINS_INACTIVE);
        }
        virConnectClose(conn);
        logMensagem("Quantidade solicitada: %d\n", qtdSolicitada);
        logMensagem("Quantidade atendida: %d\n", qtdSolicitada - contador);
        return qtdSolicitada - contador;
    }
    else {
        logMensagem("[disponibilizaDominios]: Erro ao conectar libvirt\n");
        return -1;
    }
}


/**
 * Utilizado para Teste.
 * Ativa todos os dominios.
 */
void ativaDoms() {
    virConnectPtr conn = NULL;
    int contador = 1000;
    virDomainPtr* dominios;
    conn = conectar(LIB_VIRT_ADDRESS);
    if(conn != NULL) {
        ativaDominios(conn, &contador, VIR_CONNECT_LIST_DOMAINS_PAUSED);
        ativaDominios(conn, &contador, VIR_CONNECT_LIST_DOMAINS_INACTIVE);
        virConnectClose(conn);
    }
    else {
        logMensagem("[ativaDoms]: Erro ao conectar libvirt\n");
    }
}



/**
 * Suspende todos os dominios.
 * CPU e IO nao sao utilizados. Mas a memoria alocada permanece.
 * Retorna a quantidade de Dominios interrompidos.
 */
int interrompeDominios(int quantidade, unsigned int status_flag) {
    virConnectPtr conn = NULL;
    conn = conectar(LIB_VIRT_ADDRESS);   
    int qtdInterrompido = 0;
    if(conn != NULL) {
        virDomainPtr* dominios;
        int numDominios;
        int i;
        
        numDominios = virConnectListAllDomains(conn, &dominios, status_flag);
        
        // Verifica se a quantidade de vms em Pausa eh menor que o minimo.
        // Se for, nao desativa.
        if(status_flag == VIR_CONNECT_LIST_DOMAINS_PAUSED && numDominios <= PAUSED_VM_COUNT_MIN) {
            logMensagem("Numero de dominios minimo (%d) alcancado. Nenhum dominio sera desativado.\n", PAUSED_VM_COUNT_MIN);
            quantidade = 0;     // seta 0 apenas para nao entrar no loop
        }

        
        for(i=0 ; i<numDominios && qtdInterrompido<quantidade ; i++) {
            const char* nomeDominio = virDomainGetName(dominios[i]);
            
            // Verifica nome do dominio para nao levantar VM errada:
            if(!isNomeDominioPadrao(nomeDominio)) continue;
            
            if(status_flag == VIR_CONNECT_LIST_DOMAINS_RUNNING) {
                if(virDomainSuspend(dominios[i]) == 0) {
                        // Pausa realizada com sucesso.
                        logMensagem("Dominio %s suspendido com sucesso\n", nomeDominio);
                        qtdInterrompido++;
                }
                else {
                        logMensagem("Falha ao suspender dominio %s\n", nomeDominio);
                }
            }
            else if(status_flag == VIR_CONNECT_LIST_DOMAINS_PAUSED) {
                if(virDomainDestroyFlags(dominios[i], VIR_DOMAIN_DESTROY_GRACEFUL) == 0) {
                    logMensagem("Dominio %s desligado com suceso\n", nomeDominio);
                    qtdInterrompido++;
                }
                else {
                    logMensagem("Falha ao desligar dominio %s.\n", nomeDominio);
                }
            }
        }
        
        // Libera ponteiros de dominios:
        for(i=0 ; i<numDominios ; i++) {
            virDomainFree(dominios[i]);
        }
        free(dominios);
        virConnectClose(conn);
        return qtdInterrompido;
    }
}


/**
 * Verifica se o servidor responsavel pela fila de jobs do condor estah ativo.
 * Precisa estar sempre ativo.
 * @param nomeServidorVirtual
 */
int verificaServidorVirtualEssencial(char* nomeServidorVirtual) {
    virConnectPtr conn = NULL;
    virDomainPtr dom;
    virDomainInfoPtr info;
   
    int codigoRetorno = VM_SERVIDOR_ESSENCIAL_STATUS_DESCONHECIDO;
    
    conn = conectar(LIB_VIRT_ADDRESS);
    if(conn != NULL) {
        dom = virDomainLookupByName(conn, nomeServidorVirtual);
        if(dom == NULL) codigoRetorno = VM_SERVIDOR_ESSENCIAL_STATUS_DESCONHECIDO; // ou Inativo??
        
        virDomainInfoPtr info;

        
        int state;
        int reason;
        if(virDomainGetState(dom, &state, &reason, 0) == 0) {
            logMensagem("Info Servidor Essencial: virDomainState: %d, reason: %d\n", state, reason);
            if (state == VIR_DOMAIN_SHUTOFF || state == VIR_DOMAIN_CRASHED) {
                if(virDomainCreate(dom) == 0) {
                    logMensagem("Dominio Essencial %s ativado com sucesso.\n", nomeServidorVirtual);
                    codigoRetorno = VM_SERVIDOR_ESSENCIAL_STATUS_ATIVO;
                }
                else {
                    logErro("Nao foi possivel ativar Dominio Essencial %s\n", nomeServidorVirtual);
                    codigoRetorno = VM_SERVIDOR_ESSENCIAL_STATUS_INATIVO;
                }
            }
            else if(state == VIR_DOMAIN_PAUSED) {
                if(virDomainResume(dom) == 0) {
                    logMensagem("Dominio Essencial %s retomado com sucesso.\n", nomeServidorVirtual);
                    codigoRetorno = VM_SERVIDOR_ESSENCIAL_STATUS_ATIVO;
                }
                else {
                    logErro("Nao foi possivel retomar Dominio Essencial %s\n", nomeServidorVirtual);
                    codigoRetorno = VM_SERVIDOR_ESSENCIAL_STATUS_INATIVO;
                }
            }
            else {
                // Considera como Ativo
                codigoRetorno = VM_SERVIDOR_ESSENCIAL_STATUS_ATIVO;
            }
        }
        else {
            logErro("[verificaServidorVirtualEssencial]: Erro ao acessar info do servidor.\n");
            codigoRetorno = VM_SERVIDOR_ESSENCIAL_STATUS_DESCONHECIDO;
        }
        
        virDomainFree(dom);
        virConnectClose(conn);
    }
    else {
        logErro("[verificaServidorVirtualEssencial]: Erro ao conectar libvirt\n");
        codigoRetorno = VM_SERVIDOR_ESSENCIAL_STATUS_DESCONHECIDO;
    }
    return codigoRetorno;
}