/* 
 * File:   acessoLibVirt.h
 * Author: rkendy
 *
 * Created on 29 de Abril de 2013, 21:26
 */

#ifndef ACESSOLIBVIRT_H
#define	ACESSOLIBVIRT_H

#ifdef	__cplusplus
extern "C" {
#endif


// Local:    
#define LIB_VIRT_ADDRESS "qemu:///system"


// Producao:    
//#define LIB_VIRT_ADDRESS "qemu+ssh://root@localhost/system"

// Teste kvm03:    
//#define LIB_VIRT_ADDRESS "qemu+ssh://root@kvm03/system"


#define VM_NOME_INICIO_PADRAO "Condor-wn"    
#define VM_SERVIDOR_ESSENCIAL "Servidor-omws-64bits"
#define VM_SERVIDOR_ESSENCIAL_STATUS_ATIVO 1    
#define VM_SERVIDOR_ESSENCIAL_STATUS_INATIVO 2
#define VM_SERVIDOR_ESSENCIAL_STATUS_DESCONHECIDO 3
#define PAUSED_VM_COUNT_MIN 1    

#ifdef	__cplusplus
}
#endif

#endif	/* ACESSOLIBVIRT_H */

