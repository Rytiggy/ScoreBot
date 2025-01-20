#include <LilyGo_RGBPanel.h>
#include <LV_Helper.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <lvgl.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// Global variables and constants
LilyGo_RGBPanel panel;
WebSocketsClient webSocketClient;
lv_obj_t *startButton = NULL;
lv_obj_t *buttonLabel = NULL;
const char *WIFI_SSID = "IOT";
const char *WIFI_PASSWORD = "";

// Function prototypes
void setupWiFi();
void initializeDisplay();
void initializeWebSocket();
void displayMessage(const char *message);
void createStartButton();
void handleWebSocketEvent(WStype_t eventType, uint8_t *payload, size_t length);
void handleButtonClick(lv_event_t *event);
void sendStartGameRequest();
void drawVersionText();
void clearScreen();

void setup()
{
  Serial.begin(115200);

  setupWiFi();
  initializeDisplay();
  initializeWebSocket();

  createStartButton();
  drawVersionText();
  panel.setBrightness(16);
}

void loop()
{
  lv_task_handler();      // Handle LVGL tasks
  webSocketClient.loop(); // Handle WebSocket events
  delay(5);
}

// Setup Wi-Fi connection
void setupWiFi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

// Initialize the display
void initializeDisplay()
{
  if (!panel.begin())
  {
    Serial.println("Failed to initialize display");
    while (true)
    {
      delay(1000);
    }
  }
  beginLvglHelper(panel);

  // Set black background
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);
}

// Initialize WebSocket connection
void initializeWebSocket()
{
  webSocketClient.begin("skeeball.local", 80, "/");
  webSocketClient.onEvent(handleWebSocketEvent);
}

// Handle WebSocket events
void handleWebSocketEvent(WStype_t eventType, uint8_t *payload, size_t length)
{
  StaticJsonDocument<200> jsonDocument;
  DeserializationError error = deserializeJson(jsonDocument, payload);

  if (error)
  {
    Serial.println("Failed to parse JSON");
    return;
  }

  const char *eventTypeString = jsonDocument["type"];
  if (!eventTypeString)
  {
    Serial.println("Missing 'type' field in JSON");
    return;
  }

  switch (eventType)
  {
  case WStype_CONNECTED:
    Serial.println("Connected to WebSocket server");
    break;
  case WStype_TEXT:
    Serial.printf("Received WebSocket message: %s\n", payload);

    if (strcmp(eventTypeString, "start") == 0)
    {
      displayMessage("Game Started!");
    }
    else if (strcmp(eventTypeString, "end") == 0)
    {
      createStartButton();
    }
    else if (strcmp(eventTypeString, "end-pending") == 0)
    {
      displayMessage("Game End Pending...");
    }
    else if (strcmp(eventTypeString, "score") == 0)
    {
      int score = jsonDocument["data"]["game"]["score"];
      char scoreMessage[50];
      snprintf(scoreMessage, sizeof(scoreMessage), "Your Score: %d", score);
      displayMessage(scoreMessage);
    }
    break;
  case WStype_DISCONNECTED:
    Serial.println("Disconnected from WebSocket server");
    break;
  default:
    break;
  }
}

// Display a message by removing the button and updating the text
void displayMessage(const char *message)
{
  Serial.println("Updating display message");

  if (startButton != NULL)
  {
    lv_obj_del(startButton);
    startButton = NULL;
  }

  clearScreen();

  lv_obj_t *messageLabel = lv_label_create(lv_scr_act());
  lv_label_set_text(messageLabel, message);

  // Style for arcade-styled text
  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_text_color(&style, lv_color_hex(0xFF0000));
  lv_style_set_text_font(&style, &lv_font_montserrat_40); // Replace with arcade font if available
  lv_obj_add_style(messageLabel, &style, LV_PART_MAIN);

  lv_obj_center(messageLabel);
}

// Create the start button
void createStartButton()
{
  // Create the button
  startButton = lv_btn_create(lv_scr_act());
  lv_obj_set_size(startButton, 300, 150);
  lv_obj_center(startButton);

  // Style for the button
  static lv_style_t style_btn;
  lv_style_init(&style_btn);

  lv_style_set_radius(&style_btn, 20);
  lv_style_set_bg_color(&style_btn, lv_color_hex(0xF32812));
  lv_style_set_bg_grad_color(&style_btn, lv_color_hex(0xE74C3C));
  lv_style_set_bg_grad_dir(&style_btn, LV_GRAD_DIR_HOR);
  lv_style_set_border_width(&style_btn, 2);
  lv_style_set_border_color(&style_btn, lv_color_hex(0xD35400));
  lv_style_set_shadow_width(&style_btn, 10);
  lv_style_set_shadow_color(&style_btn, lv_color_make(255, 107, 107));
  lv_obj_add_style(startButton, &style_btn, LV_PART_MAIN);

  // Add a click event callback
  lv_obj_add_event_cb(startButton, handleButtonClick, LV_EVENT_CLICKED, NULL);

  // Create the label on the button
  buttonLabel = lv_label_create(startButton);

  // Style for the label
  static lv_style_t style_label;
  lv_style_init(&style_label);

  lv_style_set_text_color(&style_label, lv_color_hex(0xFFFFFF)); // White text color
  lv_style_set_text_font(&style_label, &lv_font_montserrat_28);  // Larger font size
  lv_style_set_text_letter_space(&style_label, 2);               // Letter spacing
  lv_style_set_text_align(&style_label, LV_TEXT_ALIGN_CENTER);   // Centered text
  lv_obj_add_style(buttonLabel, &style_label, LV_PART_MAIN);

  // Set the label text
  lv_label_set_text(buttonLabel, "Start");
  lv_obj_center(buttonLabel);
}

// Handle button click event
void handleButtonClick(lv_event_t *event)
{
  LV_LOG_USER("Start button clicked");
  sendStartGameRequest();
}

// Send POST request to start the game
void sendStartGameRequest()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient httpClient;
    httpClient.begin("http://skeeball.local:3000/start-game");

    int responseCode = httpClient.POST("");
    if (responseCode > 0)
    {
      String response = httpClient.getString();
      Serial.println(response);
    }
    else
    {
      Serial.println("Failed to send POST request");
    }
    httpClient.end();
  }
}

// Draw version information on the screen
void drawVersionText()
{
  static lv_style_t style;
  lv_style_init(&style);

  lv_style_set_text_color(&style, lv_color_hex(0xFF0000));
  lv_style_set_text_font(&style, &lv_font_montserrat_20); // Replace with arcade font if available

  lv_obj_t *versionLabel = lv_label_create(lv_scr_act());
  lv_obj_add_style(versionLabel, &style, LV_PART_MAIN);
  lv_label_set_text(versionLabel, "Skeeball\nV1.0.0");
  lv_obj_align(versionLabel, LV_ALIGN_TOP_MID, 0, 10);
}

// Clear the screen (remove all objects)
void clearScreen()
{
  lv_obj_clean(lv_scr_act());
}
