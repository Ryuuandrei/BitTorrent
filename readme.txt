Rusei Andrei 334CD

Nu stiu unde e limita de caractere pentru o linie in readme pentru ca scriu in notepad. Aveti monitoare
mari nu va mai plangeti, limita de caractere pentru o linie a fost facuta cand monitoarele nu aveau
rezolutii la fel de mari ca acum. E cringe. Multumesc! O sa incerc sa scriu liniile la fel de lungi, dar
nu stiu cate caractere are o linie. Scadeti-mi punctaj ca e linia prea lunga ICBA!

Peer:
	Structuri folosite:
	peer_data este o structura ce contine datele folosite de un peer. Aceste date sunt:
	- owned_files: mapa file_name -> vector de hash-uri detinute de un client.
	- wanted_files: vector cu numele fisierelor pe care clientul doreste sa e descarce.
	
	Metode speficice acestei structuri:
	- send_tracker_owned_files(): trimite catre tracker datele din structura owned_files, facand 
	urmatorii pasi: trimite numarul de elemente din structura(este egal cu numarul de fisiere detinute).
	Pentru fiecare fisier detinut, ii trimite numele fisierul, numarul de hash-uri pe care le are fisierul,
	apoi trimite toate hash-urile si asteapta un ACK de la tracker.

	- recv_peers(peers): peers este o mapa rank_client -> hash-uri detinute pentru un fisier. Functia este folosita
	pentru a afla raspunsul atunci cand clientul ii cere server-ul ce peers are un fisier si segmentele detinute de
	fiecare. Functia primeste de la tracker numarul de peers. Apoi primeste peers si pentru fiecare peer primeste
	numarul de segmente detinut de fiecare si toate segmentele.
	
	Folosire:
	Peer-ul citeste datele din fisierul de intrare(am facut acest pas in constructorul structurii de peer_data).
	Apoi trimite datele sale catre tracker folosind metoda send_tracker_owned_files, astepta ACK de la tracker
	si porneste thread-urile de download si upload.
	
	Download:
	Tracker-ul trimite request catre tracker ca doreste un fisier, dupa care trimite numele fisierul. Serverul ii
	raspunde cu numarul de hash-uri pentru fisierul respectiv, apoi ii trimite toate hash-urile fisierului. Dupa
	primeste de la tracker numarul de peers, toti peers pentru fisierul respectiv si segmentele detinute de fiecare.
	Facand acest lucru stiu de la tracker care sunt hash-urile segmentelor fisierului. Puteam si sa trimit doar swarm-ul
	fisierului insa trebuia sa vad cine e seed(cine are cele mai multe segmente) pentru a stii care sunt toate hash-urile
	si care e ordinea lor. Am ales sa trimita tracker-ul care sunt hash-urile unui fisier pentru a fii mai convenabil 
	pentru mine. Dupa ce stiu ce hash-uri are fiecare segment si ce hash-uri are fiecare client am cautat intai in lista
	trimisa de tracker daca clientul are hash-ul cautat. Daca da il adaug intr-o lista de peers buni, apoi aleg random un
	peer din acea lista. Ii trimit un mesaj cu hash-ul cautat si acesta imi intoarce un mesaj de ok daca il detine(clar il
	detine, dar am facut-o ca sa si arat ca il detine). La fiecare 10 segmente descarcate i-am trimis tracker-ului un mesaj
	de tip update. Aici a trebuit doar sa apelez functiile send_tracker_owned_data() si recv_peers(), datorita implementarii
	pe care am folosit-o si al proprietatilor unordered_map din cpp. Daca am terminat de trimis un fisier ii trimit
	tracker-ului un mesaj ca am terminat si scriu in fisierului potrivit hash-urile fisierului. La terminarea tuturor 
	descarcarilor ii trimit tracker-ului alt mesaj.

	Upload:
	Primesc un mesaj. Daca acest mesaj este "SHUTDOWN", trimis de la tracker inchid acest thread, altfel este un hash primit
	de la un alt client. Caut hash-ul respectiv in lista de fisiere(l-am cautat degeaba ca oricum stiu ca il are, ca altfel nu
	i-as fii trimis hash-ul) si am trimis un ok clientului.

Tracker:
	Structuri folosite:	
	tracker_data este o structura ce contine datele folosite de tracker. Aceste date sunt:
	- files: mapa file_name -> vector cu hash-uri ce contine toate fisierele cu hash-urile acestora.
	Aceasta structura este mai mult pentru convenienta.
	- swarm: mapa file_name -> (mapa rank_client -> hash-uri detinute de client pentru file_name). Este folosita
	pentru  tine minte pentru fiecare fisier cine il are si ce hash-uri are.

	Metode specifice acestei structuri:
	- recv_files_from_client(): este inversul send_tracker_owned_files(). Unde trimite un client date, tracker-ul
	primeste si unde trimite tracker-ul dae checker-ul primeste. Datele primite le foloseste pentru a updata swarm-ul
	fisierului. La final trimite un ack clientului.

	- send_peers(file_name, dest): inversul recv_peers. Trimite catre clientul cu rank dest toti peers si segementele
	detinute de fiecare pentru fisierul file_name.

	Folosire:
	Tracker-ul primeste date de la toti clienti legat de ce fisiere au. Acesta etapa este foarte asemanatoare cu
	recv_files_from_client(), atat ca pun date in swarm, dar si in files unde tin minte toate fisierle si ce hash-uri au
	fiecare. Dupa primire trimit un ACK clientilor pentru a semnala faptul ca au verde sa inceapa descarcarea.

	Mesajele primite de tracker au urmatoarele tipuri:
	0 - cerere de peers pentru un fisier
		Primeste de la un client un fisier si ii raspunde cu hash-urile fisierului si cu swarmul fisierului.

	1 - update de la un client
		Primeste fisierele detinute de fisier si hash-urile pentru fiecare si ii trimite swarmul pentru fisierul pe
		care il descarca.

	2 - fisier descarcat
		Marcheaza clientul ca avand toate hash-urile pentru un fisier.

	3 - toate fisierele descarcate
		Decrementeaza numarul de clieti care inca descarca.
	
	Daca au terminat toti clientii de descarcat tracker-ul trimite "SHUTDOWN" catre threadul de upload, facandu-l pe acesta
	sa se inchida.
		

