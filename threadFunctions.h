#ifndef HW2_THREADFUNCTIONS_H
#define HW2_THREADFUNCTIONS_H

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

// #include "priorityq.h"

#define MAX_MINERS 50
#define MAX_TRANSPORTERS 50
#define MAX_SMELTERS 50
#define MAX_FOUNDRIES 50


int nM, nT, nS, nF, a, b, minerStopped, semcheck, remainingOre=0;

struct minerData																			//create input structs
{
	int id, i, c, r, curr, reserved, working;
	OreType t;
};
struct smelterData
{
	int id, i, c, producedIngots, waitingOre, working;
	OreType t;
};

struct transporterData
{
	int id, i;
};
struct foundryData
{
	int id, i, c, producedIngots, waitingIron, waitingCoal, working;
};

sem_t *canProduce[MAX_MINERS], *minerProduced;								//declare semaphores
sem_t *smelterCheckOre[MAX_SMELTERS], *foundryCheckOre[MAX_FOUNDRIES], *produced;
sem_t *minerMutex[MAX_MINERS], *smelterMutex[MAX_SMELTERS], *foundryMutex[MAX_FOUNDRIES];

pthread_t miner[MAX_MINERS], transporter[MAX_TRANSPORTERS], smelter[MAX_SMELTERS], foundry[MAX_FOUNDRIES];	//declare thread arrays
	
struct minerData miners[MAX_MINERS];							//declare input structs
struct transporterData transporters[MAX_TRANSPORTERS];
struct smelterData smelters[MAX_SMELTERS];
struct foundryData foundries[MAX_FOUNDRIES];	

struct timespec ts;

void *Miner(void *input){																			//miner thread function
	struct minerData *x = (struct minerData *) input;
	struct MinerInfo *y = (struct MinerInfo*)malloc(sizeof(struct MinerInfo));

	FillMinerInfo(y, x->id, x->t, x->c, x->curr);
	WriteOutput(y, NULL, NULL, NULL, MINER_CREATED);												//miner created
	
	while(x->r){																					//while remaining...
		
		sem_wait(canProduce[x->id]);	
																			//if there's storage space
		sem_wait(minerMutex[x->id]);
											//mutex
		FillMinerInfo(y, x->id, x->t, x->c, miners[x->id].curr);
		WriteOutput(y, NULL, NULL, NULL, MINER_STARTED);
	
		sem_post(minerMutex[x->id]);	
											//miner started
		usleep(x->i - (x->i*0.01) + (rand()%(int)(x->i*0.02)));
		sem_wait(minerMutex[x->id]);
		miners[x->id].curr++;
		remainingOre++;
		x->r--;
		sem_post(minerProduced);
		FillMinerInfo(y, x->id, x->t, x->c, miners[x->id].curr);
		WriteOutput(y, NULL, NULL, NULL, MINER_FINISHED);						
		sem_post(minerMutex[x->id]);
																		//release mutex
		usleep(x->i - (x->i*0.01) + (rand()%(int)(x->i*0.02)));																		
	}

	sem_wait(minerMutex[x->id]);
	FillMinerInfo(y, x->id, x->t, x->c, miners[x->id].curr);	
	WriteOutput(y, NULL, NULL, NULL, MINER_STOPPED);
	minerStopped++;		
	miners[x->id].working = 0;		
	sem_post(minerMutex[x->id]);								//miner stopped
}

void *Smelter(void *input){																	//smelter thread function
	struct smelterData *x = (struct smelterData *) input;
	struct SmelterInfo *y = (struct SmelterInfo*)malloc(sizeof(struct SmelterInfo));

	FillSmelterInfo(y, x->id, x->t, x->c, smelters[x->id].waitingOre, smelters[x->id].producedIngots);													
	WriteOutput(NULL, NULL, y, NULL, SMELTER_CREATED);										//smelter created

	while(1){																				
		
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += 5;
						
		semcheck = sem_timedwait(smelterCheckOre[x->id], &ts);
		if(semcheck == -1){
	     	if (errno == ETIMEDOUT){
	     		break;}}
       			
		// sem_wait(smelterCheckOre[x->id]);
		sem_wait(smelterMutex[x->id]);
	
		if(smelters[x->id].waitingOre>=2){																		//if enough ores...
			smelters[x->id].waitingOre--;smelters[x->id].waitingOre--;	
			FillSmelterInfo(y, x->id, x->t, x->c, smelters[x->id].waitingOre, smelters[x->id].producedIngots);
			WriteOutput(NULL, NULL, y, NULL, SMELTER_STARTED);
			sem_post(smelterMutex[x->id]);

			usleep(x->i - (x->i*0.01) + (rand()%(int)(x->i*0.02)));

			sem_wait(smelterMutex[x->id]);
			smelters[a].producedIngots++;
			sem_post(produced);
			sem_post(produced);
			FillSmelterInfo(y, x->id, x->t, x->c, smelters[x->id].waitingOre, smelters[x->id].producedIngots);							//smelter finished
			WriteOutput(NULL, NULL, y, NULL, SMELTER_FINISHED);
		}
		sem_post(smelterMutex[x->id]);	
															//release mutex
	}
	
	sem_wait(smelterMutex[x->id]);	
	smelters[x->id].working = 0;
	FillSmelterInfo(y, x->id, x->t, x->c, smelters[x->id].waitingOre, smelters[x->id].producedIngots);
	WriteOutput(NULL, NULL, y, NULL, SMELTER_STOPPED);	
	sem_post(smelterMutex[x->id]);	
									//smelter stopped
}

void *Foundry(void *input){
	struct foundryData *x = (struct foundryData *) input;										//foundry thread function
	struct FoundryInfo *y = (struct FoundryInfo*)malloc(sizeof(struct FoundryInfo));

	FillFoundryInfo(y, x->id, x->c, foundries[x->id].waitingIron, foundries[x->id].waitingCoal, foundries[x->id].producedIngots);						//foundry created
	WriteOutput(NULL, NULL, NULL, y, FOUNDRY_CREATED);

	while(1){							
	
		clock_gettime(CLOCK_REALTIME, &ts);

		ts.tv_sec += 5;
						
		semcheck = sem_timedwait(foundryCheckOre[x->id], &ts);
		if(semcheck == -1){
	    	if (errno == ETIMEDOUT){
	    		break;}}

		// sem_wait(foundryCheckOre[x->id]);

		sem_wait(foundryMutex[x->id]);	
	    if(foundries[x->id].waitingIron && foundries[x->id].waitingCoal){
		    
			FillFoundryInfo(y, x->id, x->c, foundries[x->id].waitingIron, foundries[x->id].waitingCoal, foundries[x->id].producedIngots);
			WriteOutput(NULL, NULL, NULL, y, FOUNDRY_STARTED);	
			sem_post(foundryMutex[x->id]);										//foundry started
		
			usleep(x->i - (x->i*0.01) + (rand()%(int)(x->i*0.02)));

			sem_wait(foundryMutex[x->id]);
			foundries[x->id].producedIngots++;
			foundries[x->id].waitingIron--;		foundries[x->id].waitingCoal--;										
			sem_post(produced);

			FillFoundryInfo(y, x->id, x->c, foundries[x->id].waitingIron, foundries[x->id].waitingCoal, foundries[x->id].producedIngots);
			WriteOutput(NULL, NULL, NULL, y, FOUNDRY_FINISHED);										//foundry finished													//release mutex
		}
		sem_post(foundryMutex[x->id]);	
	}

	sem_wait(foundryMutex[x->id]);
	FillFoundryInfo(y, x->id, x->c, foundries[x->id].waitingIron, foundries[x->id].waitingCoal, foundries[x->id].producedIngots);						//foundry stopped
	WriteOutput(NULL, NULL, NULL, y, FOUNDRY_STOPPED);
	foundries[x->id].working = 0;
	sem_post(foundryMutex[x->id]);	
}

void *Transporter(void *input){
	struct transporterData *x = (struct transporterData *) input;																//transporter thread function
	struct TransporterInfo *y = (struct TransporterInfo*)malloc(sizeof(struct TransporterInfo));							
	struct MinerInfo *t = (struct MinerInfo*)malloc(sizeof(struct MinerInfo));
	struct SmelterInfo *s = (struct SmelterInfo*)malloc(sizeof(struct SmelterInfo));
	struct FoundryInfo *f = (struct FoundryInfo*)malloc(sizeof(struct FoundryInfo));

	int lastMiner = 1;
	int visitedSmelter, ironWait, coalWait;
	int producer;

	OreType *carrying = NULL;

	FillTransporterInfo(y, x->id, carrying);																					//transporter created
	WriteOutput(NULL, y, NULL, NULL, TRANSPORTER_CREATED);

	while(minerStopped!=nM || remainingOre){

		visitedSmelter = 0;
		sem_wait(minerProduced);
	
		while(1){		
	
			sem_wait(minerMutex[lastMiner]);
		
			if(miners[lastMiner].curr && !miners[lastMiner].reserved){

				FillMinerInfo(t, miners[lastMiner].id, 0, 0, 0);																				//transporter travels to found miner
				FillTransporterInfo(y, x->id, carrying);
				WriteOutput(t, y, NULL, NULL, TRANSPORTER_TRAVEL);
				miners[lastMiner].reserved = 1;

				sem_post(minerMutex[lastMiner]);
				usleep(x->i - (x->i*0.01) + (rand()%(int)(x->i*0.02)));

				sem_wait(minerMutex[lastMiner]);

				miners[lastMiner].curr--;	
				remainingOre--;																									//take ore
				carrying = &(miners[lastMiner].t);
				
				FillMinerInfo(t, miners[lastMiner].id, miners[lastMiner].t, miners[lastMiner].c, miners[lastMiner].curr);			
				FillTransporterInfo(y, x->id, carrying);
				WriteOutput(t, y, NULL, NULL, TRANSPORTER_TAKE_ORE);																			//transporter take ore
				miners[lastMiner].reserved = 0;
																							//release mutex
				sem_post(canProduce[lastMiner]);	
				sem_post(minerMutex[lastMiner]);																								//signal miner
				usleep(x->i - (x->i*0.01) + (rand()%(int)(x->i*0.02)));

				break;	
			}
			else{
						sem_post(minerMutex[lastMiner]);	}

			if(lastMiner==nM){
					lastMiner=0;
				}
				lastMiner++;

		}

		sem_wait(produced);
		for(producer=1; producer<=nS; producer++){
			sem_wait(smelterMutex[producer]);

			if(smelters[producer].c > smelters[producer].waitingOre && smelters[producer].t == *carrying && smelters[producer].working){

				FillSmelterInfo(s, producer, *carrying, 0, 0, 0);
				FillTransporterInfo(y, x->id, carrying);
				WriteOutput(NULL, y, s, NULL, TRANSPORTER_TRAVEL);

				sem_post(smelterMutex[producer]);
				usleep(x->i - (x->i*0.01) + (rand()%(int)(x->i*0.02)));

				sem_wait(smelterMutex[producer]);

				FillSmelterInfo(s, producer, *carrying, smelters[producer].c, smelters[producer].waitingOre+1, smelters[producer].producedIngots);
				FillTransporterInfo(y, x->id, carrying);
				WriteOutput(NULL, y, s, NULL, TRANSPORTER_DROP_ORE);
				sem_post(smelterMutex[producer]);

				usleep(x->i - (x->i*0.01) + (rand()%(int)(x->i*0.02)));

				sem_wait(smelterMutex[producer]);
				smelters[producer].waitingOre++;
				sem_post(smelterCheckOre[producer]);

				visitedSmelter = 1;
			
				sem_post(smelterMutex[producer]);

				break;

			}		
			else{
				sem_post(smelterMutex[producer]);
			}

		}

		if(!visitedSmelter){

			for(producer=1; producer<=nF; producer++){
				sem_wait(foundryMutex[producer]);
				if(((foundries[producer].c > foundries[producer].waitingIron && *carrying==0) || (foundries[producer].c > foundries[producer].waitingCoal &&  *carrying==2)) && foundries[producer].working){

					FillFoundryInfo(f, producer, 0, 0, 0, 0);
					FillTransporterInfo(y, x->id, carrying);
					WriteOutput(NULL, y, NULL, f, TRANSPORTER_TRAVEL);
					sem_post(foundryMutex[producer]);
					usleep(x->i - (x->i*0.01) + (rand()%(int)(x->i*0.02)));

					sem_wait(foundryMutex[producer]);
					
					if(*carrying==0){
						ironWait = foundries[producer].waitingIron + 1;
					}
					else{
						coalWait = foundries[producer].waitingCoal + 1;
					}

					FillFoundryInfo(f, producer, foundries[producer].c, ironWait, coalWait, foundries[producer].producedIngots);
					FillTransporterInfo(y, x->id, carrying);
					WriteOutput(NULL, y, NULL, f, TRANSPORTER_DROP_ORE);
					sem_post(foundryMutex[producer]);

					usleep(x->i - (x->i*0.01) + (rand()%(int)(x->i*0.02)));

					sem_wait(foundryMutex[producer]);

					if(*carrying==0){
						foundries[producer].waitingIron++;
					}
					else{
						foundries[producer].waitingCoal++;
					}
					sem_post(foundryCheckOre[producer]);
					break;
				}
				sem_post(foundryMutex[producer]);
			}
		}
	}
	FillTransporterInfo(y, x->id, carrying);
	WriteOutput(NULL, y, NULL, NULL, TRANSPORTER_STOPPED);
}
#endif