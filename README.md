# Laporan Resmi Soal 1 Shift Modul 3 - Sistem Operasi

Daftar Isi:
1. [Deskripsi Tugas](#1-deskripsi-tugas)
2. [Persyaratan Sistem](#2-persyaratan-sistem)
3. [Implementasi Kode](#3-implementasi-kode)
4. [Dokumentasi Output](#4-dokumentasi-output)

---

## 1. Deskripsi Tugas
Tugas ini mensimulasikan pembangunan infrastruktur digital bernama **"The Wired"**. Fokus utamanya adalah pembuatan **Server** yang stabil dan aplikasi **NAVI (Client)** yang mampu menangani fragmentasi identitas serta sinkronisasi tanpa batas di dalam jaringan.

## 2. Persyaratan Sistem
Berdasarkan soal yang diberikan, sistem harus memenuhi kriteria berikut:
*   **Koneksi:** Terdaftar melalui alamat dan port tertentu di `file protocol`.
*   **Asynchronous:** NAVI harus mampu mengirim dan menerima pesan secara asinkron tanpa menggunakan `fork`.
*   **Skalabilitas:** Server harus mendeteksi banyak klien, membedakan pesan masuk, dan menangani diskoneksi (`/exit` atau signal interrupt).
*   **Unique Identity:** Validasi nama agar tidak ada dua klien dengan nama yang sama.
*   **Broadcast:** Setiap pesan dari satu klien diteruskan ke seluruh klien lain yang aktif.
*   **Remote Procedure Call (The Knights):** Fitur admin khusus dengan autentikasi password untuk mengecek user aktif, uptime server, dan mematikan server.
*   **Logging:** Pencatatan otomatis ke `history.log` dengan format waktu `[YYYY-MM-DD HH:MM:SS]`.

## 3. Implementasi Kode

### A. Server (The Wired)
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include "protocol.h"

typedef struct {
    int fd;
    char name[50];
    int authenticated;
} Client;

Client clients[30];
time_t start_time;

void log_event(const char* tag, const char* content) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp == NULL) return;
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] [%s]\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec, tag, content);
    fclose(fp);
}

void broadcast(int sender_fd, char* msg, char* name) {
    char out_buffer[1100];
    sprintf(out_buffer, "[%s]: %s", name, msg);
    for (int i = 0; i < 30; i++) {
        if (clients[i].fd > 0 && clients[i].fd != sender_fd && clients[i].authenticated == 1) {
            send(clients[i].fd, out_buffer, strlen(out_buffer), 0);
        }
    }
}

int main() {
    int server_fd, new_socket, max_sd;
    struct sockaddr_in address;
    fd_set readfds;
    start_time = time(NULL);

    for (int i = 0; i < 30; i++) clients[i].fd = 0;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) exit(EXIT_FAILURE);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) exit(EXIT_FAILURE);
    if (listen(server_fd, 10) < 0) exit(EXIT_FAILURE);
    
    log_event("System", "SERVER ONLINE");
    printf("The Wired is running on port %d...\n", PORT);

    while(1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < 30; i++) {
            if (clients[i].fd > 0) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_sd) max_sd = clients[i].fd;
            }
        }

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, NULL, NULL);
            for (int i = 0; i < 30; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    clients[i].authenticated = 0;
                    char *ask = "Enter your name: ";
                    send(new_socket, ask, strlen(ask), 0);
                    break;
                }
            }
        }

        for (int i = 0; i < 30; i++) {
            int sd = clients[i].fd;
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                char buffer[1024] = {0};
                int valread = recv(sd, buffer, 1024, 0);
                
                if (valread <= 0) {
                    char logmsg[100];
                    sprintf(logmsg, "User '%s' disconnected", clients[i].name);
                    log_event("System", logmsg);
                    close(sd);
                    clients[i].fd = 0;
                    memset(clients[i].name, 0, 50);
                } else {
                    buffer[strcspn(buffer, "\n")] = 0;

                    if (clients[i].authenticated == 0) {
                        int duplicate = 0;
                        for (int j = 0; j < 30; j++) {
                            if (clients[j].fd > 0 && i != j && strcmp(clients[j].name, buffer) == 0) {
                                duplicate = 1;
                            }
                        }

                        if (duplicate) {
                            char err[100];
                            sprintf(err, "[System] The identity '%s' is already synchronized in The Wired.\nEnter your name: ", buffer);
                            send(sd, err, strlen(err), 0);
                        } else if (strcmp(buffer, "The Knights") == 0) {
                            strcpy(clients[i].name, buffer);
                            clients[i].authenticated = 2;
                            send(sd, "Enter Password: ", 16, 0);
                        } else {
                            strcpy(clients[i].name, buffer);
                            clients[i].authenticated = 1;
                            char logmsg[100], welcome[100];
                            sprintf(logmsg, "User '%s' connected", buffer);
                            sprintf(welcome, "--- Welcome to The Wired, %s ---", buffer);
                            log_event("System", logmsg);
                            send(sd, welcome, strlen(welcome), 0);
                        }
                    } 
                    else if (clients[i].authenticated == 2) {
                        if (strcmp(buffer, PASSWORD) == 0) {
                            clients[i].authenticated = 3;
                            char* menu = "\n[System] Authentication Successful. Granted Admin privileges.\n\n=== THE KNIGHTS CONSOLE ===\n1. Check Active Entities (Users)\n2. Check Server Uptime\n3. Execute Emergency Shutdown\n4. Disconnect\nCommand >> ";
                            send(sd, menu, strlen(menu), 0);
                            log_event("System", "User 'The Knights' connected");
                        } else {
                            send(sd, "[System] Wrong Password. Disconnecting...\n", 42, 0);
                            close(sd); clients[i].fd = 0;
                        }
                    }
                    else if (clients[i].authenticated == 3) {
                        if (strcmp(buffer, "1") == 0) {
                            char list[512] = "Active NAVIs: ";
                            for (int j = 0; j < 30; j++) {
                                if (clients[j].authenticated == 1) {
                                    strcat(list, clients[j].name); strcat(list, ", ");
                                }
                            }
                            strcat(list, "\nCommand >> ");
                            send(sd, list, strlen(list), 0);
                            log_event("Admin", "RPC_GET_USERS");
                        } else if (strcmp(buffer, "2") == 0) {
                            char upt[128];
                            sprintf(upt, "Server Uptime: %ld seconds\nCommand >> ", time(NULL) - start_time);
                            send(sd, upt, strlen(upt), 0);
                            log_event("Admin", "RPC_GET_UPTIME");
                        } else if (strcmp(buffer, "3") == 0) {
                            log_event("Admin", "RPC_SHUTDOWN");
                            log_event("System", "EMERGENCY SHUTDOWN INITIATED");
                            printf("Shutdown by Admin.\n");
                            exit(0);
                        } else if (strcmp(buffer, "4") == 0) {
                            send(sd, "[System] Disconnecting from The Wired...\n", 42, 0);
                            close(sd); clients[i].fd = 0;
                        } else {
                            send(sd, "Invalid Command.\nCommand >> ", 28, 0);
                        }
                    }
                    else {
                        if (strlen(buffer) > 0) {
                            log_event(clients[i].name, buffer);
                            broadcast(sd, buffer, clients[i].name);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
```
<p align="justify">
Secara garis besar, kode ini mengimplementasikan sebuah Multipurpose Chat Server menggunakan protokol TCP/IP. Berikut adalah poin-poin utama mekanismenya:

  1. **Pengelolaan Klien Tanpa Fork (select):**
Server menggunakan fungsi select() untuk menangani banyak koneksi klien secara bersamaan dalam satu process (asinkron). Ini memungkinkan server tetap responsif menunggu input dari klien lama sambil tetap bisa menerima koneksi klien baru tanpa perlu membuat proses baru (fork).

  2. **Manajemen Identitas (The Wired):**
Setiap klien yang terhubung disimpan dalam array of struct Client. Server mewajibkan klien memasukkan nama saat pertama kali terhubung. Terdapat pengecekan duplikasi nama untuk memastikan identitas setiap entitas di "The Wired" bersifat unik.

  3. **Mekanisme Komunikasi (Broadcast):**
Fungsi broadcast() bertugas meneruskan pesan yang diterima dari satu klien ke seluruh klien lain yang statusnya sudah terautentikasi (authenticated == 1), sehingga tercipta ruang obrolan kolektif.

  4. **Remote Procedure Call (The Knights):**
Server memiliki logika khusus untuk entitas admin bernama "The Knights". Jika klien masuk dengan nama tersebut, server akan meminta password. Setelah login berhasil, admin diberikan akses ke fungsi internal server seperti:

    1. Melihat daftar pengguna aktif.

    2. Mengecek uptime server (berapa lama server sudah berjalan).

    3. Melakukan Emergency Shutdown (mematikan server secara remote).

  5. **Logging Otomatis:**
Setiap kejadian penting (server online, user masuk/keluar, chat, dan perintah admin) dicatat secara otomatis ke dalam file history.log melalui fungsi log_event(). Format log mencakup stempel waktu (timestamp) sesuai dengan permintaan pada soal.
</p>

### B. Client (Navi)
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "protocol.h"

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    fd_set readfds;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed.\n");
        return -1;
    }

    while(1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);

        select(sock + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(sock, &readfds)) {
            memset(buffer, 0, 1024);
            int valread = read(sock, buffer, 1024);
            if (valread <= 0) break;
            
            printf("%s", buffer);
            fflush(stdout);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, 1024);
            fgets(buffer, 1024, stdin);
            buffer[strcspn(buffer, "\n")] = 0;
            send(sock, buffer, strlen(buffer), 0);
        }
    }
    close(sock);
    return 0;
}
```
<p align="justify">
Kode ini berfungsi sebagai NAVI, yaitu aplikasi klien yang menghubungkan pengguna ke jaringan "The Wired". Berikut adalah mekanisme utamanya:

  1. **Inisialisasi Koneksi:**
Client membuat sebuah socket TCP dan mencoba menghubungkan diri ke alamat IP dan Port yang telah ditentukan di dalam protocol.h. Jika koneksi berhasil, client langsung masuk ke siklus komunikasi.

  2. **Komunikasi Asinkron dengan select:**
Sesuai dengan syarat tugas (tanpa menggunakan fork), client menggunakan fungsi select() untuk memantau dua hal sekaligus secara bersamaan:

    1. Terminal/Keyboard (STDIN_FILENO): Menunggu input dari pengguna untuk dikirim ke server.

    2. Socket Jaringan (sock): Menunggu kiriman pesan atau data dari server.
Ini memungkinkan client menerima pesan dari server (seperti pesan broadcast dari user lain) secara real-time meskipun pengguna sedang tidak mengetik apa pun.

  3. **Pengiriman Data:**
Setiap kali pengguna mengetik pesan dan menekan Enter, input tersebut akan dibersihkan dari karakter newline (\n) dan dikirimkan ke server menggunakan fungsi send().

  4. **Penerimaan Data:**
Setiap data yang masuk dari server akan langsung dicetak ke terminal pengguna. Jika server memutus koneksi (misalnya saat admin melakukan shutdown), client akan mendeteksi nilai read yang kosong (valread <= 0), keluar dari perulangan, dan menutup aplikasi secara bersih.
</p>
