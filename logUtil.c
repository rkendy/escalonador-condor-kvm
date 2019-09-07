#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#pragma GCC diagnostic ignored "-Wformat-security"

static FILE *log = NULL;
static time_t mytime;
static int dayLog;

/**
 * Utilizado acessoCondorQueue. Armazena timestamp inicial.
 * @return 
 */
long getSystemTimestamp() {
    mytime = time(NULL);
    return (unsigned)mytime;
}


/**
 * Retorna o dia associado a geracao do arquivo de log (nome do arquivo).
 * @return 
 */
int getDiaAtual() {
    mytime = time(NULL);
    struct tm *Tm;
    Tm=localtime(&mytime);
    return Tm->tm_mday;
    //return Tm->tm_min;
}


/**
 * Retorna dia em que foi gerado o log atual.
 * Valor obtido na gerado do arquivo (nome do arquivo).
 * @return 
 */
int getDiaLog() {
    return dayLog;
}


/**
 * Gera nome do arquivo de log.
 * Seta dayLog, dia associado da geracao do log.
 * @param str
 */
void geraNomeArquivo(char *str) {
    mytime = time(NULL);
    struct tm *Tm;
    Tm=localtime(&mytime);
    dayLog = Tm->tm_mday;
    //dayLog = Tm->tm_min;
    sprintf(str, "log-%04d%02d%02d-%02d%02d.log", Tm->tm_year+1900, Tm->tm_mon+1, Tm->tm_mday, Tm->tm_hour, Tm->tm_min);
}



/**
 * Gera string da hora para uma mensagem de log.
 * Eh gerado sempre que logar uma mensagem.
 * @param str
 */
void geraDataHoraStr(char *str) {
    mytime = time(NULL);
    struct tm *Tm;
    Tm=localtime(&mytime);
    sprintf(str, "[%04d%02d%02d %02d:%02d:%02d] ", Tm->tm_year+1900, Tm->tm_mon+1, Tm->tm_mday, Tm->tm_hour, Tm->tm_min, Tm->tm_sec);
}


void logErro(const char *format, ...) {
    va_list arglist;
    va_start( arglist, format );  
    char dataHoraStr[100];
    geraDataHoraStr(dataHoraStr);
    fprintf(log, "[ERRO]: ");
    fprintf(log, dataHoraStr);
    vfprintf(log, format, arglist);
    va_end( arglist );
    fflush(log);
}

/**
 * Loga mensagem.
 * @param format
 * @param ...
 */
void logMensagem(const char *format, ...) {
    //verificaArquivoLog();
    va_list arglist;
    va_start( arglist, format );  
    char dataHoraStr[100];
    geraDataHoraStr(dataHoraStr);
    fprintf(log, dataHoraStr);
    vfprintf(log, format, arglist);
    va_end( arglist );
    fflush(log);
}


/**
 * Gera log sem timestamp.
 * @param format
 * @param ...
 */
void logMensagemPuro(const char *format, ...) {
    //verificaArquivoLog();
    va_list arglist;
    va_start( arglist, format );
    vfprintf(log, format, arglist);
    va_end( arglist );
    fflush(log);
}



/**
 * Abre arquivo. Gera novo nome de arquivo log.
 * Detalhe: abrindo com a opcao "w" gerava erro estranho:
 * O arquivo era criado, mas ao tentar fazer "tail -f", o processo parava
 * na abertura do popen.
 * Se tail for executado em outra sessao, nao ocorre o travamento. 
 * Possivel causa: popen com STDOUT, misturado com terminal sh e tail -f.
 */
void abreLog() {
    char logFilename[300];
    geraNomeArquivo(logFilename);
    log = fopen(logFilename, "a");
    printf("Arquivo log gerado com sucesso: %s\n", logFilename);
}


/**
 * Fecha o arquivo.
 */
void fechaLog() {
    fclose(log);
    log = NULL;
}
