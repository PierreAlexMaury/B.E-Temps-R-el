/*
 * File:   global.h
 * Author: pehladik
 *
 * Created on 21 avril 2011, 12:14
 */

#include "global.h"

RT_TASK tServeur;
RT_TASK tconnect;
RT_TASK tmove;
RT_TASK tenvoyer;
RT_TASK tbatterie;
RT_TASK tcamera;
RT_TASK tclean;
//RT_TASK twatchdog;

RT_MUTEX mutexEtat;
RT_MUTEX mutexMove;
RT_MUTEX mutexRobot;
RT_MUTEX mutexVerifConnexion;
RT_MUTEX mutexServeur;
RT_MUTEX mutexArena;
RT_MUTEX mutexCamera;

RT_SEM semConnecterRobot;
RT_SEM semDeplacerRobot;
RT_SEM semBatterie;
RT_SEM semWatchdog;
RT_SEM semCamera;
RT_SEM semBarriere;


RT_QUEUE queueMsgGUI;

int etatCommMoniteur = 1;
int etatCommRobot = 1;
int compteur = 0;
int etatCamera = ACTION_STOP_COMPUTE_POSITION;
DRobot *robot;
DMovement *move;
DServer *serveur;
DBattery *batterie;
DArena *arena;
DCamera *camera;


int MSG_QUEUE_SIZE = 10;

int PRIORITY_TSERVEUR = 70;
int PRIORITY_TCONNECT = 80;
int PRIORITY_TMOVE = 40;
int PRIORITY_TENVOYER = 60;
int PRIORITY_TBATTERIE = 5;
int PRIORITY_TWATCHDOG = 99;
int PRIORITY_TCAMERA = 90;
int PRIORITY_TCLEAN = 98;