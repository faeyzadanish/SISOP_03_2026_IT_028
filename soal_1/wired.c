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
    int authenticated; // 0: baru konek, 1: user aktif, 2: nunggu pass admin, 3: admin aktif
} Client;

Client clients[30];
time_t start_time;

// Fungsi Logging sesuai Screenshot (217).png
void log_event(const char* tag, const char* content) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp == NULL) return;
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    // Format: [YYYY-MM-DD HH:MM:SS] [Tag] [Content]
    fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] [%s]\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec, tag, content);
    fclose(fp);
}

// Fungsi Broadcast sesuai Screenshot (215).png
void broadcast(int sender_fd, char* msg, char* name) {
    char out_buffer[1100];
    sprintf(out_buffer, "[%s]: %s", name, msg);
    for (int i = 0; i < 30; i++) {
        // Kirim ke semua user aktif kecuali pengirim dan admin yang sedang di konsol
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

    // Inisialisasi daftar klien
    for (int i = 0; i < 30; i++) clients[i].fd = 0;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) exit(EXIT_FAILURE);

    // Agar port bisa langsung dipakai ulang (Reuse Address)
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind & Listen
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

        // 1. Handling Koneksi Baru
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, NULL, NULL);
            for (int i = 0; i < 30; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    clients[i].authenticated = 0;
                    // Kirim perintah masukkan nama (Penting!)
                    char *ask = "Enter your name: ";
                    send(new_socket, ask, strlen(ask), 0);
                    break;
                }
            }
        }

        // 2. Handling Pesan Masuk / Disconnect
        for (int i = 0; i < 30; i++) {
            int sd = clients[i].fd;
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                char buffer[1024] = {0};
                int valread = recv(sd, buffer, 1024, 0);
                
                if (valread <= 0) {
                    // Client Disconnected (Poin 3)
                    char logmsg[100];
                    sprintf(logmsg, "User '%s' disconnected", clients[i].name);
                    log_event("System", logmsg);
                    close(sd);
                    clients[i].fd = 0;
                    memset(clients[i].name, 0, 50);
                } else {
                    buffer[strcspn(buffer, "\n")] = 0; // Hapus newline

                    // FASE LOGIN (IDENTITAS UNIK) - Screenshot (214).png
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
                            clients[i].authenticated = 2; // Menunggu password
                            send(sd, "Enter Password: ", 16, 0);
                        } else {
                            strcpy(clients[i].name, buffer);
                            clients[i].authenticated = 1; // User Aktif
                            char logmsg[100], welcome[100];
                            sprintf(logmsg, "User '%s' connected", buffer);
                            sprintf(welcome, "--- Welcome to The Wired, %s ---", buffer);
                            log_event("System", logmsg);
                            send(sd, welcome, strlen(welcome), 0);
                        }
                    } 
                    // FASE PASSWORD ADMIN - Screenshot (216).png
                    else if (clients[i].authenticated == 2) {
                        if (strcmp(buffer, PASSWORD) == 0) {
                            clients[i].authenticated = 3; // Admin Aktif
                            char* menu = "\n[System] Authentication Successful. Granted Admin privileges.\n\n=== THE KNIGHTS CONSOLE ===\n1. Check Active Entities (Users)\n2. Check Server Uptime\n3. Execute Emergency Shutdown\n4. Disconnect\nCommand >> ";
                            send(sd, menu, strlen(menu), 0);
                            log_event("System", "User 'The Knights' connected");
                        } else {
                            send(sd, "[System] Wrong Password. Disconnecting...\n", 42, 0);
                            close(sd); clients[i].fd = 0;
                        }
                    }
                    // FASE KONSOL ADMIN (RPC) - Screenshot (216).png
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
                    // FASE CHAT BIASA (BROADCAST) - Screenshot (215).png
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
