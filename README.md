# B.E-Temps-Reel

Projet réalisé à l'INSA en quadrinôme et encadré par Pierre Emmanuel Hladik.

Les bibliothèque destjil et xenomai nous étaient fournies. Nous avons du développer le fonctionnement d'un robot via un moniteur, 
c'est à dire pouvoir le filmer en temps réel afin de le piloter, calculer sa position dans une arène, modifier sa vitesse. Tout
ces facteurs étant soumis à des contraintes temporelles. On a donc créé différent threads lancés en même temps 
mais dont l'exécution était gérer par des sémaphores. l'emploi de variables globales nous a permis de manipuler aussi des mutexs.
