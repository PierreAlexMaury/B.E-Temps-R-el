#include "fonctions.h"
#include 

int write_in_queue(RT_QUEUE *msgQueue, void * data, int size);

int echec = 0;

void envoyer(void * arg) {
    DMessage *msg;
    int status;
    int err;
    int retour;

    while (1) {
        rt_printf("tenvoyer : Attente d'un message à envoyer\n");
      
        
        if ((err = rt_queue_read(&queueMsgGUI, &msg, sizeof (DMessage), TM_INFINITE)) >= 0) {
/*
            if(echec) break; //unblock + break ici ne break pas reellement
*/
            
            
            //Etat de la communication
            rt_mutex_acquire(&mutexEtat, TM_INFINITE);
            status = etatCommMoniteur;
            rt_mutex_release(&mutexEtat);
            
            //Si ok en envoie le message
            if (status == STATUS_OK) {
                rt_printf("tenvoyer : envoi d'un message au moniteur\n");
                rt_mutex_acquire(&mutexServeur, TM_INFINITE);
                retour = serveur->send(serveur, msg);
                rt_mutex_release(&mutexServeur);
                
                if (retour <= 0) {
                     if (err = rt_task_start(&tclean, &cleanComMoniteur, NULL)) {
                        rt_printf("Error task start clean: %s\n", strerror(-err));
                        exit(EXIT_FAILURE);
                    }  
                }
                msg->free(msg);
            }
            

        } else if (!echec) {
            rt_printf("Error msg queue write: %s\n", strerror(-err));
        }
    }
    rt_printf("************* FIN ENVOYER *************\n\n");
}

void connecter(void * arg) {
    int status;
    DMessage *message;

    rt_printf("tconnect : Debut de l'exécution de tconnect\n");

    while (1) {
        rt_printf("tconnect : Attente du sémarphore semConnecterRobot\n");
        rt_sem_p(&semConnecterRobot, TM_INFINITE);
        if(echec) break;
        //récupération de l'état de la communication avec le robot
        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        status = etatCommRobot;
        rt_mutex_release(&mutexEtat);

        if (status != STATUS_OK) {
            rt_printf("tconnect : Ouverture de la communication avec le robot\n");
            
            rt_mutex_acquire(&mutexRobot, TM_INFINITE);
            status = robot->open_device(robot);
            rt_mutex_release(&mutexRobot);
            
            //envoi du résultat de la tentative de connexion
            message = d_new_message();
            message->put_state(message, status);

            if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                message->free(message);
            } 
            
            //MAJ du nb de perte de connexions avec le robot
            verifierNbConnexion(status);
            
            if (status == STATUS_OK){
                rt_printf("tconnect : Robot connecté\n");
                
                do {
                    rt_printf("tconnect : tentavive de démarrage du robot\n");
                    rt_mutex_acquire(&mutexRobot, TM_INFINITE);
                    //status = robot->start_insecurely(robot);
                    status = robot->start_insecurely(robot);
                    //ici robot start alors commenter start_insecurely
                    rt_mutex_release(&mutexRobot);
                
                } while (status != STATUS_OK);
               
                //MAJ du nb de perte de connexions avec le robot
                verifierNbConnexion(status);
                rt_printf("tconnect : Robot démarré\n");
                //lancer watchdog
                //rt_sem_v(&semWatchdog);
                /*On libère le sémaphore pour que deplacer puisse se lancer*/
                rt_sem_v(&semDeplacerRobot);
                /*On libère le sémaphore pour que niveaBatterie puisse se lancer*/
                rt_sem_v(&semBatterie);
                
            }
            
        }
        else {
            rt_printf("tconnect : Robot déjà déconnecté, rien à faire\n");
        }        
    }
     rt_sem_v(&semBarriere);
    rt_printf("************* FIN CONNECTER *************\n\n");
}

void communiquer(void *arg) {
    DMessage *msg = d_new_message();
    int num_msg = 0;
    int status;
    int size = 1;
    int err;
    

    rt_printf("tserver : Début de l'exécution de serveur\n");
    
    while (!echec) {
        
        //if (echec) break;
        
        rt_mutex_acquire(&mutexServeur, TM_INFINITE);
        status = serveur->open(serveur, "8000");
        rt_mutex_release(&mutexServeur);
        rt_printf("tserver : Connexion\n");
        
        if (status == STATUS_OK) {
            rt_printf("tserver: on a STATUS_OK\n");
            
            /*Liberation du semaphore Camera*/
            rt_sem_v(&semCamera);
            rt_mutex_acquire(&mutexEtat, TM_INFINITE);
            etatCommMoniteur = STATUS_OK;
            rt_mutex_release(&mutexEtat);
            while (size > 0) {
               
                //Pas protégé c        extern int size;ar send l'ai déjà, on évite donc l'interblocage
                rt_printf("tserver : Attente d'un message à lire\n");
                size = serveur->receive(serveur, msg);
                num_msg++;
            
                if (size > 0) {
                    switch (msg->get_type(msg)) {
                    case MESSAGE_TYPE_ACTION:
                        rt_printf("tserver : Le message %d reçu est une action\n",
                                num_msg);
                        DAction *action = d_new_action();
                        action->from_message(action, msg);
                        switch (action->get_order(action)) {
                            case ACTION_CONNECT_ROBOT:
                                rt_printf("tserver : Action connecter robot\n");
                                rt_sem_v(&semConnecterRobot);
                                break;
                            case ACTION_FIND_ARENA:
                                rt_printf("tserver : ACTION_FIND_ARENA\n");
                                rt_mutex_acquire(&mutexCamera,TM_INFINITE);
                                etatCamera = ACTION_FIND_ARENA;
                                rt_mutex_release(&mutexCamera);
                                break;
                            case ACTION_ARENA_FAILED:
                                rt_printf("tserver : ACTION_ARENA_FAILED\n");
                                rt_mutex_acquire(&mutexCamera,TM_INFINITE);
                                etatCamera = ACTION_ARENA_FAILED;
                                rt_printf("tserver : Attente d'un message à lire\n");rt_mutex_release(&mutexCamera);
                                break;
                            case ACTION_ARENA_IS_FOUND:
                                rt_printf("tserver : ACTION_ARENA_IS_FOUND\n");
                                rt_mutex_acquire(&mutexCamera,TM_INFINITE);
                                etatCamera = ACTION_ARENA_IS_FOUND;
                                rt_mutex_release(&mutexCamera);
                                break;
                             case ACTION_COMPUTE_CONTINUOUSLY_POSITION:
                                rt_printf("tserver : ACTION_COMPUTE_CONTINUOUSLY_POSITION\n");
                                rt_mutex_acquire(&mutexCamera,TM_INFINITE);
                                etatCamera = ACTION_COMPUTE_CONTINUOUSLY_POSITION;
                                rt_mutex_release(&mutexCamera);
                                break;
                             case ACTION_STOP_COMPUTE_POSITION:
                                rt_printf("tserver : ACTION_STOP_COMPUTE_POSITION\n");
                                rt_mutex_acquire(&mutexCamera,TM_INFINITE);
                                etatCamera = ACTION_STOP_COMPUTE_POSITION;
                                rt_mutex_release(&mutexCamera);
                                break;
                        }
                        break;
                    case MESSAGE_TYPE_MOVEMENT:
                        rt_printf("tserver : Le message reçu %d est un mouvement\n",
                                num_msg);
                        rt_mutex_acquire(&mutexMove, TM_INFINITE);
                        move->from_message(move, msg);
                        //move->print(move); surcharge affichage
                        rt_mutex_release(&mutexMove);
                        break;
                    }
                }
                else {
                    if (err = rt_task_start(&tclean, &cleanComMoniteur, NULL)) {
                        rt_printf("Error task start clean: %s\n", strerror(-err));
                        exit(EXIT_FAILURE);
                    }                 
                }
             }        
        }
        else {
            rt_printf("tserveur : STATUS_KO, problème ouverture serveur\n");
        } 
    }
    rt_sem_v(&semBarriere);
    rt_printf("************* FIN DE COMMUNIQUER *************\n\n");
}

void deplacer(void *arg) {
    int status = 1;        extern int size;
    int gauche;
    int droite;

    rt_printf("tmove : Debut de l'éxecution de periodique à 200ms\n");
    rt_task_set_periodic(NULL, TM_NOW, 200000000);
    
    
    rt_sem_p(&semDeplacerRobot,TM_INFINITE);
    
    while (!echec) {
        /* Attente de l'activation périodique */
        rt_task_wait_period(NULL);

        //if (echec) break;
        
        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        status = etatCommRobot;
        rt_mutex_release(&mutexEtat);

        if (status == STATUS_OK) {
            rt_printf("tmove : En Mouvement\n");
            rt_mutex_acquire(&mutexMove, TM_INFINITE);
            switch (move->get_direction(move)) {
                case DIRECTION_FORWARD:
                    if(move->get_speed(move)<=75){                     
                        gauche = MOTEUR_ARRIERE_LENT;
                        droite = MOTEUR_ARRIERE_LENT;
                    }else{
                        gauche = MOTEUR_ARRIERE_RAPIDE;
                        droite = MOTEUR_ARRIERE_RAPIDE;
                    }
                    break;
                case DIRECTION_LEFT:
                    if(move->get_speed(move)<=75){
                        gauche = MOTEUR_ARRIERE_LENT;
                        droite = MOTEUR_AVANT_LENT;
                    }else{
                        gauche = MOTEUR_ARRIERE_RAPIDE;
                        droite = MOTEUR_AVANT_RAPIDE;
                    }
                    break;
                case DIRECTION_RIGHT:
                    if(move->get_speed(move)<=75){
                        gauche = MOTEUR_AVANT_LENT;
                        droite = MOTEUR_ARRIERE_LENT;
                    }else{
                        gauche = MOTEUR_AVANT_RAPIDE;
                        droite = MOTEUR_ARRIERE_RAPIDE;
                    }
                    break;
                case DIRECTION_STOP:
                    gauche = MOTEUR_STOP;
                    droite = MOTEUR_STOP;
                    break;
                case DIRECTION_STRAIGHT:
                    if(move->get_speed(move)<=75){
                        gauche = MOTEUR_AVANT_LENT;
                        droite = MOTEUR_AVANT_LENT;
                    }else{
                        gauche = MOTEUR_AVANT_RAPIDE;
                        droite = MOTEUR_AVANT_RAPIDE;
                    }
                    break;
                default:
                    break;
            }
            rt_mutex_release(&mutexMove);
            
            rt_mutex_acquire(&mutexRobot, TM_INFINITE);
            status = robot->set_motors(robot, gauche, droite);
            rt_mutex_release(&mutexRobot);
            
            verifierNbConnexion(status);
        }
    }
    rt_sem_v(&semBarriere);
    rt_printf("************* FIN DE DEPLACER *************\n\n");
}

void niveaubatterie(void *arg){
    
    //communication avec robot
    int status; 
    //niveau batterie inconnu
    int niv = -1;
    DMessage *message;
    
    rt_sem_p(&semBatterie,TM_INFINITE);
    if(!echec) rt_printf("tbatterie : Debut de l'éxecution de periodique à 250ms\n");
    rt_task_set_periodic(NULL, TM_NOW, 250000000);
    
    while(!echec){
        /* Attente de l'activation périodique */
        rt_task_wait_period(NULL);
        
        //if (echec) break;
        
        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        status = etatCommRobot;
        rt_mutex_release(&mutexEtat);
        
        //Si communication ok avec robot
        if(status == STATUS_OK){
            //récupération niv batterie
            rt_mutex_acquire(&mutexRobot, TM_INFINITE);
            status=robot->get_vbat(robot, &niv);
            rt_mutex_release(&mutexRobot);
            //verification nb perte de connexions
            verifierNbConnexion(status);
            
            //Si récupération ok
            if(status == STATUS_OK){
                batterie->set_level(batterie,niv);
                
                //envoi message d'info niveau batterie au moniteur
                message = d_new_message();
                message->put_battery_level(message,batterie);
                rt_printf("tbatterie : Envoi message niveau\n");
                
                if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                message->free(message);
                }                 
            }
        }
    }
    rt_sem_v(&semBarriere);
    rt_printf("************* FIN ETAT BATTERIE *************\n\n");
}

int write_in_queue(RT_QUEUE *msgQueue, void * data, int size) {
    void *msg;
    int err;

    msg = rt_queue_alloc(msgQueue, size);
    memcpy(msg, &data, size);

    if ((err = rt_queue_send(msgQueue, msg, sizeof (DMessage), Q_NORMAL)) < 0) {
        rt_printf("Error msg queue send: %s\n", strerror(-err));
    }
    rt_queue_free(&queueMsgGUI, msg);

    return err;
}

void cleanComMoniteur() {
    //communication perdue

    rt_mutex_acquire(&mutexEtat, TM_INFINITE);
    etatCommMoniteur = 1;//perdu
    etatCommRobot = 1;
    rt_mutex_release(&mutexEtat);
     
    
    echec = 1;
    rt_printf("==================CLEAN MONITEUR =================\n");
   
    rt_sem_v(&semConnecterRobot); //on debloque connecter 
    
    rt_task_unblock(&tcamera);
    rt_task_unblock(&tmove);   
    rt_task_unblock(&tbatterie);
    rt_task_delete(&tenvoyer); //seul et unique solution
    //rt_task_unblock(&tenvoyer); //l'ordre ne change rien 
    //if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0); //le mess seul engendre segfault
    
       
    rt_sem_p(&semBarriere,TM_INFINITE); //*5 car 5 threads qui doivent libérer leur sem (sauf envoyer qu'on kill)
    rt_sem_p(&semBarriere,TM_INFINITE);
    rt_sem_p(&semBarriere,TM_INFINITE); 
    rt_sem_p(&semBarriere,TM_INFINITE); 
    rt_sem_p(&semBarriere,TM_INFINITE); 
      
    d_movement_free(move);
    d_robot_free(robot);
    d_battery_free(batterie);
    d_server_close(serveur);
    d_server_free(serveur);
    d_camera_close(camera);
    d_camera_free(camera);
    d_arena_free(arena);
     
    rt_queue_flush(&queueMsgGUI);
    
    reinitStruct();
    echec = 0;
        
    startTasks();
    
    rt_printf("=================Tous les threads sont terminés ====================\n");
    
   
}

/*TODO: faire un return pour plus tard
 * Modifier communiquer avec les actions
 */

int verifierNbConnexion(int status) {
    DMessage *message;
    rt_printf("================Vérification nb perte de connexions======\n");
    rt_mutex_acquire(&mutexVerifConnexion, TM_INFINITE);
    
    if (status == STATUS_OK) {
        rt_printf("verifierNbConnexion : Tudo bem, tudo bem\n");
        
        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        etatCommRobot = status;
        rt_mutex_release(&mutexEtat);
        
        compteur = 0;
        
    }
    else {
        compteur++;
        
        rt_printf("verifierNbConnexion : COMPTEUR = %d  \n",compteur);
        if (compteur >= 3) {
            message = d_new_message();
            message->put_state(message, status);

            rt_printf("verifierNbConnexion : Envoi message d'erreur\n");
            if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
               message->free(message);
            }      
            //au bout de trois pertes on mets étatCommRobot dans un status d'erreur
            rt_mutex_acquire(&mutexEtat, TM_INFINITE); //je demande au robot de redémarrer au bout de trois pertes
            etatCommRobot = status;
            rt_mutex_release(&mutexEtat);
        }
        
        //status pas ok, on tente une reconnexion dans tout les cas
        rt_sem_v(&semConnecterRobot);
    }
    
    rt_mutex_release(&mutexVerifConnexion);

}


void t_camera (void *arg) {
    DImage *image;
    DJpegimage *jpeg;
    DMessage *message;
    DPosition *position;
    int status;
    int action;
    int backup = ACTION_STOP_COMPUTE_POSITION;
    
    rt_task_set_periodic(NULL, TM_NOW, 600000000); //pour une image plus fluide
   
    rt_sem_p(&semCamera, TM_INFINITE);
    
    rt_mutex_acquire(&mutexCamera, TM_INFINITE);
    camera->open(camera);
    rt_mutex_release(&mutexCamera);
    
    while(!echec) {
        
        rt_task_wait_period(NULL);
        
        //if ( echec ) break;
        
        
        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        status = etatCommMoniteur;
        rt_mutex_release(&mutexEtat);
        
        if (status == STATUS_OK) {
            rt_mutex_acquire(&mutexCamera, TM_INFINITE);
            action = etatCamera;
            rt_mutex_release(&mutexCamera);
            
            switch (action) {
                case  ACTION_STOP_COMPUTE_POSITION:
                    rt_printf("tcamera : ACTION_STOP_COMPUTE_POSITION detected\n");
                    image = d_new_image();
                    camera->get_frame(camera,image);
                    
                    if (image != NULL) {
                        jpeg = d_new_jpegimage();
                        jpeg->compress(jpeg,image);
                        
                        message = d_new_message();
                        message->put_jpeg_image(message,jpeg);
                        
                        if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                                message->free(message);
                        }
                        
                        backup = action;
                        
                        image->free(image);
                        jpeg->free(jpeg);
                    }
                    break; 
                    
                case ACTION_FIND_ARENA:
                    rt_printf("tcamera : ACTION_FIND_ARENA detected\n");
                    image = d_new_image();
                    jpeg = d_new_jpegimage();
                    
                    camera->get_frame(camera,image);
                    
                    if (image != NULL) {
                        jpeg = d_new_jpegimage();
                        
                        rt_mutex_acquire(&mutexArena,TM_INFINITE);
                        arena = image->compute_arena_position(image);
                        
                        if (arena != NULL) {
                            d_imageshop_draw_arena(image,arena);
                            jpeg->compress(jpeg,image);
                            
                            message = d_new_message();
                            message->put_jpeg_image(message,jpeg);
                        
                            if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                                    message->free(message);
                            }
                        }
                        
                        rt_mutex_release(&mutexArena);
                        
                        image->free(image);
                        jpeg->free(jpeg);
                    }
                    
                    
                    break;
                    
                case ACTION_ARENA_IS_FOUND:
                    rt_printf("tcamera : ACTION_ARENA_IS_FOUND detected\n");
                    
                    rt_mutex_acquire(&mutexCamera, TM_INFINITE);
                    etatCamera = backup;
                    rt_mutex_release(&mutexCamera);
                    
                    break;
                    
                case ACTION_ARENA_FAILED:
                    rt_printf("tcamera : ACTION_ARENA_FAILED detected\n");
                    
                    rt_mutex_acquire(&mutexCamera, TM_INFINITE);
                    etatCamera = backup;
                    rt_mutex_release(&mutexCamera);
                    
                    rt_mutex_acquire(&mutexArena, TM_INFINITE);
                    arena->free(arena);
                    arena = NULL;
                    rt_mutex_release(&mutexArena);
                    
                    break;
                    
                case ACTION_COMPUTE_CONTINUOUSLY_POSITION:
                    rt_printf("tcamera : ACTION_COMPUTE_CONTINUOUSLY_POSITION detected\n");
                    image = d_new_image();
                    camera->get_frame(camera,image);
                    
                    if (image != NULL) {
                        
                        rt_mutex_acquire(&mutexArena,TM_INFINITE);
                        position = image->compute_robot_position(image,arena);
                        rt_mutex_release(&mutexArena);
                        
                        
                        if (position != NULL) {
                           d_imageshop_draw_position(image,position);
                           message = d_new_message();
                           message->put_position(message,position);
                       
                           if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                               message->free(message);
                           }
                           jpeg = d_new_jpegimage();
                           jpeg->compress(jpeg,image);
                           
                           message = d_new_message();
                           message->put_jpeg_image(message,jpeg);
                       
                           if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                               message->free(message);
                           }
                           jpeg->free(jpeg);
                        }
                        image->free(image);
                        
                    }
                    
                    backup = action;
                    break;
            }
        }
        
    }
    rt_sem_v(&semBarriere);
    rt_printf("************* FIN CAMERA *************\n\n");
}

/*void watchdog (void *arg) {
    int status = STATUS_OK;
    
    rt_printf("twatchdog : Debut de l'éxecution de periodique à 1s\n");
    rt_task_set_periodic(NULL, TM_NOW, 1000000000);
    
    while(!echec) {
        rt_sem_p(&semWatchdog,TM_INFINITE);
        
        while (status == STATUS_OK) {
            rt_task_wait_period(NULL);
        
            rt_mutex_acquire(&mutexEtat, TM_INFINITE);
            status = etatCommRobot;
            rt_mutex_release(&mutexEtat);
            
            if (status == STATUS_OK) {
                rt_mutex_acquire(&mutexRobot,TM_INFINITE);
                status = robot->reload_wdt(robot);
                rt_mutex_release(&mutexRobot);
                
                rt_printf("twatchdog: status = %d",status);
                if (status == STATUS_ERR_CHECKSUM) {
                    status = STATUS_OK;
                }
                
                verifierNbConnexion(status);
            }
        }
    }
}*/
