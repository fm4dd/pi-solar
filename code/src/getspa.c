/* ------------------------------------------------------------ *
 * file:        getspa.c                                        *
 *                                                              *
 * purpose:     Calculate local sunrise and sunset times,       *
 *              solar zenith and azimuth values for a given     *
 *              longitude, latitude, and timestamp value.       *
 *              Returns 1 for nighttime, 0 for daytime, or      *
 *              -1 for any errors.                              *
 *                                                              *
 * author:      07/15/2018 Frank4DD                             *
 *                                                              *
 * compile:     gcc spa.c getspa.c -o getspa -lm                *
 *                                                              *
 * ------------------------------------------------------------ */
#include <stdio.h>
#include <stdbool.h>   // for bool datatype
#include <stdlib.h>    // for strtod()
#include <ctype.h>     // for isspace()
#include <unistd.h>    // for getopt()
#include <string.h>    // for strncpy()
#include <time.h>      // for struct tm
#include "spa.h"       //include the SPA header file

/* ------------------------------------------------------------ *
 * Global variables and defaults                                *
 * ------------------------------------------------------------ */
int verbose = 0;
time_t calc_t = 0;
float latitude = 0;
float longitude = 0;
long tzoffset = 0;
char tzstring[255] = "";
char htmfile[1024] = "";
int outflag = 0;        // set when arg -f is given
spa_data spa;           //declare the SPA structure
float sunrisemin;
int sunrisesec;
float sunsetmin;
int sunsetsec;

extern char *optarg;
extern int optind, opterr, optopt;


/* ------------------------------------------------------------ *
 * print_usage() prints the programs commandline instructions.  *
 * ------------------------------------------------------------ */
void usage() {
   static char const usage[] = "Usage: getspa -t timestamp -x longitude -y latitude -s timezone -f filename\n\n\
Command line parameters have the following format:\n\
   -t   Unix timestamp, example: 1486784589, optional, defaults to now\n\
   -x   longitude, example: 12.45277778\n\
   -y   latitude, example: 51.340277778\n\
   -z   timezone offset in hrs, example: 9, optional, defaults to local system timezone offset\n\
   -s   timezone name, example: \"Europe/Berlin\", optional, prefered instead of -z option\n\
   -f   write html output to file\n\
   -v   verbose output flag\n\
   -h   print usage flag\n\n\
Usage example:\n\
./getspa -t 1486784589 -x 12.45277778 -y 51.340277778 -s Europe/Berlin -f /home/pi/pi-ws01/web/getspa.htm\n";
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
 * parseargs() checks the commandline arguments with C getopt   *
 * ------------------------------------------------------------ */
void parseargs(int argc, char* argv[]) {
   int arg;
   opterr = 0;

   if(argc == 1) { usage(); exit(-1); }

   while ((arg = (int) getopt (argc, argv, "t:x:y:z:s:vhf:")) != -1)
      switch (arg) {
         // arg -t timestamp, type: time_t, example: 1486784589
         // optional, defaults to now
         case 't':
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

         // arg -f html file name, type: string, example: "/home/pi/pi-ws01/web/getspa.htm"
         // optional
         case 'f':
            outflag = 1;
            strncpy(htmfile, optarg, sizeof(htmfile));
            break;

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

void write_html(char *file){
   /* -------------------------------------------------------- *
    *  Open the html file for writing the table data           *
    * -------------------------------------------------------- */
   FILE *html;
   if(! (html=fopen(file, "w"))) {
      printf("Error open %s for writing.\n", htmfile);
      exit(-1);
   }
   if(verbose == 1) printf("Debug: Writing to file [%s]\n", file);
   /* -------------------------------------------------------- *
    *  Write the solar tracking data table                     *
    * -------------------------------------------------------- */
   fprintf(html, "<table><tr>\n");
   fprintf(html, "<td class=\"sensordata\">Sunrise and Sunset:");
   fprintf(html, "<span class=\"sensorvalue\"> %d:%02d - %d:%02d</span></td>\n",
           (int) spa.sunrise, (int) sunrisemin, (int) spa.sunset, (int) sunsetmin);
   fprintf(html, "<td class=\"sensorspace\"></td>\n");

   fprintf(html, "<td class=\"sensordata\">Solar Zenith:");
   fprintf(html, "<span class=\"sensorvalue\">%.6f</span></td>\n", spa.zenith);
   fprintf(html, "<td class=\"sensorspace\"></td>\n");

   fprintf(html, "<td class=\"sensordata\">Solar Azimuth:");
   fprintf(html, "<span class=\"sensorvalue\">%.6f</span></td>\n", spa.azimuth);
   fprintf(html, "</tr></table>\n");

   if(verbose == 1) printf("Debug: Finished writing to file [%s]\n", file);
   fclose(html);
}

int main (int argc, char *argv[]) {

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

   if(verbose == 1) {
      printf("Origin UTCtimestamp: %ld\n", (long) calc_t);
      printf("Local calctimestamp: %ld\n", (long) calc_ttz);
      printf("Local timezone date: %s", asctime(&calc_tm));
   }

   int result;

   //enter required input values into SPA structure

   spa.year          = year;
   spa.month         = mon;
   spa.day           = day;
   spa.hour          = calc_tm.tm_hour;
   spa.minute        = calc_tm.tm_min;
   spa.second        = (double) calc_tm.tm_sec;
   spa.timezone      = (tzoffset / 3600);  // - for trailing GMT, e.g. US, + for ahead of GMT, e.g. Japan
   spa.delta_ut1     = 0;
   spa.delta_t       = 67;
   spa.longitude     = longitude;
   spa.latitude      = latitude;
   spa.elevation     = 1830.14;
   spa.pressure      = 1005;
   spa.temperature   = 16;
   spa.slope         = 0;   // observer slope, if the slope angle is set, the incidence angle is different from azimuth angle
   spa.azm_rotation  = -10; // observer offset from south
   spa.atmos_refract = 0.5667;
   spa.function      = SPA_ALL;

   if (verbose == 1) {
      printf("Calculation data input values:\n");
      printf("------------------------------\n");
      printf("Year:        %d\n", spa.year);
      printf("Month:       %d\n", spa.month);
      printf("Day:         %d\n", spa.day);
      printf("Hour:        %d\n", spa.hour);
      printf("Minute:      %d\n", spa.minute);
      printf("Second:      %.6f\n", spa.second);
      printf("TimeZone:    %.6f\n", spa.timezone);
      printf("Delta UTL:   %.6f\n", spa.delta_ut1);
      printf("Delat T:     %.6f\n", spa.delta_t);
      printf("Longitude:   %.6f\n", spa.longitude);
      printf("Latitude:    %.6f\n", spa.latitude);
      printf("Elevation:   %.6f\n", spa.elevation);
      printf("Pressure:    %.6f\n", spa.pressure);
      printf("Temperature: %.6f\n", spa.temperature);
      printf("Slope:       %.6f\n", spa.slope);
      printf("Rotation:    %.6f\n", spa.azm_rotation);
      printf("Refraction:  %.6f\n", spa.atmos_refract);
      printf("Function:    %d\n", spa.function);
   }

   //call the SPA calculate function and pass the SPA structure
   result = spa_calculate(&spa);
   sunrisemin = 60.0*(spa.sunrise - (int)(spa.sunrise));
   sunrisesec = (int) 60.0*(sunrisemin - (int)sunrisemin);
   sunsetmin = 60.0*(spa.sunset - (int)(spa.sunset));
   sunsetsec = (int) 60.0*(sunsetmin - (int)sunsetmin);

   if (verbose == 1) { //check for SPA errors
      if (result == 0) { //check for SPA errors
        printf("Intermediate output values:\n");
        printf("---------------------------\n");
        printf("Julian Day:    %.6f\n",spa.jd);
        printf("L longitude:   %.6e degrees\n",spa.l);
        printf("B latitude:    %.6e degrees\n",spa.b);
        printf("R radius:      %.6f AU\n",spa.r);
        printf("H hr angle:    %.6f degrees\n",spa.h);
        printf("Delta Psi:     %.6e degrees\n",spa.del_psi);
        printf("Delta Epsilon: %.6e degrees\n",spa.del_epsilon);
        printf("Epsilon:       %.6f degrees\n",spa.epsilon);
        printf("\nFinal output values:\n");
        printf("--------------------\n");
        printf("Zenith:        %.6f degrees\n",spa.zenith);
        printf("Azimuth:       %.6f degrees\n",spa.azimuth);
        printf("Incidence:     %.6f degrees\n",spa.incidence);
        printf("Sunrise:       %02d:%02d:%02d Local Time\n", (int)(spa.sunrise), (int) sunrisemin, sunrisesec);
        printf("Sunset:        %02d:%02d:%02d Local Time\n", (int)(spa.sunset), (int) sunsetmin, sunsetsec);
      }
      else printf("SPA Error Code: %d\n", result);
   }

   // RRD format: 1531641966:116.604309:78.838956
   printf("%ld:%.6f:%.6f\n",(long) calc_t, spa.zenith, spa.azimuth);

   /* -------------------------------------------------------- *
    * with arg -f, write the html table data to file           *
    * -------------------------------------------------------- */
   if(outflag == 1) write_html(htmfile);

   return 0;
}
