#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Farvekoder hentet fra https://gist.github.com/RabaDabaDoba/145049536f815903c79944599c6f952a
#define ENABLE_COLORS // Kommenter denne linje ud for at deaktivere farver i terminalen

#ifdef ENABLE_COLORS
#define BLK "\e[0;30m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define BLU "\e[0;34m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"
#define reset "\e[0m"
#else
#define BLK ""
#define RED ""
#define GRN ""
#define YEL ""
#define BLU ""
#define MAG ""
#define CYN ""
#define WHT ""
#define reset ""
#endif

#define PORT 8080 // Port til serveren

// Filnavn til CSV-filen
#define FILENAME "Data_T01.csv"

// Grænseværdier for advarsler symbolske konstanter
#define electricalConductivityThreshold 2.0
#define pHThreshold 6.0
#define oxygenLevelThreshold 5.2
#define temperatureThreshold 21.0
#define waterHeightThreshold 1.0

typedef struct {
    char Date[20];                 // 0
    double ElectricalConductivity; // 1
    double pH;                     // 2
    double OxygenLevel;            // 3
    double Temperature;            // 4
    double WaterHeight;            // 5
} Data;

// Prototyper
int read_file(const char *, Data[], int);
void add(Data *, int, double, char *);
void alert(Data *, int);
void appendToFile(const char *, const char *);
void receiveData(Data[], int);
void plotGraph(Data[], int, char *);

int main(void) {
    // Array af structs med plads til 500 elementer (antager der er 500 data i csv-filen)
    Data data[500];

    int num_lines = read_file(FILENAME, data, 500);

    // Itererer igennem alle data i filen og printer dem
    for (int i = 0; i < num_lines; i++) {
        printf("Date: %s, Elektrisk resistivitet: %.1lf, pH: %.1lf, Oxygen niveau: %.1lf, Temperatur: %.1lf, Vandstand: %.1lf\n",
               data[i].Date, data[i].ElectricalConductivity, data[i].pH, data[i].OxygenLevel, data[i].Temperature, data[i].WaterHeight);
    }

    alert(data, num_lines);

    // Plot grafer
    plotGraph(data, num_lines, "pH");
    plotGraph(data, num_lines, "Temperature");
    plotGraph(data, num_lines, "Electrical Conductivity");
    plotGraph(data, num_lines, "Oxygen Level");
    plotGraph(data, num_lines, "Water Height");

    // Modtag data fra klienten
    receiveData(data, 500);
    

    return 0;
}

int read_file(const char *filename, Data data[], int max_lines) {
    // Indlæser filen i "read-only" mode (r)
    FILE *fp = fopen(filename, "r");

    if (fp == NULL) {
        printf("Kan ikke åbne filen");
        return 0;
    }

    // Læs headerlinjen (men ignorér den)
    char header[256];
    if (fgets(header, sizeof(header), fp) == NULL) {
        printf("Kunne ikke læse headeren");
        fclose(fp);
        return 0;
    }

    // Læs data linje for linje
    int linjeNummer = 0;
    while (fscanf(fp, " %19[^,],%lf,%lf,%lf,%lf,%lf", data[linjeNummer].Date, &data[linjeNummer].ElectricalConductivity, &data[linjeNummer].pH, &data[linjeNummer].OxygenLevel, &data[linjeNummer].Temperature, &data[linjeNummer].WaterHeight) == 6) {
        linjeNummer++;
        // Lave advarsel hvis der mangler data på linjen?

        if (linjeNummer >= max_lines) {
            printf("Maksimalt antal data nået (%d)\n", max_lines);
            break;
        }
    }

    fclose(fp);
    return linjeNummer;
}

void add(Data *data, int index, double valueToAdd, char *type) {
    printf(GRN "Tilføjer %.2lf enheder den %s til Tank 01\n" reset, valueToAdd, data->Date);
}

void alert(Data *data, int num_lines) {
    for (int i = 0; i < num_lines; i++) {
        if (data[i].pH < pHThreshold) {
            printf(RED "Advarsel: pH-værdien den Date %s er under grænseværdien!\n" reset, data[i].Date);
            add(data, i, pHThreshold - data[i].pH, "pH");
        }
        if (data[i].ElectricalConductivity < electricalConductivityThreshold) {
            printf(RED "Advarsel: Electrical Conductivity-værdien den %s er under grænseværdien!\n" reset, data[i].Date);
            add(data, i, electricalConductivityThreshold - data[i].ElectricalConductivity, "Electrical Conductivity");
        }
        if (data[i].OxygenLevel < oxygenLevelThreshold) {
            printf(RED "Advarsel: Oxygen niveauet den %s er under grænseværdien!\n" reset, data[i].Date);
            add(data, i, oxygenLevelThreshold - data[i].OxygenLevel, "Oxygen Level");
        }
        if (data[i].Temperature < temperatureThreshold) {
            printf(RED "Advarsel: Temperaturen den %s er under grænseværdien!\n" reset, data[i].Date);
            add(data, i, temperatureThreshold - data[i].Temperature, "temperatur");
        }
        if (data[i].WaterHeight < waterHeightThreshold) {
            printf(RED "Advarsel: Vandstanden den %s er under grænseværdien!\n" reset, data[i].Date);
            add(data, i, waterHeightThreshold - data[i].WaterHeight, "Water Height");
        }
    }
}

// Funktion til at append/tilføje data til CSV-filen
void appendToFile(const char *filename, const char *data) {
    FILE *fp = fopen(filename, "a");
    if (fp == NULL) {
        printf("Fejl ved åbning af filen i append mode.\n");
        exit(EXIT_FAILURE);
    }
    // Append/tilføj data til filen
    fprintf(fp, "%s\n", data);
    fclose(fp);
}

// Funktion til at håndtere socket-kommunikation for at modtage data fra klienten
void receiveData(Data data[], int max_lines) {
    int socketConnectionBind, socketConnectionRecv;
    struct sockaddr_in clientAddr;
    socklen_t addr_size = sizeof(clientAddr);

    // Opret socket
    socketConnectionBind = socket(AF_INET, SOCK_STREAM, 0); // AF_INET = IPv4 og SOCK_STREAM = TCP
    // socketConnectionBind er under 0 hvis der opstår en fejl
    if (socketConnectionBind < 0) {
        printf("Fejl ved oprettelse af socket\n");
        exit(EXIT_FAILURE);
    }

    // Opret serverInfo struct med information om serveren
    struct sockaddr_in serverInfo;
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    // Konverterer port til det korrekte format
    serverInfo.sin_port = htons(PORT);

    // Bind/åben for porten på serveren
    if (bind(socketConnectionBind, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) < 0) {
        printf("Fejl ved binding af socket\n");
        exit(EXIT_FAILURE);
    }

    // Lyt efter indkommende forbindelser
    // Tillader op til 10 forbindelser på samme tid
    if (listen(socketConnectionBind, 10) == 0) {
        printf("Lytter på port %d...\n", PORT);
    } else {
        printf("Fejl ved lytning, tjek om porten allerede er i brug\n");
        exit(EXIT_FAILURE);
    }

    // Modtag indkommende forbindelse
    if ((socketConnectionRecv = accept(socketConnectionBind, (struct sockaddr *)&clientAddr, &addr_size)) < 0) {
        printf("Fejl ved modtagelse af forbindelsen\n");
        exit(EXIT_FAILURE);
    }

    // Opret buffer til at indeholde det modtagne data
    char buffer[256];

    // Modtag data fra klienten
    recv(socketConnectionRecv, buffer, sizeof(buffer), 0);
    printf("Modtaget data fra klienten: %s\n", buffer);

    // Tilføj modtaget data til CSV-filen
    appendToFile(FILENAME, buffer);

    // Luk recieve socket og bind socket forbindelsen
    close(socketConnectionRecv);
    close(socketConnectionBind);
}

void plotGraph(Data data[], int num_lines, char *plotType) {

    // plotType = 0: pH, 1: Electrical Conductivity, 2: Oxygen Level, 3: Temperature, 4: Water Height


    if (strcmp(plotType, "pH") == 0) {
        // fopen (file open)
        FILE *tempFile = fopen("graph-data.txt", "w");
        // Skriv data til den graph-data.txt filen
        for (int i = 0; i < num_lines; i++) {
            fprintf(tempFile, "%s %lf\n", data[i].Date, data[i].pH);
        }

        fclose(tempFile);


        // popen (process open) og pclose (process close)
        FILE *gnuplotPipe = popen("gnuplot -persist", "w");

        if (gnuplotPipe) {

            fprintf(gnuplotPipe, "set xdata time\n");
            fprintf(gnuplotPipe, "set timefmt '%%Y/%%m/%%d %%H.%%M'\n");
            fprintf(gnuplotPipe, "set format x '%%Y-%%m'\n");
            fprintf(gnuplotPipe, "set xlabel 'År/måned'\n");
            fprintf(gnuplotPipe, "set ylabel 'pH værdi'\n");
            fprintf(gnuplotPipe, "set title 'pH værdi over tid'\n");
            fprintf(gnuplotPipe, "set grid\n");
            fprintf(gnuplotPipe, "plot 'graph-data.txt' using 1:3 with lines title 'pH'\n");

            fflush(gnuplotPipe);
            pclose(gnuplotPipe);
        } else {
            printf("Fejl ved åbning af gnuplot, tjek om gnuplot er installeret\n");
        }
    }
    if (strcmp(plotType, "Electrical Conductivity") == 0) {
        FILE *tempFile = fopen("graph-data.txt", "w");
        // Skriv data til den graph-data.txt filen
        for (int i = 0; i < num_lines; i++) {
            fprintf(tempFile, "%s %lf\n", data[i].Date, data[i].ElectricalConductivity);
        }

        fclose(tempFile);

        FILE *gnuplotPipe = popen("gnuplot -persist", "w");

        if (gnuplotPipe) {

            fprintf(gnuplotPipe, "set xdata time\n");
            fprintf(gnuplotPipe, "set timefmt '%%Y/%%m/%%d %%H.%%M'\n");
            fprintf(gnuplotPipe, "set format x '%%Y-%%m'\n");
            fprintf(gnuplotPipe, "set xlabel 'År/måned'\n");
            fprintf(gnuplotPipe, "set ylabel 'Elektrisk resistivitet'\n");
            fprintf(gnuplotPipe, "set title 'Elektrisk resistivitet over tid'\n");
            fprintf(gnuplotPipe, "set grid\n");
            fprintf(gnuplotPipe, "plot 'graph-data.txt' using 1:3 with lines title 'Elektrisk resistivitet'\n");

            fflush(gnuplotPipe);
            pclose(gnuplotPipe);
        } else {
            printf("Fejl ved åbning af gnuplot, tjek om gnuplot er installeret\n");
        }
    }
    if (strcmp(plotType, "Oxygen Level") == 0) {
        FILE *tempFile = fopen("graph-data.txt", "w");
        // Skriv data til den graph-data.txt filen
        for (int i = 0; i < num_lines; i++) {
            fprintf(tempFile, "%s %lf\n", data[i].Date, data[i].OxygenLevel);
        }

        fclose(tempFile);

        FILE *gnuplotPipe = popen("gnuplot -persist", "w");

        if (gnuplotPipe) {

            fprintf(gnuplotPipe, "set xdata time\n");
            fprintf(gnuplotPipe, "set timefmt '%%Y/%%m/%%d %%H.%%M'\n");
            fprintf(gnuplotPipe, "set format x '%%Y-%%m'\n");
            fprintf(gnuplotPipe, "set xlabel 'År/måned'\n");
            fprintf(gnuplotPipe, "set ylabel 'Oxygen niveau'\n");
            fprintf(gnuplotPipe, "set title 'Oxygen niveau over tid'\n");
            fprintf(gnuplotPipe, "set grid\n");
            fprintf(gnuplotPipe, "plot 'graph-data.txt' using 1:3 with lines title 'Oxygen niveau'\n");

            fflush(gnuplotPipe);
            pclose(gnuplotPipe);
        } else {
            printf("Fejl ved åbning af gnuplot, tjek om gnuplot er installeret\n");
        }
    }
    if (strcmp(plotType, "Temperature") == 0) {

        FILE *tempFile = fopen("graph-data.txt", "w");
        // Skriv data til den graph-data.txt filen
        for (int i = 0; i < num_lines; i++) {
            fprintf(tempFile, "%s %lf\n", data[i].Date, data[i].Temperature);
        }

        fclose(tempFile);

        FILE *gnuplotPipe = popen("gnuplot -persist", "w");

        if (gnuplotPipe) {

            fprintf(gnuplotPipe, "set xdata time\n");
            fprintf(gnuplotPipe, "set timefmt '%%Y/%%m/%%d %%H.%%M'\n");
            fprintf(gnuplotPipe, "set format x '%%Y-%%m'\n");
            fprintf(gnuplotPipe, "set xlabel 'År/måned'\n");
            fprintf(gnuplotPipe, "set ylabel 'Temperatur'\n");
            fprintf(gnuplotPipe, "set title 'Temperatur over tid'\n");
            fprintf(gnuplotPipe, "set grid\n");
            fprintf(gnuplotPipe, "plot 'graph-data.txt' using 1:3 with lines title 'Temperatur'\n");

            fflush(gnuplotPipe);
            pclose(gnuplotPipe);
        } else {
            printf("Fejl ved åbning af gnuplot, tjek om gnuplot er installeret\n");
        }
    }
    if (strcmp(plotType, "Water Height") == 0) {
        FILE *tempFile = fopen("graph-data.txt", "w");

        // Skriv data til den graph-data.txt filen
        for (int i = 0; i < num_lines; i++) {
            fprintf(tempFile, "%s %lf\n", data[i].Date, data[i].WaterHeight);
        }

        fclose(tempFile);

        FILE *gnuplotPipe = popen("gnuplot -persist", "w");

        if (gnuplotPipe) {

            fprintf(gnuplotPipe, "set xdata time\n");
            fprintf(gnuplotPipe, "set timefmt '%%Y/%%m/%%d %%H.%%M'\n");
            fprintf(gnuplotPipe, "set format x '%%Y-%%m'\n");
            fprintf(gnuplotPipe, "set xlabel 'År/måned'\n");
            fprintf(gnuplotPipe, "set ylabel 'Vandstand'\n");
            fprintf(gnuplotPipe, "set title 'vandstand over tid'\n");
            fprintf(gnuplotPipe, "set grid\n");
            fprintf(gnuplotPipe, "plot 'graph-data.txt' using 1:3 with lines title 'Vandstand'\n");

            fflush(gnuplotPipe);
            pclose(gnuplotPipe);
        } else {
            printf("Fejl ved åbning af gnuplot, tjek om gnuplot er installeret\n");
        }
    }
}