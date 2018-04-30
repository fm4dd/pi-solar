/* ------------------------------------------------------------ *
 * file:        daytcalc.c                                      *
 *                                                              *
 * purpose:     Calculate local sunrise and sunset times.       *
 *              Returns 1 for nighttime, 0 for daytime, or      *
 *              -1 for any errors.                              *
 *                                                              *
 * author:      02/11/2017 Frank4DD                             *
 *                                                              *
 * compile:     gcc daytcalc.c -o daytcalc -lm                  *
 *                                                              *
 * example run: fm@susie:~$ ./daytcalc 1486784589 -v            *
 * 2017-02-05 DST: 0 Sunrise: 6:38 Sunset: 17:11 Duration: 10:33*
 * SunriseTS: 1486244280 SunsetTS: 1486282260 DurationSec: 37980*
 *                                                              *
 * example run: fm@susie:~$ ./daytcalc 1486784589 -f            *
 * date=2017-02-10 sunrise=6:33  sunset=17:18 daytime=10:33     *
 * ------------------------------------------------------------ */
/* http://stackoverflow.com/questions/7064531/sunrise-sunset-times-in-c */
#define _DEFAULT_SOURCE	1
#define PI 3.141592
#define ZENITH -.83

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

/* ------------------------------------------------------------ *
 * print_usage() prints the programs commandline instructions.  *
 * ------------------------------------------------------------ */
void usage() {
   static char const usage[] = "Usage: daytcalc -t timestamp -x longitude -y latitude -z offset -d -f\n\n\
Command line parameters have the following format:\n\
   -t   Unix timestamp, example: 1486784589, optional, defaults to now\n\
   -x   longitude, example: 12.45277778\n\
   -y   latitude, example: 51.340277778\n\
   -z   timezone offset in hrs, example: 9, optional, defaults to local system timezone offset\n\
   -s   timezone name, example: \"Europe/Berlin\", optional, prefered instead of -z option\n\
   -f   output text for redirect into file\n\
   -v   verbose output flag\n\
   -h   print usage flag\n\n\
Usage example:\n\
./daytcalc -t 1486784589 -x 12.45277778 -y 51.340277778 -z 1 -d -f\n";
   printf(usage);
}

bool is_float(const char *s, float *dest) {
  if (s == NULL) return false;
  char *endptr;
  *dest = (float) strtod(s, &endptr);
  if (s == endptr) return false; // no conversion

  // Look at trailing text
  while (isspace((unsigned char ) *endptr))
    endptr++;
  return *endptr == '\0';
}

/* ------------------------------------------------------------ *
 * Global variables and defaults                                *
 * ------------------------------------------------------------ */
int verbose = 0;
time_t calc_t = 0;
float latitude = 0;
float longitude = 0;
long tzoffset = 0;
char tzstring[255] = "";
int txt = 0;

extern char *optarg;
extern int optind, opterr, optopt;

/* ------------------------------------------------------------ *
 * parseargs() checks the commandline arguments with C getopt   *
 * ------------------------------------------------------------ */
void parseargs(int argc, char* argv[]) {
   int arg;
   opterr = 0;

   if(argc == 1) { usage(); exit(-1); }

   while ((arg = (int) getopt (argc, argv, "t:x:y:z:s:vhf")) != -1)
      switch (arg) {
         // arg -t timestamp, type: time_t, example: 1486784589
         // optional, defaults to now
         case 't':
            if(verbose == 1) printf("arg -t, value %s\n", optarg);
            calc_t = (time_t) atoll(optarg);
            if (calc_t < 1) {
               printf("Error: Cannot get valid -t timestamp argument.\n");
               exit(-1);
            }
            break;

         // arg -n longitude, type: float, example: 12.45277777777777
         // optional, defaults to prime meridian 0°0'5.3?W (0.001389)
         // One meter resolution can be represented using 5 decimal places
         case 'x':
            //longitude = atof(optarg);
            if(is_float(optarg, &longitude)) {
               if(verbose == 1) printf("arg -x, string %s, value (float) %e\n", optarg, longitude);
            } else {
               printf("Error: Cannot get valid -y latitude argument.\n");
               exit(-1);
            }
            if(longitude < -180.0f || longitude > 180.0f) {
               printf("Error: longitude value %e is out of range (< -180 or > 180).\n", longitude);
               exit(-1);
            }
            break;

         // arg -n latitude, type: float, example: 51.34027777777778
         // optional, defaults to prime meridian 51°28'40.1?N (51.477778)
         // One meter resolution can be represented using 5 decimal places
         case 'y':
            //latitude = atof(optarg);
            if(is_float(optarg, &latitude)) {
               if(verbose == 1) printf("arg -y, string %s, value (float) %e\n", optarg, latitude);
            } else {
               printf("Error: Cannot get valid -y latitude argument.\n");
               exit(-1);
            }
            if(latitude < -90.0f || latitude > 90.0f) {
               printf("Error: latitude value %e is out of range (< -90 or > 90).\n", latitude);
               exit(-1);
            }
            break;

         // arg -z timezone, type: integer, example: 9
         // optional, defaults to 0
         case 'z':
            if(verbose == 1) printf("arg -z, value %s\n", optarg);
            tzoffset = strtol(optarg, NULL, 10);
            if (tzoffset < -11 || tzoffset > 11) {
               printf("Error: Cannot get valid -z timezone offset argument.\n");
               exit(-1);
            }
            else {
               // convert cmdline tzoffset hours into seconds
               tzoffset = tzoffset*3600; 
            }
            break;

         // arg -s timezone name, type: string, example: "Europe/Berlin"
         // optional, defaults to system time zone
         case 's':
            if(verbose == 1) printf("Debug: arg -s, value %s\n", optarg);
            strncpy(tzstring, optarg, sizeof(tzstring));
            setenv("TZ", tzstring, 1);
            tzset();
            time_t tzget = time(NULL);
            struct tm lt = {0};
            localtime_r(&tzget, &lt);
            tzoffset = lt.tm_gmtoff;
            break;

         // arg -v verbose, type: flag, optional
         case 'v':
            verbose = 1; break;

         // arg -h usage, type: flag, optional
         case 'h':
            usage(); exit(0);

         // arg -f, type: flag, optional, creates text output
         case 'f':
            if(verbose == 1) printf("arg f, creating text\n");
            txt = 1; break;

         case '?':
            if (isprint (optopt))
               printf ("Error: Unknown option `-%c'.\n", optopt);
            else
               printf ("Error: Unknown option character `\\x%x'.\n", optopt);

         default:
            usage();
    }
    if (calc_t < 1) {
      calc_t = time(NULL);
      if(verbose == 1) printf("Missing -t arg, set calc_t to now %ld\n", calc_t);
    }
}

float calculateSunrise(int day, float lat, float lng) {

   //1. convert the longitude to hour value and calculate an approximate time
   float lngHour = lng / 15.0;
   float t = day + ((6 - lngHour) / 24);   //if rising time is desired:

   //2. calculate the Sun's mean anomaly
   float M = (0.9856 * t) - 3.289;

   //3. calculate the Sun's true longitude
   float L = fmod(M + (1.916 * sin((PI/180)*M)) + (0.020 * sin(2 *(PI/180) * M)) + 282.634,360.0);

   //4a. calculate the Sun's right ascension
   float RA = fmod(180/PI*atan(0.91764 * tan((PI/180)*L)),360.0);

   //4b. right ascension value needs to be in the same quadrant as L
   float Lquadrant  = floor( L/90) * 90;
   float RAquadrant = floor(RA/90) * 90;
   RA = RA + (Lquadrant - RAquadrant);

   //4c. right ascension value needs to be converted into hours
   RA = RA / 15;

   //5. calculate the Sun's declination
   float sinDec = 0.39782 * sin((PI/180)*L);
   float cosDec = cos(asin(sinDec));

   //6a. calculate the Sun's local hour angle
   float cosH = (sin((PI/180)*ZENITH) - (sinDec * sin((PI/180)*lat))) / (cosDec * cos((PI/180)*lat));
   /*
   if (cosH >  1)
   the sun never rises on this location (on the specified date)
   if (cosH < -1)
   the sun never sets on this location (on the specified date)
   */

   //6b. finish calculating H and convert into hours
   float H = 360 - (180/PI)*acos(cosH);   //   if if rising time is desired:
   H = H / 15;

   //7. calculate local mean time of rising/setting
   float T = H + RA - (0.06571 * t) - 6.622;

   //8. adjust back to UTC
   float UT = fmod(T - lngHour,24.0);
   UT = UT + (((float) tzoffset)/3600);
   return UT;
}

float calculateSunset(int day, float lat, float lng) {

   //1. convert the longitude to hour value and calculate an approximate time
   float lngHour = lng / 15.0;
   float t = day + ((18 - lngHour) / 24);   // if setting time is desired:

   //2. calculate the Sun's mean anomaly
   float M = (0.9856 * t) - 3.289;

   //3. calculate the Sun's true longitude
   float L = fmod(M + (1.916 * sin((PI/180)*M)) + (0.020 * sin(2 *(PI/180) * M)) + 282.634,360.0);

   //4a. calculate the Sun's right ascension
   float RA = fmod(180/PI*atan(0.91764 * tan((PI/180)*L)),360.0);

   //4b. right ascension value needs to be in the same quadrant as L
   float Lquadrant  = floor( L/90) * 90;
   float RAquadrant = floor(RA/90) * 90;
   RA = RA + (Lquadrant - RAquadrant);

   //4c. right ascension value needs to be converted into hours
   RA = RA / 15;

   //5. calculate the Sun's declination
   float sinDec = 0.39782 * sin((PI/180)*L);
   float cosDec = cos(asin(sinDec));

   //6a. calculate the Sun's local hour angle
   float cosH = (sin((PI/180)*ZENITH) - (sinDec * sin((PI/180)*lat))) / (cosDec * cos((PI/180)*lat));
   /*
   if (cosH >  1)
   the sun never rises on this location (on the specified date)
   if (cosH < -1)
   the sun never sets on this location (on the specified date)
   */

   //6b. finish calculating H and convert into hours
   float H = (180/PI)*acos(cosH);    // if setting time is desired:
   H = H / 15;

   //7. calculate local mean time of rising/setting
   float T = H + RA - (0.06571 * t) - 6.622;

   //8. adjust back to UTC
   float UT = fmod(T - lngHour,24.0);
   UT = UT + (((float) tzoffset)/3600);
   return UT;
}

int main(int argc, char* argv[]) {

   /* ------------------------------------------------------------ *
    * Create system timezone offset, may be overwritten by -z arg  *
    * ------------------------------------------------------------ */
   time_t tzf = time(NULL);
   struct tm lt = {0};
   localtime_r(&tzf, &lt);
   tzoffset = lt.tm_gmtoff;

   parseargs(argc, argv);

   if(verbose == 1) printf("Local timezone diff: %lds (%ldhrs)\n", tzoffset, tzoffset/3600);

   /* ------------------------------------------------------------ *
    * We convert the timestamp string to a time based struct       *
    * ------------------------------------------------------------ */
   time_t calc_ttz = calc_t + tzoffset;
   struct tm calc_tm;
   calc_tm.tm_gmtoff = 0;

   calc_tm = *gmtime(&calc_ttz);
   int year = calc_tm.tm_year+1900;
   int mon = calc_tm.tm_mon+1;
   int day = calc_tm.tm_mday;
   int day_year = calc_tm.tm_yday+1;

   if(verbose == 1) {
      printf("Origin UTCtimestamp: %ld\n", (long) calc_t);
      printf("Local calctimestamp: %ld\n", (long) calc_ttz);
      printf("Local timezone date: %s", asctime(&calc_tm));
      printf("The day of the year: %d\n", day_year);
   }

   if(verbose == 1) {
   }

   /* ------------------------------------------------------------ *
    * calculateSunrise() /calculateSunset() argument description:  *
    * -----------------------------------------------------------  *
    * day: # day of the year to calculate the sunrise/sunset for.  *
    * lat, lng: the locations latitude and longitude, in decimal.  *
    * ------------------------------------------------------------ */
   float sunriseUT = calculateSunrise(day_year, latitude, longitude);
   double sunrise_hr = fmod(24 + sunriseUT,24.0);
   double sunrise_min = modf(fmod(24+sunriseUT,24.0),&sunrise_hr)*60;

   float sunsetUT = calculateSunset(day_year, latitude, longitude);
   double sunset_hr = fmod(24 + sunsetUT,24.0);
   double sunset_min = modf(fmod(24+sunsetUT,24.0),&sunset_hr)*60;

   struct tm sunrise_local;
   sunrise_local.tm_year = year-1900;
   sunrise_local.tm_mon = mon-1;
   sunrise_local.tm_mday = day;
   sunrise_local.tm_hour = (int) sunrise_hr;
   sunrise_local.tm_min = (int) (sunrise_min+0.5);
   sunrise_local.tm_sec = 0;
   sunrise_local.tm_gmtoff = tzoffset;

   time_t sunrise = timegm(&sunrise_local);
   if(sunrise == -1) printf("Error creating sunrise timestamp");

   struct tm sunset_local;
   sunset_local.tm_year = year-1900;
   sunset_local.tm_mon = mon-1;
   sunset_local.tm_mday = day;
   sunset_local.tm_hour = (int) sunset_hr;
   sunset_local.tm_min = (int) (sunset_min+0.5);
   sunset_local.tm_sec = 0;
   sunset_local.tm_gmtoff = tzoffset;

   time_t sunset = timegm(&sunset_local);
   if(sunset == -1) printf("Error creating sunset timestamp");

   long daytime = sunset-sunrise;
   int daytime_hr = daytime / 3600;
   int daytime_min = (daytime % 3600) / 60;

   /* ------------------------------------------------------------ *
    * The calculation for sunrise_hr/min came up with 5:60, so we  *
    * use the timestamp-converted time data which fixes the issue. *
    * ------------------------------------------------------------ */
   char rise[6];
   strftime(rise, sizeof(rise), "%H:%M", &sunrise_local);
   char sset[6];
   strftime(sset, sizeof(sset), "%H:%M", &sunset_local);

   if(verbose == 1) {
      printf("\n");
      printf("Local sunrise: %2.0f:%2.0f sunset: %2.0f:%2.0f\n", sunrise_hr, sunrise_min, sunset_hr, sunset_min);
      printf("Local sunrise: %s", asctime(&sunrise_local));
      printf("Local  sunset: %s", asctime(&sunset_local));
      printf("Daylight time: %d:%d\n", daytime_hr, daytime_min);
      printf("Calc TS: %ld SunriseTS: %ld SunsetTS: %ld\n", calc_ttz, sunrise, sunset);
   }

   /* ------------------------------------------------------------- *
    * Here we create the html output string for use with index.php  *
    * &nbsp; &#9788; 6:35 &#9790; 16:20 &#9788;&#10142;&#9790; 9:45 *
    * ------------------------------------------------------------- */
   if(txt == 1) {
      printf("&nbsp; &#9788; %s &#9790; %s &#9788; &#10142; &#9790; %.2d:%.2d\n",
              rise, sset, daytime_hr, daytime_min);
   }
   char darkness[2][6] = { "day", "night" };
   int daytimeflag = 0;
   if(calc_ttz < sunrise) { 
      daytimeflag = 1;
      if(verbose == 1) printf("ts %ld < sr %ld, dayt %d (%s)\n", calc_ttz, sunrise, daytimeflag, darkness[daytimeflag]);
   }
   if(calc_ttz > sunset) {
      daytimeflag = 1;
      if(verbose == 1) printf("ts %ld > ss %ld, dayt %d (%s)\n", calc_ttz, sunset, daytimeflag, darkness[daytimeflag]);
   }
   if(verbose == 1) printf("RRD return value: %d (%s)\n", daytimeflag, darkness[daytimeflag]);

   exit(daytimeflag);
}
