/* SERVER */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>	//strlen
#include <sys/socket.h>
#include <arpa/inet.h>	//inet_addr

#include <time.h>
#include "listaFunctions.h"
#include "structsBalistica.h"
#include <stdbool.h>
#include <math.h>

#include <unistd.h>	//write
#include <pthread.h>
#include <semaphore.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ipAconectar "127.0.0.1"
#define porta 5007
#define qtePosicoesParaDisparo 10
#define qteDisparospossiveis 4

#define xAlvo 10000.0
#define yAlvo 10000.0
#define zAlvo 0.0

#define xCanhao 10000.0
#define yCanhao 10000.0
#define zCanhao 0.0
//#define velProjetil 694.444444
// todas as unidades de medidas estao no S.I.

int socket_desc ;
int client_sock ;
int c ;
int read_size ;
struct sockaddr_in server , client;

Info disparo;
bool alvoVoando;
bool podeAtirar;

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t semaforo;

void *threadDeDisparo (void *p);
void processaInfo(Lista *l);
int abreComunicacao();
void printPosicao(Posicao *pos);
void printVelocidade(Vetor *vel);

int main(int argc , char *argv[]){
    srand(time(NULL));

	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1){
		printf("Could not create socket");
	}
	puts("Canhao carregado.   (Socket created)");
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	//server.sin_addr.s_addr = INADDR_ANY;
	server.sin_addr.s_addr = inet_addr(ipAconectar);
	server.sin_port = htons( porta );
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("Radar ativo.   (bind done)");
	
	//Listen
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Escaneado...   (connections)");
	c = sizeof(struct sockaddr_in);
	
	//accept connection from an incoming client
	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0){
		perror("accept failed");
		return 1;
	}
	puts("Alvo detectado!   (connection accepted)");
    alvoVoando = true;
    double tempoInicial = 0.0;
    
    Posicao aviaoPos;
    
    disparo.azinute = 0;
    disparo.elevacao = 0;
    disparo.velDisparo = 0;
    
    Lista *posicoes = malloc(sizeof(Lista));
    inicializaLista(posicoes);
    
    pthread_t processaResposta;
    pthread_create(&processaResposta, NULL, threadDeDisparo, NULL);
	
	//Receive a message from client
	while( (read_size = recv(client_sock , (void*)&aviaoPos , sizeof(Posicao) , 0)) > 0 ){
		//Send the message back to client
        
        printf("tempo: %0.1f posicao x: %0.1f posicao y: %0.1f posicao z: %0.1f \n", tempoInicial, aviaoPos.x, aviaoPos.y, aviaoPos.z);
		fflush(stdout);
        
        pthread_mutex_lock(&server_mutex);
        insereListaF(posicoes, aviaoPos.x, aviaoPos.y, aviaoPos.z, tempoInicial);
        pthread_mutex_unlock(&server_mutex);
        tempoInicial = tempoInicial + 0.5;
        
        if (posicoes->qte >= qtePosicoesParaDisparo) {
            processaInfo(posicoes);
        }
	}
	
	if(read_size == 0){
		puts("Client disconnected");
		fflush(stdout);
	}
	else{
		if(read_size == -1){
		    perror("recv failed");
	    }
	}
    
    esvaziaLista(posicoes);
	free(posicoes);
    
	return 0;
}


void *threadDeDisparo (void *p){
    //    pthread_mutex_lock(&client_mutex);
    //    sem_post(&semaforo);
    //    sem_wait(&semaforo);
    //    pthread_mutex_unlock(&client_mutex);
    int disparosEfetuados = 0;
    do{
        if (podeAtirar && disparosEfetuados < qteDisparospossiveis) {
            pthread_mutex_lock(&server_mutex);
            write(client_sock , (void *)&disparo , sizeof(disparo));
            disparosEfetuados++;
            disparo.azinute = 0;
            disparo.elevacao = 0;
            disparo.velDisparo = 0;
            disparo.instante = 0;
            podeAtirar = false;
            pthread_mutex_unlock(&server_mutex);
        }
        else{
            sem_wait(&semaforo);
        }
    }while(alvoVoando);
    
    pthread_exit(NULL);
}

void processaInfo(Lista *l){
    puts("Preparando para atirar!");
    // encontrando angulo do percurso (assumindo que seja reto)
    // trata o caminho percorrido do aviao como reta entre os pontos recebidos
    double alphas[qtePosicoesParaDisparo - 1];// n posicoes
    double alphaMedio = 0;                    // n - 1 angulos entre
    double varianciaAlpha = 0;
    double desvioPadraoAlpha = 0;

    int i = 0;
    Elemento *e;
    for(e = l->comeco; i < qtePosicoesParaDisparo -1; e = e->proximo){
        alphas[i] = atan( fabs(e->y - e->proximo->y)/ fabs(e->x - e->proximo->x) );
        alphaMedio = alphaMedio + alphas[i];
        i++;
	}
    alphaMedio = alphaMedio/(qtePosicoesParaDisparo - 1); //alpha eh o angulo da reta(1) que o aviao percorre
    
    for (int j = 0; j < qtePosicoesParaDisparo - 1 ; j++) {
        varianciaAlpha = varianciaAlpha + pow(alphaMedio - alphas[j], 2);
    }
    varianciaAlpha = varianciaAlpha/(qtePosicoesParaDisparo - 1);
    desvioPadraoAlpha = sqrt(varianciaAlpha);
    // angulo e desvio padrao encontrados
    
    if (0.02 <= desvioPadraoAlpha) {//tolerancia de 0.02 radianos / 1.145 graus
        pthread_mutex_lock(&server_mutex);
        disparo.azinute = 0;
        disparo.elevacao = 0;
        disparo.velDisparo = 0;
        disparo.instante = 0;
        podeAtirar = false;
        
        for (int j = 0; j < qtePosicoesParaDisparo; j++) {
            retiraLista(l);
        }
        pthread_mutex_unlock(&server_mutex);
        puts("o percurso nao eh uma reta");
        return;
    }
    
    //entrontrando as velocidades do aviao em cada eixo
    double velocidadeX[qtePosicoesParaDisparo - 1];// n - 1 velocidades
    double velocidadeMediaX = 0;
    double velocidadeY[qtePosicoesParaDisparo - 1];
    double velocidadeMediaY = 0;
    double velocidadeZ[qtePosicoesParaDisparo - 1];
    double velocidadeMediaZ = 0;
    
    i = 0;
    for(e = l->comeco; i < qtePosicoesParaDisparo - 1; e = e->proximo){
        velocidadeX[i] = (e->x - e->proximo->x)/ (e->instante - e->proximo->instante);
        velocidadeMediaX = velocidadeMediaX + velocidadeX[i];
        
        velocidadeY[i] = (e->y - e->proximo->y)/ (e->instante - e->proximo->instante);
        velocidadeMediaY = velocidadeMediaY + velocidadeY[i];
        
        velocidadeZ[i] = (e->z - e->proximo->z)/ (e->instante - e->proximo->instante);
        velocidadeMediaZ = velocidadeMediaZ + velocidadeZ[i];
        i++;
	}
    velocidadeMediaX = velocidadeMediaX/(qtePosicoesParaDisparo - 1);
    velocidadeMediaY = velocidadeMediaY/(qtePosicoesParaDisparo - 1);
    velocidadeMediaZ = velocidadeMediaZ/(qtePosicoesParaDisparo - 1);
//    printf("v.x: %0.1f v.y: %0.1f v.z: %0.1f \n", velocidadeMediaX, velocidadeMediaY, velocidadeMediaZ);
//    fflush(stdout);
    //velocidades aproximadas do aviao de cada eixo encontradas
    


    pthread_mutex_lock(&server_mutex);
    while (l->comeco != l->fim) {
        retiraLista(l); //retira todas as posicoes menos a ultima, onde ele supostamente esta agora
    }
    pthread_mutex_unlock(&server_mutex);
    
    double instanteAtual = l->comeco->instante;
    double instanteAbate = instanteAtual + 10.0;
    double instanteDisparo = instanteAtual + 2.0;
    
    Objeto inimigo; //passa a ultima posicao recebida e as velocidades para o struct para eu poder pensar melhor
    inimigo.pos.x = l->comeco->x;
    inimigo.pos.y = l->comeco->y;
    inimigo.pos.z = l->comeco->z;
    inimigo.velocidade.x = velocidadeMediaX;
    inimigo.velocidade.y = velocidadeMediaY;
    inimigo.velocidade.z = velocidadeMediaZ;
    
    Posicao alvo; //passa as constantes para o struct para eu poder pensar melhor
    alvo.x = xAlvo;
    alvo.y = yAlvo;
    alvo.z = zAlvo;
    
    //calcula a posicao do aviao inimigo 10 segundos a frente
    inimigo.pos.x = inimigo.pos.x + inimigo.velocidade.x* ( instanteAbate - instanteAtual );
    inimigo.pos.y = inimigo.pos.y + inimigo.velocidade.y* ( instanteAbate - instanteAtual );
    inimigo.pos.z = inimigo.pos.z + inimigo.velocidade.z* ( instanteAbate - instanteAtual );
    
    Objeto projetil;
    projetil.pos.x = xAlvo;
    projetil.pos.y = yAlvo;
    projetil.pos.z = zAlvo;
    projetil.velocidade.x = (inimigo.pos.x - projetil.pos.x)/(instanteAbate - instanteDisparo);
    projetil.velocidade.y = (inimigo.pos.y - projetil.pos.y)/(instanteAbate - instanteDisparo);
    projetil.velocidade.z = (2*(inimigo.pos.z - projetil.pos.z) + gravidade*pow(instanteAbate - instanteDisparo, 2))/
                                        (2*(instanteAbate - instanteDisparo))  ;
    
    
    printVelocidade(&projetil.velocidade);
    double velProjetil = sqrt( pow(projetil.velocidade.x, 2) +
                               pow(projetil.velocidade.y, 2) +
                               pow(projetil.velocidade.z, 2) );
    printf("v = %0.1f \n",velProjetil*3.6);
    fflush(stdout);
    
    if ( !(1500/3.6 < velProjetil && velProjetil < 2500/3.6) ) {
        pthread_mutex_lock(&server_mutex);
        disparo.azinute = 0;
        disparo.elevacao = 0;
        disparo.velDisparo = 0;
        disparo.instante = 0;
        podeAtirar = false;
        
        retiraLista(l);
        pthread_mutex_unlock(&server_mutex);
        puts("NAO eh posivel disparar");
        return;
    }
    
    double velXY = sqrt( pow(projetil.velocidade.x, 2) + pow(projetil.velocidade.y, 2) );
    
    pthread_mutex_lock(&server_mutex);
    disparo.azinute  = atan( projetil.velocidade.x/projetil.velocidade.y);
    disparo.elevacao = atan( (projetil.velocidade.z)/(velXY));
    disparo.velDisparo = velProjetil;
    disparo.instante = instanteDisparo;
    podeAtirar = true;
    
    printf("azinute: %0.1f elevacao: %0.1f velDisparo: %0.1f instante: %0.1f \n", disparo.azinute, disparo.elevacao, disparo.velDisparo, disparo.instante);
    fflush(stdout);
    
    pthread_mutex_unlock(&server_mutex);
    
    retiraLista(l);
    sem_post(&semaforo);
    return ;
}


void printPosicao(Posicao *pos){
    printf("p.x: %0.1f  p.y: %0.1f p.z: %0.1f \n", pos->x, pos->y, pos->z);
    fflush(stdout);
}

void printVelocidade(Vetor *vel){
    printf("v.x: %0.1f  v.y: %0.1f v.z: %0.1f \n", vel->x, vel->y, vel->z);
    fflush(stdout);
}


