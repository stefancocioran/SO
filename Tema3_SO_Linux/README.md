Nume: Cocioran Ștefan
Grupă: 331CA

# Tema 3 Loader de Executabile

Organizare
-

- Pentru implementarea temei am folosit scheletul dat, precum și rezolvarea <br />
exercițiilor din Laboratorul 6. 
- Scopul acesteia este implementarea sub forma unei biblioteci partajate/dinamice <br />
un loader de fișiere executabile în format ELF pentru Linux și PE pentru Windows. <br />
- Loader-ul va încărca fișierul executabil în memorie pagină cu pagină, folosind <br />
un mecanism de tipul demand paging.
- În fișierul ***utils.h*** se găsește macrou-ul DIE, folosit în sursele din <br />
laborator, și o structură de tipul **so_page** ce conține spațiul de adresă al <br />
unei pagini și un pointer către urmatoarea pagină mapata în memorie (fiind astfel <br />
folosită ca o listă).


Implementare
-

- Se parseaza fișierul binar iar în structura **so_exec** sunt introduse date <br />
despre segmentele executabilului și atributele lui. 
- Dupa apelarea funcției **so_start_exec** se vor executa page fault-uri pentru <br />
fiecare acces de pagină nouă/nemapată.
- Am implementat page fault handler-ul prin intermediul unei rutine pentru <br />
tratarea semnalului SIGSEGV sau a unui handler de excepție:
    - Se va afla adresa de memorie care a cauzat page fault-ul;
    - Se caută segmentul executabilului în care se află cuprinsă această adresă, <br />
    dacă nu este găsit înseamnă că se încearcă un acces invalid la memorie și <br />
    se rulează default handler-ul;
    - Se verifică dacă adresa se află într-o pagină deja mapată, daca este se <br />
    va rula handler-ul default, fiind un acces la memorie care nu este permis;<br />
    - Dacă adresa este într-o pagină ne-mapată, se copiaza din segmentul din <br />
    fișier datele;
    - Pentru a ști cantitatea de date care trebuie citită din fișier, se face <br />
    diferența între adresa de start a paginii următoare și file_size. Daca diferența este <br />
    pozitivă, înseamnă că se vor citi **page_size - diff** bytes (nu a depășit pagina <br />
    curentă), iar dacă este negativă înseamnă că file_size-ul depășește pagina <br />
    curentă și se vor citi maxim **page_size** bytes (întreaga dimensiune a paginii);
    - După copierea datelor, se zeroizează diferența între spațiul din memorie <br />
    și spațiul din fișier;
    - Se mapează pagina în memorie la adresa sa de început;
    - Se modifică permisiunile cu cele ale segmentului din care face parte.
    - Pagina este adăugată în lista din **loader**, este marcată ca fiind mapată în memorie. <br />
    Loader-ul este de fapt o structură de tip **so_page** ce reprezintă head-ul listei <br />
    de pagini mapate în memorie.


**Windows** : Pentru a-i da permisiuni folosind **VirtualProtect**, a fost nevoie <br />
să implementez o funcție auxiliară față de Linux ( ***get_perm*** ) pentru a trece <br />
flag-ul de protecție (permisiunile) al segmentului la valori specifice **win32** **[5]**.<br /> 
(ex: PERM_R -> PAGE_READONLY)


Bibliografie
-

- [1]. https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-04
- [2]. https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-06
- [3]. https://github.com/systems-cs-pub-ro/so/blob/master/labs/lab06/sol/lin/5-prot/prot.c
- [4]. https://github.com/systems-cs-pub-ro/so/blob/master/labs/lab06/sol/win/4-prot/libvm.c
- [5]. https://docs.microsoft.com/en-us/previous-versions/ms913495(v=msdn.10)
- [6]. https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc
