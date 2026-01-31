// Graph class for m5stack CO2 Sensor
//
// October 28, 2025
// Tetsu Nishimura

#include <float.h>
#include "Graph.h"

Graph::Graph() {}

/**
 * @brief Initialize graph with given width, height, x minimum, y minimum, y grid
 * and colors for each values.
 *
 * @param width Width of the graph
 * @param height Height of the graph
 * @param xmin X minimum of the graph
 * @param ymin Y minimum of the graph
 * @param ygrid Y grid of the graph
 * @param crVal1 Color for the first value
 * @param crVal2 Color for the second value
 * @param crVal3 Color for the third value
 */
/**
 * @brief Initialize graph with given width, height, x minimum, y minimum, y grid
 * and colors for each values.
 */
void Graph::begin(TFT_eSPI *tft, uint16_t width, uint16_t height, uint16_t xmin, uint16_t ymin, uint16_t ygrid, uint32_t crVal1, uint32_t crVal2, uint32_t crVal3) {
    memset(_val1, 0, sizeof(_val1));
    memset(_val2, 0, sizeof(_val2));
    memset(_val3, 0, sizeof(_val3));
    _wp = _rp = 0;
    _width = width;
    _height = height;
    _xmin = xmin;
    _screen_ymin = ymin;
    _ymin = 0;
    _ymid = _ymin + height / 2;
    _ymax = _ymin + height;
    _ygrid = ygrid;
    _crVal1 = crVal1;
    _crVal2 = crVal2;
    _crVal3 = crVal3;

    if (_spr == nullptr) {
        _spr = new TFT_eSprite(tft);
        // Use full screen width (320) to accommodate left and right labels
        _spr->createSprite(320, _height + 2);
    }
}

/**
 * @brief Add new data points to the graph
 */
void Graph::add(float val1, float val2, float val3) {
    _val1[_wp] = val1;
    _val2[_wp] = val2;
    _val3[_wp] = val3;
    _wp++;
    _wp &= BUF_MASK;

    if (((_wp - _rp) & BUF_MASK) > _width) {
        _rp++;
        _rp &= BUF_MASK;
    }
}

/**
 * @brief Plot the graph
 */
void Graph::plot(struct tm *timeinfo, uint8_t labels) {
    if (_spr == nullptr) return;

    int8_t hour = timeinfo->tm_hour;
    uint8_t minute = timeinfo->tm_min;

    // Clear graph
    _spr->fillSprite(TFT_BLACK);
    _spr->setTextFont(1); // Explicitly set Font 1
    _spr->setTextDatum(0);
    _spr->setTextColor(TFT_WHITE);

    // draw vertical grid
    // X loops from right edge (_xmin + _width) down to _xmin
    for (int n = _xmin + _width - 1 - minute; n > _xmin; n -= 60) {
        _spr->drawLine(n, _ymin, n, _ymax, TFT_LIGHTGREY);

        if (n + 20 < _xmin + _width) {
            _spr->drawNumber(hour, n + 2, _ymax - 8);
        }

        if (--hour < 0) {
            hour = 23;
        }
    }

    // draw horizontal grid
    _spr->drawLine(_xmin, _ymid, _xmin + _width - 1, _ymid, TFT_DARKGREY);
    for (int n = _ygrid; n < _height / 2; n += _ygrid) {
        _spr->drawLine(_xmin, _ymid - n, _xmin + _width - 1, _ymid - n, TFT_DARKGREY);
        _spr->drawLine(_xmin, _ymid + n, _xmin + _width - 1, _ymid + n, TFT_DARKGREY);
    }

    // plot line
    // ystep values: CO2=200, Temp=2, Humid=5
    // labelPos: CO2=Left(_xmin), Temp=Right(306), Humid=Right(319)
    drawline(_val1, 200, _crVal1, labels & 0x01, _xmin);   
    drawline(_val2, 2, _crVal2, labels & 0x02, 306);  
    drawline(_val3, 5, _crVal3, labels & 0x04, 319);  

    // Push sprite to the screen at X=0 (since sprite is full width)
    _spr->pushSprite(0, _screen_ymin); 
}

void Graph::drawline(float* values, uint16_t ystep, uint32_t color, boolean label, uint16_t labelPos) {
    if (_spr == nullptr) return;

    // Serial.printf("drawline: %x %d\n", color, label);

    float offset = getOffset(values);
    uint16_t y;

    // check if the latest point is out of range
    float dataSpan = _height * ystep / _ygrid;
    float offsetLow = values[_wp ? _wp - 1 : BUF_SIZE - 1] - dataSpan / 2;
    float offsetHigh = values[_wp ? _wp - 1 : BUF_SIZE - 1] + dataSpan / 2;
    
    if (_wp == _rp && _wp == 0) return; 

    int latestIdx = (_wp - 1 + BUF_SIZE) & BUF_MASK;
    offsetLow = values[latestIdx] - dataSpan / 2;
    offsetHigh = values[latestIdx] + dataSpan / 2;

    if (offset < offsetLow) {
        offset = ceilf(offsetLow / ystep) * ystep;
    }
    else if (offset > offsetHigh) {
        offset = floorf(offsetHigh / ystep) * ystep;
    }
    else {
        offset = roundf(offset / ystep) * ystep; 
    }

    // draw line
    for (int i = 0, pt = (_wp - 1 + BUF_SIZE) & BUF_MASK; i < ((_wp - _rp + BUF_SIZE) & BUF_MASK); i++) {
        y = _ymid - _ygrid * (values[pt] - offset) / ystep;
        pt--;
        pt &= BUF_MASK;

        if (y > _ymin && y < _ymax) {
            // Draw relative to _xmin + _width
            _spr->drawLine(_xmin + _width - 1 - i, y - 1, _xmin + _width - 1 - i, y + 1, color);
        }
    }

    if (label) {
        _spr->setTextDatum(2); // Top-Right
        _spr->setTextColor(color);
        _spr->drawNumber((long)offset, labelPos, _ymid - 4);
        for (int n = 1; n < _height / _ygrid / 2; n++) {
            _spr->drawNumber((long)(offset + n * ystep), labelPos, _ymid - 4 - n * _ygrid);
            _spr->drawNumber((long)(offset - n * ystep), labelPos, _ymid - 4 + n * _ygrid);
        }
    }
}

float Graph::getOffset(float* value) {
    int n = _rp;
    float vmin = FLT_MAX;
    float vmax = -FLT_MAX; // Fixed from FLT_MIN (which is smallest positive normalized float)

    if (_rp == _wp) return 0; // Empty

    while (n != _wp) {
        if (value[n] < vmin) vmin = value[n];
        if (value[n] > vmax) vmax = value[n];
        n++;
        n &= BUF_MASK;
    }

    return (vmin + vmax) / 2;
}

