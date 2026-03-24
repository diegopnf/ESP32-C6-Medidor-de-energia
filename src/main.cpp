#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <LittleFS.h>

// --- CONFIGURAÇÕES DE IDENTIDADE ---
const char* DEVICE_ID = "medidor-esp32c6-01"; 
const char* FW_VERSION = "1.0.3"; 

// --- CONFIGURAÇÕES AWS IOT ---
const char* AWS_IOT_ENDPOINT = "url.iot.us-east-1.amazonaws.com";
const char* AWS_IOT_TOPIC    = "esp32/telemetria"; 

// Estrutura para tráfego de dados entre as Tasks
struct EnergyData {
    float voltage;
    float current;
    float power;
    float pf;
    float frequency;
    float energy;
};

// Variáveis de controle e Handlers
QueueHandle_t energyQueue;
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);
float totalEnergykWh = 0;
unsigned long lastSampleTime = 0;

// Função auxiliar para ler certificados do LittleFS
String readFile(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file) return "";
    String content = file.readString();
    file.close();
    return content;
}

// Conexão Segura com AWS IoT
void connectAWS() {
    if (!LittleFS.begin()) {
        Serial.println("Erro ao montar LittleFS");
        return;
    }

    Serial.println("Carregando certificados da Flash...");
    String cacert = readFile("/AmazonRootCA1.pem");
    String client_cert = readFile("/certificate.pem.crt");
    String privkey = readFile("/private.pem.key");

    if (cacert == "" || client_cert == "" || privkey == "") {
        Serial.println("Erro: Certificados ausentes no Filesystem!");
        return;
    }

    net.setCACert(cacert.c_str());
    net.setCertificate(client_cert.c_str());
    net.setPrivateKey(privkey.c_str());

    client.setServer(AWS_IOT_ENDPOINT, 8883);

    Serial.println("Conectando ao Broker AWS...");
    while (!client.connect(DEVICE_ID)) {
        Serial.print(".");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    Serial.println("\nAWS IoT Conectado!");
}

// --- TASK B: AMOSTRAGEM (Core 1 - Alta Prioridade) ---
void TaskSensors(void *pvParameters) {
    EnergyData sensorData;
    lastSampleTime = millis();

    for (;;) {
        unsigned long now = millis();
        float deltaTimeHours = (now - lastSampleTime) / 3600000.0;
        
        sensorData.voltage = 215.0 + (rand() % 100) / 10.0;
        sensorData.current = (rand() % 500) / 100.0;
        sensorData.pf = 0.85 + (rand() % 15) / 100.0;
        sensorData.frequency = 59.8 + (rand() % 4) / 10.0;
        
        sensorData.power = sensorData.voltage * sensorData.current * sensorData.pf;
        totalEnergykWh += (sensorData.power / 1000.0) * deltaTimeHours;
        sensorData.energy = totalEnergykWh;
        
        lastSampleTime = now;

        xQueueSend(energyQueue, &sensorData, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(2000)); 
    }
}

// --- TASK A: CONECTIVIDADE (Core 0 - Gestão de Rede) ---
void TaskConnectivity(void *pvParameters) {
    EnergyData dataToPublish;
    StaticJsonDocument<512> doc;
    char jsonBuffer[512];

    for (;;) {
        if (WiFi.status() == WL_CONNECTED) {
            if (!client.connected()) {
                connectAWS();
            }
            client.loop();

            if (xQueueReceive(energyQueue, &dataToPublish, portMAX_DELAY) == pdPASS) {
                doc.clear();
                doc["device_id"] = DEVICE_ID;
                doc["timestamp"] = millis();
                
                // --- CONFIGURAÇÃO DE EXPIRAÇÃO (TTL DynamoDB) ---
                // Adiciona 30 dias (em segundos) ao tempo atual do dispositivo
                // Nota: Para precisão real de calendário, recomenda-se usar NTP (Epoch Time)
                doc["expiracao"] = (millis() / 1000) + 2592000; 

                doc["voltage"]   = dataToPublish.voltage;
                doc["current"]   = dataToPublish.current;
                doc["power"]     = dataToPublish.power;
                doc["pf"]        = dataToPublish.pf;
                doc["frequency"] = dataToPublish.frequency;
                doc["energy"]    = dataToPublish.energy;
                doc["version"]   = FW_VERSION;

                serializeJson(doc, jsonBuffer);
                client.publish(AWS_IOT_TOPIC, jsonBuffer);
                
                Serial.print("AWS Publish com TTL: ");
                Serial.println(jsonBuffer);
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    energyQueue = xQueueCreate(5, sizeof(EnergyData));

    WiFiManager wm;
    if(!wm.autoConnect("ESP32-Medidor-Config")) {
        Serial.println("Falha no WiFi. Reiniciando...");
        delay(3000);
        ESP.restart();
    }

    if (energyQueue != NULL) {
        xTaskCreatePinnedToCore(TaskSensors, "TaskSensors", 4096, NULL, 2, NULL, 1);
        xTaskCreatePinnedToCore(TaskConnectivity, "TaskWiFi", 8192, NULL, 1, NULL, 0);
    }
}

void loop() {
    vTaskDelete(NULL); 
}