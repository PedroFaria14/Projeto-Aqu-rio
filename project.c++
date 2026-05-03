
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

// Variáveis de Controle da Bomba
bool bombaLigada = false;
unsigned long tempoInicioBomba = 0;
const unsigned long TEMPO_MAXIMO_BOMBA = 15000;

// Variáveis de Reposição e Evaporação
int contadorCiclos = 0;   // V5
float nivelAnterior = 0;  // Nível antes de ligar a bomba
float nivelAntesEvap = 0; // Nível antes de desligar a bomba
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
        Blynk.virtualWrite(V1, 0); // Atualiza (V1)
        Serial.println("ALERTA: Trava ativada! Bomba desligada após 15s.");
    }
    else
    {
        Serial.println("Bomba DESLIGADA.");
    }

    // Calcula a Reposição
    float aguaQueEntrou = volumeAtual - nivelAnterior;
    if (aguaQueEntrou > 0)
    {
        somaReposicoes += aguaQueEntrou;
        contadorCiclos++;                                       // V5
        float mediaReposicao = somaReposicoes / contadorCiclos; // V6

        Blynk.virtualWrite(V3, aguaQueEntrou);
        Blynk.virtualWrite(V5, contadorCiclos);
        Blynk.virtualWrite(V6, mediaReposicao);
    }

    // Salva o nível atual para medir a próxima evaporação
    nivelAntesEvap = volumeAtual;
}

void ligarBomba()
{
    digitalWrite(RELAY_PIN, HIGH);
    bombaLigada = true;
    tempoInicioBomba = millis();

    Serial.println("Bomba LIGADA.");

    // Calcula a Evaporação
    if (contadorCiclos > 0 || ciclosEvaporacao > 0)
    {
        float evaporou = nivelAntesEvap - volumeAtual;
        if (evaporou > 0.1)
        {
            somaEvaporacao += evaporou;
            ciclosEvaporacao++;
            float mediaEvaporacao = somaEvaporacao / ciclosEvaporacao; // V7

            Blynk.virtualWrite(V4, evaporou);
            Blynk.virtualWrite(V7, mediaEvaporacao);
        }
    }

    // Salva o nível atual para medir a próxima reposição
    nivelAnterior = volumeAtual;
}

// Escuta o botão  no V1
BLYNK_WRITE(V1)
{
    int statusBotao = param.asInt();
    if (statusBotao == 1)
    {
        ligarBomba();
    }
    else
    {
        desligarBomba(false);
    }
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

    float nivelReal = 10.0 - distance;
    if (nivelReal < 0)
        nivelReal = 0;
    if (nivelReal > 10)
        nivelReal = 10.0;

    nivelAguaInteiro = (int)nivelReal; // Para o V0 (Inteiro)
    volumeAtual = nivelReal;           // Para o V2 (Duplo)

    // Atualiza Nível e Volume no painel
    Blynk.virtualWrite(V0, nivelAguaInteiro);
    Blynk.virtualWrite(V2, volumeAtual);

    // Verifica a Trava de Segurança
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

    Serial.println("Conectando...");
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

    timer.setInterval(1000L, lerSensor);
}

void loop()
{
    Blynk.run();
    timer.run();
}