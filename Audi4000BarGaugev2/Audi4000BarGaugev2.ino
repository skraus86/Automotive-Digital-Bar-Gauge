#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ------------------- TFT Display -------------------
TFT_eSPI tft = TFT_eSPI();

const int gaugeCount = 3;
const int gaugeWidth = 20;
const int gaugeHeight = 200;
const int gaugeBorder = 2;
const int tickLength = 2;
const int numTicks = 5;

int gaugeX[gaugeCount] = {5, 60, 110};  // spacing for 3 gauges
int gaugeY = 30;

// Ranges for each gauge
float gaugeMin[gaugeCount] = {0, 0, 0};
float gaugeMax[gaugeCount] = {120, 170, 25};  // °C for first 2, bar for last

String gaugeUnits[gaugeCount] = {"C", "C", "PSI"};

// ------------------- OLED Display -------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define SDA_PIN 11
#define SCL_PIN 12

// ------------------- Data Variables -------------------
unsigned long counter = 0;
int demoVals[gaugeCount] = {0, 1365, 2730}; // simulated sensor values

// ------------------- Color blending -------------------
uint16_t blendColors(uint16_t color1, uint16_t color2, float t) {
  uint8_t r1 = (color1 >> 11) & 0x1F;
  uint8_t g1 = (color1 >> 5) & 0x3F;
  uint8_t b1 = color1 & 0x1F;

  uint8_t r2 = (color2 >> 11) & 0x1F;
  uint8_t g2 = (color2 >> 5) & 0x3F;
  uint8_t b2 = color2 & 0x1F;

  uint8_t r = r1 + (r2 - r1) * t;
  uint8_t g = g1 + (g2 - g1) * t;
  uint8_t b = b1 + (b2 - b1) * t;

  return (r << 11) | (g << 5) | b;
}

uint16_t getGaugeColor(float percent) {
  uint16_t green = TFT_GREEN;
  uint16_t yellow = TFT_YELLOW;
  uint16_t red = TFT_RED;

  if (percent <= 0.5f) {
    float t = percent / 0.5f;
    return blendColors(green, yellow, t);
  } else {
    float t = (percent - 0.5f) / 0.5f;
    return blendColors(yellow, red, t);
  }
}

// ------------------- Setup -------------------
void setup() {
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);  // Backlight on

  Serial.begin(115200);

  tft.init();
  tft.setRotation(4);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(5, 5);
  tft.println("WTR OIL BST");

  // Draw gauge outlines and tick marks
  for (int g = 0; g < gaugeCount; g++) {
    // Outline
    tft.drawRect(
      gaugeX[g] - gaugeBorder,
      gaugeY - gaugeBorder,
      gaugeWidth + gaugeBorder * 2,
      gaugeHeight + gaugeBorder * 2,
      TFT_WHITE
    );

    // Tick marks and labels (real units now)
    for (int i = 0; i < numTicks; i++) {
      int y = gaugeY + (gaugeHeight * i) / (numTicks - 1);
      tft.drawFastHLine(gaugeX[g] + gaugeWidth + 4, y, tickLength, TFT_WHITE);

      // Compute value for this tick
      float val = gaugeMax[g] - (i * (gaugeMax[g] - gaugeMin[g]) / (numTicks - 1));

      tft.setCursor(gaugeX[g] + gaugeWidth + tickLength + 8, y - 6);
      tft.setTextSize(1);
      if (g == 2)  // pressure with decimals
        tft.printf("%.1f", val);
      else         // temperature no decimals
        tft.printf("%.0f", val);
    }

    // Label gauge
    tft.setTextSize(1);
    tft.setCursor(gaugeX[g], gaugeY + gaugeHeight + 20);
    tft.printf("G%d (%s)", g + 1, gaugeUnits[g].c_str());
  }

  // OLED
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) ;
  }
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("Trip Gauge");
  oled.display();
}

// ------------------- Loop -------------------
void loop() {
  // Simulate input for each gauge (replace with analogRead if using sensors)
  for (int g = 0; g < gaugeCount; g++) {
    demoVals[g] += 50;
    if (demoVals[g] > 4095) demoVals[g] = 0;
  }

  // Update each gauge
  for (int g = 0; g < gaugeCount; g++) {
    int raw = demoVals[g];  // or analogRead(sensorPins[g]);
    float percent = (float)raw / 4095.0f;
    int fillH = map(raw, 0, 4095, 0, gaugeHeight);
    fillH = constrain(fillH, 0, gaugeHeight);

    // Map raw to physical value
    float value = gaugeMin[g] + percent * (gaugeMax[g] - gaugeMin[g]);

    // Clear gauge interior
    tft.fillRect(gaugeX[g], gaugeY, gaugeWidth, gaugeHeight, TFT_BLACK);

    // Fill bar
    if (fillH > 0) {
      int fillY = gaugeY + gaugeHeight - fillH;
      uint16_t color = getGaugeColor(percent);
      tft.fillRect(gaugeX[g], fillY, gaugeWidth, fillH, color);
    }

    // Draw numeric value below gauge
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.fillRect(gaugeX[g] - 10, gaugeY + gaugeHeight + 30, 70, 25, TFT_BLACK);

    tft.setCursor(gaugeX[g], gaugeY + gaugeHeight + 35);
    if (g == 2)
      tft.printf("%.1f %s", value, gaugeUnits[g].c_str());
 
    else
      tft.printf("%.0f %s", value, gaugeUnits[g].c_str());

  }

  // Update OLED counter
  counter++;
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 10);
  oled.println("Trip");
  oled.setTextSize(2);
  oled.setCursor(0, 25);
  oled.print(counter);
  oled.println(" Miles");
  oled.display();

  delay(200);
}