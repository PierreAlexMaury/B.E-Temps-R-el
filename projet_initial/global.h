/* 
 * File:   global.h
 * Author: pehladik
 *
 * Created on 12 janvier 2012, 10:11
 */

#ifndef GLOBAL_H
#define	GLOBAL_H

#include "includes.h"

/* @descripteurs des tâches */
extern RT_TASK tServeur;
extern RT_TASK tconnect;
extern RT_TASK tmove;
extern RT_TASK tenvoyer;
extern RT_TASK tbatterie;
extern RT_TASK tcamera;
extern RT_TASK twatchdog;
extern RT_TASK tclean;

/* @descripteurs des mutex */
extern RT_MUTEX mutexEtat;
extern RT_MUTEX mutexMove;
extern RT_MUTEX mutexRobot;
extern RT_MUTEX mutexVerifConnexion;
extern RT_MUTEX mutexServeur;
extern RT_MUTEX mutexArena;
extern RT_MUTEX mutexCamera;

/* @descripteurs des sempahore */
extern RT_SEM semConnecterRobot;
extern RT_SEM semDeplacerRobot;
extern RT_SEM semBatterie;
//extern RT_SEM semWatchdog;
extern RT_SEM semCamera;
extern RT_SEM semBarriere;

/* @descripteurs des files de messages */
extern RT_QUEUE queueMsgGUI;

/* @variables partagées */
extern int etatCommMoniteur;
extern int etatCommRobot;
extern int compteur;
extern int etatCamera;
extern DServer *serveur;
extern DRobot *robot;
extern DMovement *move;
extern DBattery *batterie;
extern DArena *arena;
extern DCamera *camera;

/* @constantes */
extern int MSG_QUEUE_SIZE;
extern int PRIORITY_TSERVEUR;
extern int PRIORITY_TCONNECT;
extern int PRIORITY_TMOVE;
extern int PRIORITY_TENVOYER;
extern int PRIORITY_TBATTERIE;
extern int PRIORITY_TWATCHDOG;
extern int PRIORITY_TCAMERA;
extern int PRIORITY_TCLEAN;

#endif	/* GLOBAL_H */

