# WeatherUnderground-Weather-Display-on-an-ILI9341-TFT
The Weather Underground Forecast API is used to extract data on the current weather weather conditions and then the results are displayed on a TFT ILI9341.

There is a switch to display data as Imperial(US) or Metric or Hybrid the latter displaying wind speed as MPH and rainfail as mm.

Recent changes to the Adafruit ILI9341 driver prevent BMP drawing. Only the following versions have been tested and work:
#include <Adafruit_GFX.h>     // v1.1.8 functions correctly

#include <Adafruit_ILI9341.h> // Only v1.02 functions correctly, changes made in 1.03 prevent BMP drawing

***** Version 9 now detects North or South Hemisphere locations and reverses the Moon images accordingly.

Install instructions:

1. Copy the sketch .ino to a sketch folder
2. Create a sub-folder called 'data' and copy all the images into the data folder.
3. Use the IDE flash file upload utility to upload the image files t othe data folder.
4. Adjust your City name, Country and Language and add your free WU Key.
   Find your country here: https://www.wunderground.com/weather-by-country.asp
   Click on your Contry to find ytour nearest City/Location that is supported by WU.
   In the USA use your State as Country.
4. Compile and upload the sketch, enjoy.
