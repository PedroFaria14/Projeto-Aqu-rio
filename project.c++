
#define BLYNK_TEMPLATE_ID "TMPL2AxMSq3IA"
#define BLYNK_TEMPLATE_NAME "Projeto Aquário com ESP32"
#define BLYNK_AUTH_TOKEN "wnx2z0a3TgmPEW6xCuVdOAureS2P1s7I"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "Wokwi-GUEST";
char pass[] = "";

#define TRIG_PIN 5
#define ECHO_PIN 18
#define RELAY_PIN 19

long duration;
float distance;
float volumeAtual;
float volumeAnterior = 0;

bool bombaLigada = false;
bool primeiraLeitura = true;

int contadorCiclos = 0;
int ciclosEvaporacao = 0;

float totalReposicaoCiclo = 0;
float totalEvaporacaoCiclo = 0;
float somaGeralReposicoes = 0;
float somaGeralEvaporacao = 0;

BlynkTimer timer;

void desligarBomba(bool porTrava)
{
    digitalWrite(RELAY_PIN, LOW);
    bombaLigada = false;

    if (porTrava)
    {
        Blynk.virtualWrite(V1, 0);
        Serial.println("====================================");
        Serial.println("ALERTA: Bomba desligada (Tanque Cheio).");
    }
    else
    {
        Serial.println("====================================");
        Serial.println("COMANDO: Bomba DESLIGADA.");
    }

    //  REPOSIÇÃO
    if (totalReposicaoCiclo > 0)
    {
        contadorCiclos++;
        somaGeralReposicoes += totalReposicaoCiclo;
        float mediaReposicao = somaGeralReposicoes / contadorCiclos;

        Blynk.virtualWrite(V5, contadorCiclos);
        Blynk.virtualWrite(V6, mediaReposicao);
        Serial.printf("FECHOU CICLO REPOSIÇÃO -> Entrou no total: %.3f L | Média Histórica: %.3f\n", totalReposicaoCiclo, mediaReposicao);

        totalReposicaoCiclo = 0;
    }

    Serial.println("====================================");
}

void ligarBomba()
{
    digitalWrite(RELAY_PIN, HIGH);
    bombaLigada = true;

    Serial.println("====================================");
    Serial.println("COMANDO: Bomba LIGADA.");

    // EVAPORAÇÃO
    if (totalEvaporacaoCiclo > 0)
    {
        ciclosEvaporacao++;
        somaGeralEvaporacao += totalEvaporacaoCiclo;
        float mediaEvaporacao = somaGeralEvaporacao / ciclosEvaporacao;

        Blynk.virtualWrite(V7, mediaEvaporacao);
        Serial.printf("FECHOU CICLO EVAPORAÇÃO -> Sumiu no total: %.3f L | Média Histórica: %.3f\n", totalEvaporacaoCiclo, mediaEvaporacao);

        totalEvaporacaoCiclo = 0;
    }

    Serial.println("====================================");
}

BLYNK_WRITE(V1)
{
    if (param.asInt() == 1)
        ligarBomba();
    else
        desligarBomba(false);
}

void lerSensor()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    duration = pulseIn(ECHO_PIN, HIGH);
    distance = duration * 0.034 / 2;

    float profundidadeSimulador = 15.0 - distance;
    if (profundidadeSimulador < 0)
        profundidadeSimulador = 0;
    if (profundidadeSimulador > 15.0)
        profundidadeSimulador = 15.0;

    // Calcula a proporção baseada no máximo
    volumeAtual = profundidadeSimulador * (1.0 / 15.0);

    if (primeiraLeitura)
    {
        volumeAnterior = volumeAtual;
        primeiraLeitura = false;
    }

    Blynk.virtualWrite(V0, volumeAtual);
    Blynk.virtualWrite(V2, volumeAtual);

    float variacao = volumeAtual - volumeAnterior;

    if (bombaLigada)
    {
        if (variacao > 0.01)
        {
            totalReposicaoCiclo += variacao;
        }
        Blynk.virtualWrite(V3, totalReposicaoCiclo);
        Serial.printf("[ON] Entrando no momento: %.3f L | Acumulado do ciclo: %.3f L\n", variacao > 0 ? variacao : 0, totalReposicaoCiclo);
    }
    else
    {
        if (variacao < -0.01)
        {
            totalEvaporacaoCiclo += (variacao * -1.0);
        }
        Blynk.virtualWrite(V4, totalEvaporacaoCiclo);
        Serial.printf("[OFF] Evaporando no momento: %.3f L | Acumulado do ciclo: %.3f L\n", variacao < 0 ? variacao * -1.0 : 0, totalEvaporacaoCiclo);
    }

    volumeAnterior = volumeAtual;

    // Trava de Transbordamento
    if (bombaLigada && volumeAtual >= 0.98)
    {
        Serial.println("====================================");
        Serial.println("ALERTA: Recipiente CHEIO! Prevenção ativada.");
        desligarBomba(true);
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    timer.setInterval(1000L, lerSensor);
}

void loop()
{
    Blynk.run();
    timer.run();
}