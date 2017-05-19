=====================
Virtual Machine Setup
=====================


Exercises are designed to run on a qemu based virtual machine. There
are two types of images supported by the virtual machine setup:

* minimal, based on busybox and dropbear
* prebuilt yocto image

There are also two types of architectures supported:

* x86
* ARM

In order to runt he virtual machine you will need a Linux host and the following packages:

* build-essential
* qemu-system-x86
* qemu-syste-arm
* kvm

The virtual machine scripts are available in tools/labs/ and it contains 

* a Makefile ; rulați make pentru a construi imaginea mașinii virtuale;
  busybox, pentru utilitare de bază în mașina virtuală
  
server ssh (dropbear) compilat static
alte fișiere necesare rulării mașinii virtuale vor fi create la prima rulare a make; nu este nevoie să le modificați.
Înainte de a folosi mașina virtuală este necesar sa descărcați, compilați și creați link-uri simbolice către imaginea de kernel ce va fi folosită de către mașina virtuală:

$ make setup-bzImage

~/src/qemu-so2$ make setup-bzImage
./setup-bzImage.sh
--2017-02-19 14:36:40--  https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.9.11.tar.xz
Resolving cdn.kernel.org (cdn.kernel.org)... 151.101.64.69, 151.101.128.69, 151.101.192.69, ...
Connecting to cdn.kernel.org (cdn.kernel.org)|151.101.64.69|:443... connected.
HTTP request sent, awaiting response... 200 OK
Length: 93211324 (89M) [application/x-xz]
Saving to: ‘linux-4.9.11.tar.xz’

linux-4.9.11.tar.xz                         100%[========================================================================================>]  88,89M  10,9MB/s    in 8,3s

2017-02-19 14:36:48 (10,7 MB/s) - ‘linux-4.9.11.tar.xz’ saved [93211324/93211324]

make[1]: Entering directory '/home/tavi/src/linux-4.9.11'
HOSTCC  scripts/basic/fixdep
HOSTCC  scripts/kconfig/conf.o
SHIPPED scripts/kconfig/zconf.tab.c
SHIPPED scripts/kconfig/zconf.lex.c
SHIPPED scripts/kconfig/zconf.hash.c
HOSTCC  scripts/kconfig/zconf.tab.o
HOSTLD  scripts/kconfig/conf
scripts/kconfig/conf  --olddefconfig Kconfig
#
# configuration written to .config
#
make[1]: Leaving directory '/home/tavi/src/linux-4.9.11'
make[1]: Entering directory '/home/tavi/src/linux-4.9.11'
scripts/kconfig/conf  --silentoldconfig Kconfig
SYSTBL  arch/x86/entry/syscalls/../../include/generated/asm/syscalls_32.h
...
Pentru rularea mașinii virtuale folosiți:

make
Pentru rularea mașinii virtuale fără mode “grafic” folosiți:

QEMU_DISPLAY=none make
Fișierele pe care vreți să le puneți în mașina virtuală vor trebui puse în directorul fsimg/root. Ele vor fi accesibile în mașina virtuală din directorul /root. Pentru a automatiza copierea în fsimg/root puteți adăuga un target în makefile-ul modulului, sau puteți crea symlink-uri.

GNU Make nu se pricepe prea bine la dependency tracking pe symlink-uri. Dacă alegeți să faceți symlink-uri în fsimg/root către fișierele voastre, asigurați-vă că imaginea este reconstruită de fiecare dată. Cel mai simplu este să faceți target-ul initrd.cpio phony. Generarea imaginii durează doar câteva secunde.
Veți compila modulele de kernel și programele de test pe mașina fizică. Dacă aveți un sistem pe 64 de biți, folosiți flag-ul -m32 pentru gcc, pentru a genera binare pe 32 de biți. De asemenea, folosiți opțiunea -static pentru programele de test.

Edit
Inspectare informații din mașina virtuală

Dacă aveți multe mesaje afișate în mașina virtuală puteți folosi combinațiile de taste Shift+PgUp, respectiv Shift+PgDn pentru a parcurge output-ul (scrolling) din consolă.

În afară de consola din modul grafic, setup-ul mașinii virtuale oferă o consolă adițională prin intermediul unui “pty - pseudoterminal interface”. Atunci când mașina virtuală este pornită căutați un mesaj de genul:

char device redirected to /dev/pts/20 (label virtiocon0)
Vă puteți conecta la această consolă folosind minicom sau screen:

$ minicom -D /dev/pts/20
Edit
Probleme frecvente

Q: Cum închid aplicația minicom?
A: Folosind combinația de taste Ctrl+a și apoi, separat, x. Apoi vă apare promptul de închidere a aplicației minicom.
Q: De ce apare urmatorul mesaj cand ma conectez prin ssh la masina virtuala?
ssh root@172.20.0.2
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@    WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!     @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
IT IS POSSIBLE THAT SOMEONE IS DOING SOMETHING NASTY!
Someone could be eavesdropping on you right now (man-in-the-middle attack)!
It is also possible that a host key has just been changed.
The fingerprint for the ECDSA key sent by the remote host is
SHA256:DijYADI/vj9Q5FAZPaPxqxh7BjpwQECvhuQGBcW433w.
Please contact your system administrator.
Add correct host key in /home/tavi/.ssh/known_hosts to get rid of this message.
Offending ECDSA key in /home/tavi/.ssh/known_hosts:50
remove with:
ssh-keygen -f "/home/tavi/.ssh/known_hosts" -R 172.20.0.2
ECDSA host key for 172.20.0.2 has changed and you have requested strict checking.
Host key verification failed.
A: Datorită faptului ca nu se folosește storage persistent, cheia mașinii se regenerează la fiecare boot. Clientul de ssh salveaza cheia când se conectează prima dată, iar la următorul reboot, cheia se va schimba și va cauza clientul de ssh să genereze eroarea de mai sus. Folosiți următoarea comandă pentru a evita această problemă:
ssh root@172.20.0.2 -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no


