#pragma once

#include <cstdint>
#include "GC9A01_TFT_LCD_RDL.hpp"
#include "Display.h"

#define RGBColor(r, g, b) GC9A01A_t3n::Color565(r, g, b)

typedef struct {
  int8_t  cs;              // Chip select pin.
  int8_t  dc;              // DC pin.
  int8_t  mosi;            // MOSI pin.
  int8_t  sck;             // SCK pin.
  int8_t  rst;             // reset pin, or -1 to disable reset.
  uint8_t rotation;        // The rotation value for the display (0-3).
  bool    mirror;          // Mirror the display in the X direction.
  bool    useFrameBuffer;  // Whether to use frame buffering.
  bool    asyncUpdates;    // Whether to update the screen asynchronously (for better performance).
} GC9A01A_Config;

inline constexpr uint16_t TFT_WIDTH  = 240;;  // Screen width in pixels
inline constexpr uint16_t TFT_HEIGHT = 240;   // Screen height in pixels

inline constexpr uint16_t WHITE = color16_graphics::pixel_color565_e::RDLC_WHITE;
inline constexpr uint16_t BLACK = color16_graphics::pixel_color565_e::RDLC_BLACK;

class GC9A01A_Display : public Display<GC9A01A_Display> {
private:

  bool asyncUpdates;
  bool mirror;
  bool useBuffer;
  int  displayNum;
  GC9A01_TFT *display;

#ifdef SHOW_FPS
  uint32_t framesDrawn{};
  uint32_t fps{};
  elapsedMillis elapsed{};
#endif

public:
  /// Creates a generic wrapper for a 240x240 GC9A01A round display screen.
  /// \param config the screen's configuration.
  /// \param spiSpeed the speed of the SPI bus. For maximum performance, set this as high as you can get
  /// away with. It will depend on the displays themselves, wire lengths, shielding/interference etc. My
  /// setup works up to about 90,000,000. At 100,000,000 I start seeing corruption on the displays.
  GC9A01A_Display(const GC9A01A_Config &config, const uint32_t spiSpeed = 30'000'000) :
    display(new GC9A01_TFT()), 
    asyncUpdates(config.asyncUpdates), 
    mirror(config.mirror) {

    int8_t RST_TFT          = config.rst;
    int8_t DC_TFT           = config.dc;
    int    GPIO_CHIP_DEVICE = 0;          // GPIO chip device number usually 0
    int    HWSPI_DEVICE     = 0;          // A SPI device, >= 0. which SPI interface to use
    int    HWSPI_CHANNEL    = config.cs;  // A SPI channel, >= 0. Which Chip enable pin to use
    int    HWSPI_SPEED      = spiSpeed;   // The speed of serial communication in bits per second.
    int    HWSPI_FLAGS      = 0;          // last 2 LSB bits define SPI mode, see readme, mode 0 for this device

    Serial.println("TFT Start Test 101 HWSPI");

    // ** USER OPTION 1 GPIO  **
    display->TFTSetupGPIO(RST_TFT, DC_TFT);
    //*******************************************

    // ** USER OPTION 2 Screen Setup**
    display->TFTInitScreenSize(TFT_WIDTH , TFT_HEIGHT);
    // ***********************************

    // ** USER OPTION 3 SPI settings **
    if(display->TFTInitSPI(HWSPI_DEVICE, HWSPI_CHANNEL, HWSPI_SPEED, HWSPI_FLAGS, GPIO_CHIP_DEVICE) != rdlib::Success) {
        Serial.println("Could not initialize TFT");
    }
    //*****************************
    delayMilliSecRDL(100);
    
    setRotation(config.rotation);
    mirror = config.mirror;

    static size_t displayNum{};
    Serial.print("Init GC9A01A display #");
    Serial.print(displayNum);
    Serial.print(": rotate=");
    Serial.print(config.rotation);
    Serial.print(", mirror=");
    Serial.println(config.mirror);
    
    display->setTextColor(WHITE, BLACK);

    if (config.useFrameBuffer) {
      
      Serial.print(displayNum);
      Serial.print(": useFrameBuffer() ");
      display->setAdvancedScreenBuffer_e(display->AdvancedScreenBuffer_e::On);
      auto err = display->setBuffer();
      if (err != rdlib::Return_Codes_e::Success) {
        useBuffer = false;
        Serial.println("failed");
      } else {
        useBuffer = true;
        Serial.println("OK");
      }
    }
    this->displayNum = displayNum++;
  }

  ~GC9A01A_Display() {
    delete display;
  }

  void setRotation(uint8_t rotation) {
      color16_graphics::display_rotate_e rot;
      switch(rotation) {
      case 0: rot = color16_graphics::Degrees_0; break;
      case 1: rot = color16_graphics::Degrees_90; break;
      case 2: rot = color16_graphics::Degrees_180; break;
      case 3: rot = color16_graphics::Degrees_270; break;
      other: Serial.println("Bad rotation");
      }

      display->TFTsetRotation(rot);
  }
  
  inline int16_t handed(int16_t x) {
    return mirror ? (TFT_WIDTH - 1 - x) : x;
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color565) {
      display->drawPixel(handed(x), y, color565);
  }

  void drawFastVLine(int16_t x, int16_t y, int16_t height, uint16_t color565) {
      display->drawFastVLine(handed(x), y, height, color565);
  }

  void drawText(int16_t x, int16_t y, char *text) {
      display->writeCharString(handed(x), y, text);
  }

  void update() {
    if (useBuffer) {
      display->writeBuffer();
    }
  }

  bool isAvailable() const {
    return true;
  }

};
