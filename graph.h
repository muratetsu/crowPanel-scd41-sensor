// Graph class for m5stack CO2 Sensor
//
// October 28, 2025
// Tetsu Nishimura
#ifndef GRAPH_H
#define GRAPH_H

#include <Arduino.h>
#include <TFT_eSPI.h>

#define BUF_SIZE  512
#define BUF_MASK  (BUF_SIZE - 1)

class Graph {
  public:
  Graph();
  void begin(TFT_eSPI *tft, uint16_t width, uint16_t hight, uint16_t xmin, uint16_t ymin, uint16_t ygrid, uint32_t crVal1, uint32_t crVal2, uint32_t crVal3);
  void add(float val1, float val2, float val3);
  void plot(struct tm *timeinfo, uint8_t labels);

  private:
  TFT_eSprite *_spr = nullptr;
  uint16_t _width;
  uint16_t _height;
  uint16_t _xmin;
  uint16_t _ymin;
  uint16_t _ymid;
  uint16_t _ymax;
  uint16_t _ygrid;
  uint32_t _crVal1;
  uint32_t _crVal2;
  uint32_t _crVal3;
  float _val1[BUF_SIZE];
  float _val2[BUF_SIZE];
  float _val3[BUF_SIZE];
  uint16_t _rp;
  uint16_t _wp;
  uint16_t _screen_ymin;
  void drawline(float* values, uint16_t ystep, uint32_t color, boolean label, uint16_t labelPos);
  float getOffset(float* value);
};

#endif
