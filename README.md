## Serwer DHCP
Serwer usługi DHCP, przydziela adresy IP z puli. Wysyła informację o bramie domyślnej i serwerze DNS. Rozumie polecenia DISCOVER, RELEASE i REQUEST i odpowiednio reaguje. Testowane w sieci lokalnej z telefonem z Androidem, telefon uzyskał dostęp do Internetu.

## Struktura
### Skrypty
* build.sh - skrypt do kompilacji
* config.sh - zmienne konfiguracyjne

### Źródła
* addrpool.hpp - pula adresów IP
* buffer.hpp - bufor pamięci (podobny do std::span z c++20)
* dhcpmsg.hpp - struktura wiadomości DHCP
* dhcpopts.hpp - struktura opcji DHCP
* dhcpsrvc.hpp - usługa serwera DHCP (główna logika)
* socket.hpp - prosty serwer UDP
* misc.hpp - pomocnicze makra
* pcapwrapper.hpp - wrapper nad libpcap, do debugowania
* main.cpp - punkt wejścia

## Sposób kompilacji i uruchomienia
    ./build.sh
    source config.sh
    ./app.out nazwa_interfejsu_sieciowego

`build.sh` poprosi o sudo, żeby nadać capabilities dla apki. Można też całą apkę uruchomić jako `root`.