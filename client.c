#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>

#define IPADDRESS "127.0.0.1" // (Egen computer) IP-adresse til serveren
#define PORT 8080             // Port til serveren

typedef struct {
    char Date[20];                 // 0
    double ElectricalConductivity; // 1
    double pH;                     // 2
    double OxygenLevel;            // 3
    double Temperature;            // 4
    double WaterHeight;            // 5
} Data;

// Prototyper
void sendData(char *);
void generateRandomData(Data *);
void generateManualData(Data *);

int main(void) {
    // Generer et tilfældigt seed ud fra tiden
    srand(time(NULL));
    Data newData;

    int choice;
    printf("1. Autogenerer data\n");
    printf("2. Manuel indtastning af data\n");
    printf("Indtast dit valg (1 eller 2): ");
    scanf("%d", &choice);

    if (choice == 1) {
        generateRandomData(&newData);
    } else if (choice == 2) {
        generateManualData(&newData);
    } else {
        printf("Ugyldigt valg\n");
        exit(EXIT_FAILURE);
    }

    char dataString[256];
    // Anvender sprintf (string print formatted) til at formatere data til en string
    sprintf(dataString, "%s,%.1lf,%.1lf,%.1lf,%.1lf,%.1lf",
            newData.Date, newData.ElectricalConductivity, newData.pH,
            newData.OxygenLevel, newData.Temperature, newData.WaterHeight);

    printf("\n\n%s\n\n", dataString);
    // Send det formaterede data til serveren
    sendData(dataString);

    return 0;
}

// Funktion til at sende data til serveren
void sendData(char *data) {
    int socketConnection;

    // Opret socket
    socketConnection = socket(AF_INET, SOCK_STREAM, 0); // AF_INET = IPv4 og SOCK_STREAM = TCP
    // socketConnection er under 0 hvis der opstår en fejl
    if (socketConnection < 0) {
        printf("Fejl ved oprettelse af socket\n");
        exit(EXIT_FAILURE);
    }

    // Opret serverInfo struct med information om serveren
    struct sockaddr_in serverInfo;
    serverInfo.sin_family = AF_INET;
    // Konverterer port og ip-adresse til det korrekte format
    serverInfo.sin_port = htons(PORT);
    serverInfo.sin_addr.s_addr = inet_addr(IPADDRESS);

    // Opret en forbindelse til serveren
    // connect returnerer under 0 hvis der opstår en fejl
    if (connect(socketConnection, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) < 0) {
        printf("Kunne ikke forbinde til serveren, tjek IP-adresse og PORT nummer er sat \n");
        exit(EXIT_FAILURE);
    }

    // Send data til serveren
    send(socketConnection, data, strlen(data), 0);

    // Luk socket forbindelsen til serveren
    close(socketConnection);
}

// Funktion til at generere tilfældige data
void generateRandomData(Data *newData) {
    strcpy(newData->Date, "2023/11/30 09.00");
    newData->ElectricalConductivity = (double)rand() / RAND_MAX * 10.0;
    newData->pH = (double)rand() / RAND_MAX * 14.0;
    newData->OxygenLevel = (double)rand() / RAND_MAX * 10.0;
    newData->Temperature = (double)rand() / RAND_MAX * 30.0;
    newData->WaterHeight = (double)rand() / RAND_MAX * 2.0;
}

// Funktion til manuel indtastning fra brugeren
void generateManualData(Data *newData) {
    printf("Indtast dato (YYYY/MM/DD HH.MM): ");
    scanf(" %[^\n]", newData->Date); // %[^\n] = læs indtil der et linjeskift

    printf("Indtast elektrisk resistivitet: ");
    scanf("%lf", &newData->ElectricalConductivity);

    printf("Indtast pH: ");
    scanf("%lf", &newData->pH);

    printf("Indtast Oxygen niveau: ");
    scanf("%lf", &newData->OxygenLevel);

    printf("Indtast temperatur: ");
    scanf("%lf", &newData->Temperature);

    printf("Indtast vandstand: ");
    scanf("%lf", &newData->WaterHeight);
}