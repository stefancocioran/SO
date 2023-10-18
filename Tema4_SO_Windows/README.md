Nume: Cocioran Ștefan
Grupă: 331CA

# Tema 4 Planificator de threaduri

Organizare
-

- Scopul temei este implementarea sub forma unei biblioteci partajate/dinamice <br />
un planificator de threaduri care va controla execuția acestora în user-space. <br />
Acesta va simula un **scheduler** de procese preemptiv, într-un sistem uniprocesor, <br />
care utilizează un algoritm de planificare Round Robin cu priorități.
- În fișierul ***utils.h*** se găsește macrou-ul DIE, folosit în sursele din <br />
laborator, și structuri de date de tipul **so_thread** și **so_scheduler** ce conțin:
    - **so_thread** 
        - tid = id-ul thread-ului
        - state = starea în care se află (NEW / READY / RUNNING / TERMINATED)
        - priority = prioritatea cu care rulează
        - time = timpul pe care îl are la dispoziție pentru e executa instrucțiuni
        - io_wait = evenimentul/operația I/O pe care o așteaptă thread-ul
        - handler = handler-ul executat de thread
        - semaphore = mecanism de sincronizare pentru a permite execuția unui thread
        - ***Windows*** - hThread = handler-ul returnat de apelul funcției **CreateThread** 
    - **so_scheduler**
        - time_quantum = cuanta de timp alocată fiecărui thread pentru rulare
        - io_max = numărul maxim de evenimente/io suportate
        - list_size = dimensiunea array-ului în care sunt stocate toate thread-urile
        - queue_size = dimensiunea priority queue-lui
        - running_thread = pointer către thread-ul care rulează în momentul respectiv
        - ready_queue = priority queue pentru a planifica thread-urile corespunzător
        - threads = un array de structuri **so_thread** pentru a ține evidența tuturor thread-urilor create <br />


Implementare
-

### **- so_init -**
- Funcția verifică corectitudinea parametrilor primiți (cuanta de timp și numărul de <br />
evenimente suportate) și alocă memorie pentru o structură de tipul **so_scheduler** <br />
și îi sunt inițializate toate câmpurile. 
- Array-ul în care sunt ținute toate thread-urile create și priority queue-ul au <br />
fost alocate cu o dimensiune de fixă de 1024 bytes, iar în cazul în care nu există <br />
loc pentru inserarea unei noi valori, dimensiunea acestora va fi dublată.

### **- so_fork -**
- Alocă memorie și inițializează o structură de tipul **so_thread** cu parametrii primiți.
- Se crează un thread nou folosind funcția **pthread_create**/**CreateThread**, <br />
începe execuția rutinei dată acestuia și este adăugat în **threads** ce va conține <br />
toate thread-urile create.
- Thread-ul nou creat este adăugat în priority queue, este pregătit pentru a fi<br />
planificat (trece în starea READY).
- Dacă nu există deja un thread care rulează, va rula cel nou creat și este trecut <br />
în starea RUNNING. În caz contrar, se scade o unitate de timp din cuanta thread-ului <br />
care ruleaza și se verifică daca în sistem a intrat un thread cu o prioritate mai <br />
mare și dacă thread-ul care rulează la momentul respectiv mai are timp (cazuri în <br />
care va fi actualizat, se apelează funcția **update_scheduler**).

### **- so_exec -**
- Se scade o unitate din cuanta de timp a thread-ului care rulează în momentul respectiv.
- Se verifică dacă timpul alocat rulării acestuia s-a terminat, caz în care se <br /> 
alege un nou thread care va fi rulat (se apelează funcția **update_scheduler**).

### **- so_wait -**
- Thread-ul care ruleaza va fi pus în starea de WAITING și field-ul **io_wait** <br />
al acestuia ii va fi asociat evenimentul primit ca parametru. 
- Se rulează următorul thread din coadă (**set_next_running_thread**) și thread-ul <br />
care a rulat anterior va fi întrerupt, așteaptă un semnal de deblocare.

### **- so_signal -**
- Iterează prin array-ul de thread-uri, iar toate care se află în starea WAITING, <br />
pentru evenimentul primit ca parametru, vor fi introduse în coadă.
- Întrucât au fost semnalate mai multe thread-uri, este posibil ca unul dintre <br />
acestea să aiba o prioritate mai mare decât cel care rulează. De aceea, se <br />
actualizează planificatorul și este ales thread-ul cu cea mai mare prioritate <br />
dintre toate, dacă este cazul (se apelează funcția **update_scheduler**).

### **- so_end -**
Așteaptă terminarea tuturor threadurilor înainte de părăsirea sistemului și <br />
sunt eliberate toate resursele planificatorului. 

### **- init_thread -** 
Inițializează câmpurile structurii **so_thread**.

### **- start_thread -**
- Functia este executată la crearea unui thread nou, așteaptă ca acesta să fie planificat.
- Apelează handler(prio), intră în starea TERMINATED și este rulat următorul thread din coadă (**set_next_running_thread**).

### **- update_scheduler -**
- Verifică daca cuanta de timp alocată thread-ului care rulează s-a terminat și <br />
dacă în sistem a fost introdus un thread cu o prioritate mai mare. 
- În ambele cazuri, este actualizat planificatorul și este setat sa ruleze un alt thread, <br />
dacă este cazul, iar cel care rula anterior așteaptă sa își reînceapă execuția și <br />
este adăugat în coadă.

### **- halt_running_thread -**
Thread-ul primit ca parametru este pus în așteptare, semaforul său este blocat <br />
folosind **sem_wait**/**WaitForSingleObject**.

### **- set_next_running_thread -**
- Este scos din coadă thread-ul cu prioritatea cea mai mare și starea sa devine RUNNING.
- Semaforul său este eliberat, thread-ul este deblocat pentru a putea executa instrucțiuni. 

### **- push_queue -**
- Thread-urile adăugate în coada de priorități vor avea starea setată ca READY.
- Acestea sunt inserate conform algoritmului de planificare Round Robin cu priorități.

### **- pop_queue -**
Scoate primul thread din coadă.

### **- add_thread_list -**
Thread-ul primit ca parametru este adăugat în **threads** pentru a-i putea ține evidența.

**Observații:**<br />
Operațiile de introducere/scoatere a elementelor din lista sau coada scheduler-ului <br />
(sau orice alta modificare adusă structurii **scheduler**, care este partajată între threaduri) <br />
ar trebui, de regulă, sa fie realizate folosind un mutex întrucât ar putea apărea <br />
situații de race condition și sa fie suprascrise/pierdute valori. Dar în scenariul dat, <br />
nu va apărea niciodată o astfel de problemă deoarece rulează cel mult un thread <br />
la un anumit moment de timp.

**Windows** : Singura diferență dintre cele două implementări este faptul că pentru <br />
Windows a trebuit să am un field extra în structura **so_thread** ca să rețin handler-ul <br />
returnat de funcția **CreateThread**, necesar pentru a da join thread-urilor la final <br />
folosind **WaitForSingleObject**. În rest, implementările sunt aproximativ identice, <br />
doar au fost schimbate apelurile de funcții POSIX <-> WINDOWS.

Bibliografie
-

- [1]. https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-08
- [2]. https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-09
- [3]. https://docs.microsoft.com/en-us/windows/win32/api/synchapi/