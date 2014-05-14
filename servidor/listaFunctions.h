//
//  ListaFunctions.h
//  servidor
//
//  Created by Alessandro Camillo Gimenez de Menezes on 25/04/14.
//  Copyright (c) 2014 Alessandro Camillo Gimenez de Menezes. All rights reserved.
//

#ifndef servidor_listaFunctions_h
#define servidor_listaFunctions_h

typedef struct _Elemento {
    double x;
    double y;
    double z;
    double instante;
    struct _Elemento *proximo;
    struct _Elemento *anterior;
}Elemento;

typedef struct {
    Elemento *comeco, *fim;
    int qte;
}Lista;

void inicializaLista(Lista *l);
//void imprimeLista(Lista *l) ;
void insereListaP(Lista *l, double x, double y, double z, double t);
void retiraLista (Lista *l);
void insereListaF(Lista *l, double x, double y, double z, double t);


void inicializaLista(Lista *l) {
    l->comeco = NULL;
    l->fim = NULL;
    l->qte = 0;
    return;
}

void imprimeLista(Lista *l) {
    Elemento *e;
    for(e = l->comeco; e != NULL; e = e->proximo){
        printf("posicao x: %0.2f posicao y: %0.2f posicao z: %0.2f \n", e->x, e->y, e->z);
		fflush(stdin);
	}
    printf("\n");
    return;
}

void insereListaP(Lista *l, double x, double y, double z, double t){
    Elemento *novo = malloc(sizeof(Elemento));
    novo->x = x;
    novo->y = y;
    novo->z = z;
    novo->instante = t;
    
    l->qte = l->qte + 1;
    novo->proximo = l->comeco;
    novo->anterior = NULL;
    
    if (l->comeco != NULL){
   		l->comeco->anterior = novo;
    }
    
    l->comeco = novo;
    
    if (l->fim == NULL){
   		l->fim = novo;
    }
    
    return;
}

void retiraLista(Lista *l) {
    //int valor;
    if (l->comeco != NULL){
        l->qte = l->qte - 1;
   		Elemento *velho = l->comeco;
   		//valor = velho->valor;
  		l->comeco = velho->proximo;
 		free(velho);
   		return;//(valor);
        
    }else{
   		return;//-1;
    }
}

void insereListaF(Lista *l, double x, double y, double z, double t){
    Elemento *novo = malloc(sizeof(Elemento));
    Elemento *ultimo = l->fim;
    novo->x = x;
    novo->y = y;
    novo->z = z;
    novo->instante = t;
    l->qte = l->qte + 1;
    novo->proximo = NULL;
    novo->anterior = ultimo;
    
    if(l->comeco == NULL){
        l->comeco = novo;
    }
    
    if(ultimo != NULL){
        ultimo->proximo = novo;
    }
    
    l->fim = novo;
    return;
}

void esvaziaLista(Lista *l){
    while (l->comeco != NULL) {
        retiraLista(l);
    }
}

#endif
