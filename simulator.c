#include "writeOutput.h"
#include "threadFunctions.h"

void main(){

	int iM[MAX_MINERS], cM[MAX_MINERS], tM[MAX_MINERS], rM[MAX_MINERS];				//declare inputs 
	int iT[MAX_TRANSPORTERS];
	int iS[MAX_SMELTERS], cS[MAX_SMELTERS], tS[MAX_SMELTERS];
	int iF[MAX_FOUNDRIES], cF[MAX_FOUNDRIES];
	int producerCapacity = 0;	

	sem_unlink("miner_produced");													//unlink singular semaphores
	sem_unlink("produced");

	minerProduced = sem_open("miner_produced", O_CREAT, 0600, 0);					//init singular semaphores

	char buf[50];			

	scanf("%d", &nM);												
	for(a=1; a<=nM; a++){						
		scanf("%d %d %d %d", &iM[a], &cM[a], &tM[a], &rM[a]);		//get miner inputs
	}

	scanf("%d", &nT);												
	for(a=1; a<=nT; a++){																
		scanf("%d", &iT[a]);										//get transporter inputs
	}	

	scanf("%d", &nS);												
	for(a=1; a<=nS; a++){
		scanf("%d %d %d", &iS[a], &cS[a], &tS[a]);					//get smelter inputs
		producerCapacity += cS[a];
	}

	scanf("%d", &nF);													
	for(a=1; a<=nF; a++){													
	 	scanf("%d %d", &iF[a], &cF[a]);								//get foundry inputs
	 	producerCapacity += cF[a];
	}

	produced = sem_open("produced", O_CREAT, 0600, producerCapacity);

	for(a=1; a<=nM; a++){								
		miners[a].id = a;													//prepare miner inputs			
		miners[a].i = iM[a];
		miners[a].c = cM[a];
		miners[a].t = tM[a];
		miners[a].r = rM[a];
		miners[a].curr = 0;
		miners[a].reserved = 0;
		miners[a].working = 1;

		sprintf(buf, "miner_can_produce%d", a);								//prepare miner semaphores
		sem_unlink(buf);
		canProduce[a] = sem_open(buf, O_CREAT, 0600, miners[a].c);

		sprintf(buf, "minerMutex%d", a);
		sem_unlink(buf);
		minerMutex[a] = sem_open(buf, O_CREAT, 0600, 1);

		pthread_create(miner+a, NULL, Miner, (void *) &miners[a]);			//create miner threads
	}

	for(a=1; a<=nT; a++){
		transporters[a].id = a;															
		transporters[a].i = iT[a];

		pthread_create(transporter+a, NULL, Transporter, (void *) &transporters[a]); //create transporter threads
	}

	for(a=1; a<=nS; a++){
		smelters[a].id = a;																		//prepare smelter inputs
		smelters[a].i = iS[a];
		smelters[a].c = cS[a];
		smelters[a].t = tS[a];
		smelters[a].producedIngots = 0;
		smelters[a].waitingOre = 0;
		smelters[a].working = 1;

		sprintf(buf, "smelter_check_ore%d", a);													//prepare smelter semaphores
		sem_unlink(buf);
		smelterCheckOre[a] = sem_open(buf, O_CREAT, 0600, 0);

		sprintf(buf, "smelterMutex%d", a);
		sem_unlink(buf);
		smelterMutex[a] = sem_open(buf, O_CREAT, 0600, 1);

		pthread_create(smelter+a, NULL, Smelter, (void *) &smelters[a]);						//create smelter threads
	}

	for(a=1; a<=nF; a++){
		foundries[a].id = a;												//prepare foundry inputs
		foundries[a].i = iF[a];
		foundries[a].c = cF[a];
		foundries[a].producedIngots = 0;
		foundries[a].waitingIron = 0;
		foundries[a].waitingCoal = 0;
		foundries[a].working = 1;
		
		sprintf(buf, "foundry_check_ore%d", a);								//prepare foundry semaphores
		sem_unlink(buf);
		foundryCheckOre[a] = sem_open(buf, O_CREAT, 0600, 0);

		sprintf(buf, "foundryMutex%d", a);
		sem_unlink(buf);
		foundryMutex[a] = sem_open(buf, O_CREAT, 0600, 1);

		pthread_create(foundry+a, NULL, Foundry, (void *) &foundries[a]);	//create foundry threads
	}


	for (a=1; a < nM; a++) {						//join miner threads
		pthread_join(miner[a], NULL);
	}
	for (a=1; a < nT; a++) {						//join transporter threads
		pthread_join(transporter[a], NULL);
	}
	for (a=1; a < nS; a++) {						//join smelter threads
		pthread_join(smelter[a], NULL);
	}
	for (a=1; a < nF; a++) {						//join foundry threads
		pthread_join(foundry[a], NULL);
	}

}