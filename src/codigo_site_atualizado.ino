#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ESP32Servo.h>
#include <esp_wifi.h>

// ======================================================
// CONFIGURAÇÕES WI-FI
// ======================================================

const char* WIFI_SSID = "RoboticHand";
const char* WIFI_PASSWORD = "maoRobotica123";

WebServer server(80);

// ======================================================
// CONFIGURAÇÕES DOS SERVOS
// ======================================================

const int NUM_SERVOS = 5;

Servo servos[NUM_SERVOS];

const int pinosServos[NUM_SERVOS] = {
  14, 27, 26, 25, 33
};

// ======================================================
// CONFIGURAÇÕES DOS BOTÕES
// ======================================================

const int botoes[NUM_SERVOS] = {
  15, 5, 18, 19, 2
};

// Ângulos solicitados pela interface web
int angulosWeb[NUM_SERVOS] = {
  180, 180, 180, 180, 180
};

// Últimos ângulos aplicados aos servos
int angulosAtuais[NUM_SERVOS] = {
  -1, -1, -1, -1, -1
};

// Estado do LittleFS
bool littleFSDisponivel = false;

// ======================================================
// COMUNICAÇÃO HTTP
// ======================================================

// Configura os cabeçalhos CORS
void liberarCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
}

// ======================================================
// ROTA /servo
// Recebe os comandos dos sliders
// ======================================================

void handleServo() {

  liberarCORS();

  if (!server.hasArg("id") || !server.hasArg("angulo")) {
    server.send(400, "text/plain", "Parametros ausentes");
    return;
  }

  int id = server.arg("id").toInt();
  int angulo = server.arg("angulo").toInt();

  if (id < 0 || id >= NUM_SERVOS) {
    server.send(400, "text/plain", "ID invalido");
    return;
  }

  if (angulo < 0 || angulo > 180) {
    server.send(400, "text/plain", "Angulo invalido");
    return;
  }

  // Atualiza o ângulo solicitado pelo site
  angulosWeb[id] = angulo;

  server.send(200, "text/plain", "OK");
}

// ======================================================
// ROTA /status
// Retorna informações sobre o sistema
// ======================================================

void handleStatus() {

  liberarCORS();

  int servosLigados = 0;

  String json = "{\"angulos\":[";

  for (int i = 0; i < NUM_SERVOS; i++) {

    if (servos[i].attached()) {
      servosLigados++;
    }

    json += String(angulosWeb[i]);

    if (i < NUM_SERVOS - 1) {
      json += ",";
    }
  }

  json += "],\"servos\":" + String(servosLigados);

  json += ",\"ip\":\"" + WiFi.softAPIP().toString() + "\"";

  json += ",\"littleFS\":";
  json += littleFSDisponivel ? "true" : "false";

  json += "}";

  server.send(200, "application/json", json);
}

// ======================================================
// DISPONIBILIZAÇÃO DOS ARQUIVOS LITTLEFS
// ======================================================

void handleArquivos() {

  if (!littleFSDisponivel) {
    server.send(503, "text/plain", "LittleFS indisponivel");
    return;
  }

  String caminho = server.uri();

  if (caminho == "/") {
    caminho = "/index.html";
  }

  String tipo = "text/plain";

  if (caminho.endsWith(".html")) {
    tipo = "text/html";
  }
  else if (caminho.endsWith(".css")) {
    tipo = "text/css";
  }
  else if (caminho.endsWith(".js")) {
    tipo = "application/javascript";
  }
  else if (caminho.endsWith(".png")) {
    tipo = "image/png";
  }
  else if (caminho.endsWith(".jpg") ||
           caminho.endsWith(".jpeg")) {
    tipo = "image/jpeg";
  }
  else if (caminho.endsWith(".svg")) {
    tipo = "image/svg+xml";
  }
  else if (caminho.endsWith(".ico")) {
    tipo = "image/x-icon";
  }
  else if (caminho.endsWith(".gif")) {
    tipo = "image/gif";
  }
  else if (caminho.endsWith(".webp")) {
    tipo = "image/webp";
  }

  if (!LittleFS.exists(caminho)) {
    server.send(404, "text/plain", "Arquivo nao encontrado");
    return;
  }

  File arquivo = LittleFS.open(caminho, "r");

  if (!arquivo) {
    server.send(500, "text/plain", "Erro ao abrir arquivo");
    return;
  }

  server.streamFile(arquivo, tipo);

  arquivo.close();
}

// ======================================================
// CONFIGURAÇÃO DAS ROTAS
// ======================================================

void configurarRotas() {

  server.on("/servo", HTTP_GET, handleServo);

  server.on("/status", HTTP_GET, handleStatus);

  server.onNotFound(handleArquivos);
}

// ======================================================
// CONTROLE DOS SERVOMOTORES
// ======================================================

void atualizarServos() {

  for (int i = 0; i < NUM_SERVOS; i++) {

    int angulo;

    // Botão físico possui prioridade
    if (digitalRead(botoes[i]) == LOW) {

      angulo = 0;

    } else {

      // Botão solto: utiliza o último comando web
      angulo = angulosWeb[i];
    }

    // Atualiza somente quando o ângulo muda
    if (angulo != angulosAtuais[i]) {

      servos[i].write(angulo);

      angulosAtuais[i] = angulo;
    }
  }
}

// ======================================================
// INICIALIZAÇÃO
// ======================================================

void setup() {

  Serial.begin(115200);

  // Inicializa o sistema de arquivos
  littleFSDisponivel = LittleFS.begin(false);

  if (littleFSDisponivel) {
    Serial.println("LittleFS inicializado.");
  } else {
    Serial.println("Erro ao inicializar LittleFS.");
    Serial.println("Arquivos web indisponiveis.");
  }

  // Inicializa servos e botões
  for (int i = 0; i < NUM_SERVOS; i++) {

    pinMode(botoes[i], INPUT_PULLUP);

    servos[i].setPeriodHertz(50);

    servos[i].attach(pinosServos[i], 500, 1500);
  }

  // Cria a rede Wi-Fi
  WiFi.mode(WIFI_AP);

  if (WiFi.softAP(WIFI_SSID, WIFI_PASSWORD)) {

    Serial.println("Rede Wi-Fi iniciada.");

    Serial.print("IP do ESP32: ");

    Serial.println(WiFi.softAPIP());

  } else {

    Serial.println("Erro ao iniciar a rede Wi-Fi.");
  }

  // Inicializa o servidor HTTP
  configurarRotas();

  server.begin();

  Serial.println("Servidor HTTP iniciado.");
}

// ======================================================
// LOOP PRINCIPAL
// ======================================================

void loop() {

  // Processa as requisições HTTP
  server.handleClient();

  // Atualiza os servomotores
  atualizarServos();
}
