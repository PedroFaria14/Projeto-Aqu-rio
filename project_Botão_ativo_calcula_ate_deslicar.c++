/*
  PROJETO IOT CODEPATH - Monitoramento em Tempo Real
*/

#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "Wokwi-GUEST";
char pass[] = "";

#define TRIG_PIN 5
#define ECHO_PIN 18
#define RELAY_PIN 19

// Variáveis de Leitura
long duration;
float distance;
int nivelAguaInteiro; // V0
float volumeAtual;    // V2

// Variáveis de Controle e Cálculo
bool bombaLigada = false;
unsigned long tempoInicioBomba = 0;
const unsigned long TEMPO_MAXIMO_BOMBA = 15000;

int contadorCiclos = 0; // V5
float nivelAnteriorBomba = 0;
float nivelAntesEvap = 0;
float somaReposicoes = 0;
float somaEvaporacao = 0;
int ciclosEvaporacao = 0;

BlynkTimer timer;

void desligarBomba(bool porTrava)
{
    digitalWrite(RELAY_PIN, LOW);
    bombaLigada = false;

    if (porTrava)
    {
        Blynk.virtualWrite(V1, 0);
        Serial.println("ALERTA: Bomba desligada por segurança.");
    }
    else
    {
        Serial.println("Bomba DESLIGADA.");
    }

    // Finaliza o ciclo e calcula a média final da reposição
    float aguaFinalEntrou = volumeAtual - nivelAnteriorBomba;
    if (aguaFinalEntrou > 0)
    {
        contadorCiclos++;
        somaReposicoes += aguaFinalEntrou;
        float mediaReposicao = somaReposicoes / contadorCiclos;

        Blynk.virtualWrite(V5, contadorCiclos);
        Blynk.virtualWrite(V6, mediaReposicao);
    }

    nivelAntesEvap = volumeAtual;
}

void ligarBomba()
{
    digitalWrite(RELAY_PIN, HIGH);
    bombaLigada = true;
    tempoInicioBomba = millis();

    // Captura o nível exato do momento do acionamento
    nivelAnteriorBomba = volumeAtual;
    Serial.println("Bomba LIGADA.");

    // Cálculo de Evaporação
    if (contadorCiclos > 0 || ciclosEvaporacao > 0)
    {
        float evaporou = nivelAntesEvap - volumeAtual;
        if (evaporou > 0.1)
        {
            somaEvaporacao += evaporou;
            ciclosEvaporacao++;
            float mediaEvaporacao = somaEvaporacao / ciclosEvaporacao;
            Blynk.virtualWrite(V4, evaporou);
            Blynk.virtualWrite(V7, mediaEvaporacao);
        }
    }
}

BLYNK_WRITE(V1)
{
    int statusBotao = param.asInt();
    if (statusBotao == 1)
        ligarBomba();
    else
        desligarBomba(false);
}

void lerSensor()
{
    // Pulso do Sensor
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    duration = pulseIn(ECHO_PIN, HIGH);
    distance = duration * 0.034 / 2;

    // Lógica de Volume (Tanque de 10cm)
    float nivelReal = 10.0 - distance;
    if (nivelReal < 0)
        nivelReal = 0;
    if (nivelReal > 10)
        nivelReal = 10.0;

    nivelAguaInteiro = (int)nivelReal;
    volumeAtual = nivelReal;

    // Atualiza Nível e Volume
    Blynk.virtualWrite(V0, nivelAguaInteiro);
    Blynk.virtualWrite(V2, volumeAtual);

    if (bombaLigada)
    {
        float parcialEntrou = volumeAtual - nivelAnteriorBomba;
        if (parcialEntrou < 0)
            parcialEntrou = 0;
        Blynk.virtualWrite(V3, parcialEntrou);
    }

    // Trava de Segurança
    if (bombaLigada && (millis() - tempoInicioBomba > TEMPO_MAXIMO_BOMBA))
    {
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
    timer.setInterval(1000L, lerSensor); // Atualização a cada 1 segundo
}

void loop()
{
    Blynk.run();
    timer.run();
}