# WeatherUnderground-Weather-Display-on-an-ILI9341-TFT
The Weather Underground Forecast API is used to extract data on the current weather weather conditions and then the results are displayed on a TFT ILI9341.

There is a switch to display data as Imperial(US) or Metric or Hybrid the latter displaying wind speed as MPH and rainfail as mm.

Recent changes to the Adafruit ILI9341 driver prevent BMP drawing. Only the following versions have been tested and work:
#include <Adafruit_GFX.h>     // v1.1.8 functions correctly
#include <Adafruit_ILI9341.h> // Only v1.02 functions correctly, changes made in 1.03 prevent BMP drawing
