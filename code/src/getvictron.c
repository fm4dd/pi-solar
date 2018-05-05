/* ------------------------------------------------------------ *
 * file:        getvictron.c                                    *
 * purpose:     Read ve.direct serial line output from Victron  *
 *              BlueSolar charge controllers, convert it into   *
 *              RRD update string and write into a html file    *
 *              that should be included in a Raspi PHP web page *
 *              e.g. using <? php include("./getsolar.htm"); ?> *
 *                                                              *
 * returncode:	-1 on errors, charge state code (CS) on success *
 *                                                              *
 * reference:	Victron Energy VE.Direct Protocol Specification *
 *              victron-ve-direct-protocol.pdf Text-mode output *
 *                                                              *
 * author:      03/30/2018 Frank4DD http://github.com/fm4dd     *
 *                                                              *
 * compile:	gcc serial.c getvictron.c -o getvictron         *
 * ------------------------------------------------------------ */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

/* ------------------------------------------------------------ *
 * Global variables and defaults                                *
 * ------------------------------------------------------------ */
int verbose = 0;                   // set when arg -v is given
int outflag = 0;                   // set when arg -o is given
int retcode = 0;                   // return code of getvictron
char device[255] = "/dev/ttyAMA0"; // cmdline arg -s overrides it
char htmfile[255];                 // html output file and path
char serbuf[512];                  // serial line data buffer
char blockbuf[256];                // one block of ve.direct data
extern char *optarg;
extern int optind, opterr, optopt;

/* ------------------------------------------------------------ *
 * The fields struct describes a single line of serial data.    *
 * Longest code string defined so far is "Checksum"             *
 * ------------------------------------------------------------ */
struct fields { char code[9]; char lbl[255]; char val[255]; float base; };

/* ------------------------------------------------------------ *
 * The bsolar[] list defines the known codes for Victron MPPT   *
 * BlueSolar and SmartSolar charge controllers. It decodes the  *
 * serial output into readable format, and we load the received *
 * data block values into the last field "val".                 *
 * ------------------------------------------------------------ */
struct fields bsolar[] = {
   {"V",       "Battery Voltage"},         //  0
   {"VPV",     "Panel Voltage"},           //  1
   {"PPV",     "Panel Power"},             //  2
   {"I",       "Battery Current"},         //  3
   {"IL",      "Load Current"},            //  4
   {"LOAD",    "Load Output State"},       //  5
   {"Relay",   "Relay State"},             //  6
   {"H19",     "Yield Total"},             //  7
   {"H20",     "Yield Today"},             //  8
   {"H21",     "Maximum Power Today"},     //  9
   {"H22",     "Yield Yesterday"},         // 10
   {"H23",     "Maximum Power Yesterday"}, // 11
   {"ERR",     "Error Code"},              // 12
   {"CS",      "Operational State"},       // 13
   {"FW",      "Firmware Version"},        // 14
   {"PID",     "Type"},                    // 15
   {"SER#",    "Serial"},                  // 16
   {"HSDS",    "Day Sequence Number"},     // 17
   {"Checksum","Checksum"}                 // 18
};

/* ------------------------------------------------------------ *
 * external function prototypes for sensor-type specific code   *
 * ------------------------------------------------------------ */
int config_serial(int fd, int speed, int parity);
int get_serial(char *device, char *serbuf, int verbose);

/* ------------------------------------------------------------ *
 * strstr_last() returns pointer for str2's last occurence      *
 * ------------------------------------------------------------ */
char* strstr_last(const char* str1, const char* str2) {
   char* strp;
   int len1, len2;

   len2 = strlen(str2);
   if(len2 == 0) return (char*)str1;

   len1 = strlen(str1);
   if(len1 - len2 <= 0) return 0;

   strp = (char*)(str1 + len1 - len2);
   while(strp != str1) {
      if(*strp == *str2) {
         if(strncmp(strp,str2,len2) == 0)
            return strp;
      }
      strp--;
   }
   return 0;
}

/* ------------------------------------------------------------ *
 * add_values() loads the received values into the bsolar list. *
 * ------------------------------------------------------------ */
void add_values(char *key, char *value) {
   /* --------------------------------------------------------- *
    * Define list item pointer for begin of the list            *
    * --------------------------------------------------------- */
   struct fields *ptr = bsolar;
   /* --------------------------------------------------------- *
    * Define list item pointer for the end of the list          *
    * --------------------------------------------------------- */
   struct fields *endPtr = bsolar + sizeof(bsolar)/sizeof(bsolar[0]);
   /* --------------------------------------------------------- *
    * Check if the key exists, and set the corresponding value. *
    * --------------------------------------------------------- */
   int found = 0;
   while(ptr < endPtr){
      if(strcmp(key, ptr->code) == 0) {
         sprintf(ptr->val, "%s", value);
         found = 1;
      }
      ptr++;
   }
   if(found == 0) printf ("Error: could not find key [%s]\n", key);
}

/* ------------------------------------------------------------ *
 * parse_block() identifies key/value pairs in the data block   *
 * ------------------------------------------------------------ */
void parse_block(char *block) {
   /* --------------------------------------------------------- *
    * keyflag indicates if we got a key (0), or its value (1)   *
    * --------------------------------------------------------- */
   int keyflag = 0;
   size_t linepos = 0;
   char key[255];
   char value[255];

   /* --------------------------------------------------------- *
    * Loop over the block and parse data into keys and values.  *
    * --------------------------------------------------------- */
   size_t bufpos;
   for(bufpos=0; bufpos<=strlen(block); bufpos++) {
      /* ------------------------------------------------------ *
       * Check for TAB character 0x09                           * 
       * ------------------------------------------------------ */
      if(block[bufpos] == 0x09) {
         key[linepos] = '\0';
         keyflag = 1;
         linepos = 0;
         continue;
      }
      /* ------------------------------------------------------ *
       * Check for CARRIAGE RETURN character 0x0d               * 
       * ------------------------------------------------------ */
      if(block[bufpos] == 0x0d) {
         value[linepos] = '\0';
         if(verbose == 1) printf("key [%s] value [%s]\n", key, value);
         keyflag = 0;
         linepos = 0;
         // add value at key position into struct
         add_values(key, value);
         continue;
      }
      /* ------------------------------------------------------ *
       * Check for NEWLINE character 0x0a                       * 
       * ------------------------------------------------------ */
      if(block[bufpos] == 0x0a) continue;
      /* ------------------------------------------------------ *
       * Check for string termination character 0x0             * 
       * ------------------------------------------------------ */
      if(block[bufpos] == '\0') {
         value[linepos] = '\0';
         if(verbose == 1) printf("key [%s] value [%s]\n", key, value);
         // add value at key position into struct
         add_values(key, value);
         break;
      }
      /* ------------------------------------------------------ *
       * Generate the key                                       * 
       * ------------------------------------------------------ */
      if(keyflag == 0) { key[linepos] = block[bufpos]; linepos++; }
      /* ------------------------------------------------------ *
       * Generate the value                                     * 
       * ------------------------------------------------------ */
      if(keyflag == 1) { value[linepos] = block[bufpos]; linepos++; }
   }
}

/* ------------------------------------------------------------ *
 * convert_units() convert values into SI base units (V, A, Wh) *
 * and writes it as float type into the base field of the list. *
 * ------------------------------------------------------------ */
void convert_units(struct fields *list) {
   struct fields *ptr;
   /* --------------------------------------------------------- *
    * Convert Battery Voltage (V) from mV to V                  *
    * --------------------------------------------------------- */
   ptr = list+0; // Battery Voltage list location
   ptr->base = atol(ptr->val);
   ptr->base = ptr->base / 1000;

   /* --------------------------------------------------------- *
    * Convert Panel Voltage (VPV) from mV to V                  *
    * --------------------------------------------------------- */
   ptr = list+1; // Panel Voltage list location
   ptr->base = atol(ptr->val);
   ptr->base = ptr->base / 1000;

   /* --------------------------------------------------------- *
    * Convert Panel Power (PPV) string into float               *
    * --------------------------------------------------------- */
   ptr = list+2; // Panel Voltage list location
   ptr->base = atol(ptr->val);

   /* --------------------------------------------------------- *
    * Convert Battery Current (I) from mA to A                  *
    * --------------------------------------------------------- */
   ptr = list+3; // Battery Current list location
   ptr->base = atol(ptr->val);
   ptr->base = ptr->base / 1000;

   /* --------------------------------------------------------- *
    * Convert Load Current (IL) from mA to A                    *
    * --------------------------------------------------------- */
   ptr = list+4; // Load Current list location
   ptr->base = atol(ptr->val);
   ptr->base = ptr->base / 1000;

   /* --------------------------------------------------------- *
    * Convert Yield Total (H19) from kWh to Wh                  *
    * --------------------------------------------------------- */
   ptr = list+7; // Yield Total list location
   ptr->base = atol(ptr->val);
   ptr->base = ptr->base * 1000;

   /* --------------------------------------------------------- *
    * Convert Yield Today (H20) from kWh to Wh                  *
    * --------------------------------------------------------- */
   ptr = list+8; // Yield Today list location
   ptr->base = atol(ptr->val);
   ptr->base = ptr->base * 1000;

   /* --------------------------------------------------------- *
    * Convert Yield Yesterday (H20) from kWh to Wh              *
    * --------------------------------------------------------- */
   ptr = list+10; // Yield Yesterday list location
   ptr->base = atol(ptr->val);
   ptr->base = ptr->base * 1000;

   /* --------------------------------------------------------- *
    * Convert Operational State code into Description String    * 
    * --------------------------------------------------------- */
   ptr = list+13; // State of Operation CS
   ptr->base = atol(ptr->val);
   retcode = atoi(ptr->val);
   switch(retcode) {
      case 0: strncpy(ptr->val, "Off", 255); break;
      case 2: strncpy(ptr->val, "Fault", 255); break;
      case 3: strncpy(ptr->val, "Bulk", 255); break;
      case 4: strncpy(ptr->val, "Absorption", 255); break;
      case 5: strncpy(ptr->val, "Float", 255); break;
   }

   /* --------------------------------------------------------- *
    * Convert Firmware Version (FW) from 130 to 1.30            *
    * --------------------------------------------------------- */
   ptr = list+14; // Firmware Version list location
   ptr->base = atol(ptr->val);
   ptr->base = ptr->base / 100;

   /* --------------------------------------------------------- *
    * Convert Product ID into the full Product Name String      *
    * --------------------------------------------------------- */
   ptr = list+15; // Product ID PID
   ptr->base = strtol(ptr->val, NULL, 0);
   switch(strtol(ptr->val, NULL, 0)) {
      case 0x0300: strncpy(ptr->val, "BlueSolar MPPT 70/15", 255);break;
      case 0xa040: strncpy(ptr->val, "BlueSolar MPPT 75/50", 255);break;
      case 0xa041: strncpy(ptr->val, "BlueSolar MPPT 150/35", 255);break;
      case 0xa042: strncpy(ptr->val, "BlueSolar MPPT 75/15", 255);break;
      case 0xa043: strncpy(ptr->val, "BlueSolar MPPT 100/15", 255);break;
      case 0xa044: strncpy(ptr->val, "BlueSolar MPPT 100/30", 255);break;
      case 0xa045: strncpy(ptr->val, "BlueSolar MPPT 100/50", 255);break;
      case 0xa046: strncpy(ptr->val, "BlueSolar MPPT 150/70", 255);break;
      case 0xa047: strncpy(ptr->val, "BlueSolar MPPT 150/100", 255);break;
      case 0xa048: strncpy(ptr->val, "BlueSolar MPPT 75/50 rev2", 255);break;
      case 0xa049: strncpy(ptr->val, "BlueSolar MPPT 100/50 rev2", 255);break;
      case 0xa04a: strncpy(ptr->val, "BlueSolar MPPT 100/30 rev2", 255);break;
      case 0xa04b: strncpy(ptr->val, "BlueSolar MPPT 100/35 rev2", 255);break;
      case 0xa04c: strncpy(ptr->val, "BlueSolar MPPT 75/10", 255);break;
      case 0xa04d: strncpy(ptr->val, "BlueSolar MPPT 150/45", 255);break;
      case 0xa04e: strncpy(ptr->val, "BlueSolar MPPT 150/60", 255);break;
      case 0xa04f: strncpy(ptr->val, "BlueSolar MPPT 150/85", 255);break;
      case 0xa050: strncpy(ptr->val, "SmartSolar MPPT 250/100", 255);break;
      case 0xa051: strncpy(ptr->val, "SmartSolar MPPT 150/100", 255);break;
      case 0xa052: strncpy(ptr->val, "SmartSolar MPPT 150/85", 255);break;
      case 0xa053: strncpy(ptr->val, "SmartSolar MPPT 75/15", 255);break;
      case 0xa054: strncpy(ptr->val, "SmartSolar MPPT 75/10", 255);break;
      case 0xa055: strncpy(ptr->val, "SmartSolar MPPT 100/15", 255);break;
      case 0xa056: strncpy(ptr->val, "SmartSolar MPPT 100/30", 255);break;
      case 0xa057: strncpy(ptr->val, "SmartSolar MPPT 100/50", 255);break;
      case 0xa058: strncpy(ptr->val, "SmartSolar MPPT 150/35", 255);break;
      case 0xa059: strncpy(ptr->val, "SmartSolar MPPT 150/100 rev2", 255);break;
      case 0xa05a: strncpy(ptr->val, "SmartSolar MPPT 150/85 rev2", 255);break;
      case 0xa05b: strncpy(ptr->val, "SmartSolar MPPT 250/70 rev2", 255);break;
      case 0xa05c: strncpy(ptr->val, "SmartSolar MPPT 250/85", 255);break;
      case 0xa05d: strncpy(ptr->val, "SmartSolar MPPT 250/60", 255);break;
      case 0xa05e: strncpy(ptr->val, "SmartSolar MPPT 250/45", 255);break;
      case 0xa05f: strncpy(ptr->val, "SmartSolar MPPT 100/20", 255);break;
      default: strncpy(ptr->val, "*UNKNOWN*", 255);break;
    }

   if(verbose == 1) printf("Debug: Unit conversion into SI base complete.\n");
}

/* ------------------------------------------------------------ *
 * create_rrdstr() constructs the RRD database update string.   *
 * The string must match the RRD database schema defined in     *
 * ../install/rrdcreate.sh. String format is:  N:value[:value]  *
 * (see man rrdupdate). If there is no value output set to "U". *
 * ------------------------------------------------------------ */
void create_rrdstr(struct fields *list, char *str) {
   struct fields *ptr;
   /* --------------------------------------------------------- *
    * get the string components                                 *
    * --------------------------------------------------------- */
   time_t tsnow = time(NULL);
   ptr = list+0; // Battery Voltage list location
   float vbat = ptr->base;
   ptr = list+3; // Battery Current list location
   float cbat = ptr->base;
   ptr = list+1; // Panel Voltage list location
   float vpan = ptr->base;
   ptr = list+2; // Panel Power list location
   float ppan = ptr->base;
   ptr = list+4; // Load Current list location
   float cload = ptr->base;
   ptr = list+13; // Charge State Current list location
   float opcs = ptr->base;
   /* --------------------------------------------------------- *
    * Combine, format and write the string per RRD schema order *
    *                                                           *
    * pi-solar DB schema: timestamp:V:I:VPV:PPV:IL:CS:dayt-flag *
    * e.g. 1522807566:12.3000:0.0210:15.0130:0.1450:0.2750:0:1  *
    *                                                           *
    * The daytime flag is externally calculated and left out.   *
    * Its added by the script solar-data.sh                     *
    * --------------------------------------------------------- */
   snprintf(str, 255, "%lld:%.4f:%.4f:%.4f:%4f:%.4f:%.4f",
            (long long) tsnow, vbat, cbat, vpan, ppan, cload, opcs);

   if(verbose == 1) printf("Debug: RRD update string creation complete.\n");
}

/* ------------------------------------------------------------ *
 * print_usage() prints the programs commandline instructions.  *
 * ------------------------------------------------------------ */
void usage() {
   static char const usage[] = "Usage: getvictron -s [serial-tty] -o [html-output] [-v]\n\
\n\
Command line parameters have the following format:\n\
   -s   serial line device, Examples: /dev/ttyS1, /dev/ttyAMA0\n\
   -o   optional, write sensor data to HTML file, Example: -o ./getsolar.htm\n\
   -h   optional, display this message\n\
   -v   optional, enables debug output\n\
\n\
Usage examples:\n\
./getvictron -s /dev/ttyAMA0 -o ./getsolar.htm -v\n\
./getvictron -s /dev/ttyS1 -o ./getsolar.htm -v\n";
   printf(usage);
}

/* ------------------------------------------------------------ *
 * parseargs() checks the commandline arguments with C getopt   *
 * ------------------------------------------------------------ */
void parseargs(int argc, char* argv[]) {
   int arg;
   opterr = 0;

   if(argc == 1) { usage(); exit(-1); }

   while ((arg = (int) getopt (argc, argv, "s:o:vh")) != -1) {
      switch (arg) {
         // arg -s + serial device, type: string
         // mandatory, example: /dev/ttyAMA0
         case 's':
            strncpy(device, optarg, sizeof(device));
            break;

         // arg -o + dst HTML file, type: string
         // optional, example: /tmp/getsolar.htm
         case 'o':
            outflag = 1;
            strncpy(htmfile, optarg, sizeof(htmfile));
            break;

         // arg -v verbose, type: flag, optional
         case 'v':
            verbose = 1; break;

         // arg -h usage, type: flag, optional
         case 'h':
            usage(); exit(0);

         case '?':
            if(isprint (optopt))
               printf ("Error: Unknown option `-%c'.\n", optopt);
            else
               printf ("Error: Unknown option character `\\x%x'.\n", optopt);
            usage();
            exit(-1);

         default:
            usage();
      }
   }
   if (strlen(device) < 8) {
      printf("Error: Cannot get valid -s serial device argument.\n");
      exit(-1);
   }
}

void write_html(char *file, struct fields *list){
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
    *  Write the ve.direct data output table                   *
    * -------------------------------------------------------- */
   struct fields *ptr;
   fprintf(html, "<table class=\"solartable\">\n");
   ptr = list+15; // Product ID
   fprintf(html, "<tr><th class=\"solarth\" rowspan=4>Charge Controller</th>");
   fprintf(html, "<td class=\"solartd\"><div class=\"solarlbl\">%s</div>", ptr->lbl);
   fprintf(html, "<div class=\"solarval\">%s</div></td></tr>\n", ptr->val);
   ptr = list+16; // Serial Number
   fprintf(html, "<tr><td class=\"solartd\"><div class=\"solarlbl\">%s</div>", ptr->lbl);
   fprintf(html, "<div class=\"solarval\">%s</div></td></tr>\n", ptr->val);
   ptr = list+14; // Firmware Version
   fprintf(html, "<tr><td class=\"solartd\"><div class=\"solarlbl\">%s</div>", ptr->lbl);
   fprintf(html, "<div class=\"solarval\">%.2f</div></td></tr>\n", ptr->base);
   ptr = list+13; // State of Operation CS
   fprintf(html, "<tr><td class=\"solartd\"><div class=\"solarlbl\">%s</div>", ptr->lbl);
   fprintf(html, "<div class=\"solarval\">%s</div></td></tr>\n", ptr->val);
   ptr = list+0;  // Battery Voltage
   fprintf(html, "<tr><th class=\"solarth\" rowspan=2>Battery</th>");
   fprintf(html, "<td class=\"solartd\"><div class=\"solarlbl\">%s</div>", ptr->lbl);
   fprintf(html, "<div class=\"solarval\">%.2f&thinsp;V</div></td></tr>\n", ptr->base);
   ptr = list+3;  // Battery Current
   fprintf(html, "<tr><td class=\"solartd\"><div class=\"solarlbl\">%s</div>", ptr->lbl);
   fprintf(html, "<div class=\"solarval\">%.2f&thinsp;A</div></td></tr>\n", ptr->base);
   ptr = list+1;  // Panel Voltage
   fprintf(html, "<tr><th class=\"solarth\" rowspan=2>PV Panel</th>");
   fprintf(html, "<td class=\"solartd\"><div class=\"solarlbl\">%s</div>", ptr->lbl);
   fprintf(html, "<div class=\"solarval\">%.2f&thinsp;V</div></td></tr>\n", ptr->base);
   ptr = list+2;  // Panel Power
   fprintf(html, "<tr><td class=\"solartd\"><div class=\"solarlbl\">%s</div>", ptr->lbl);
   fprintf(html, "<div class=\"solarval\">%.2f&thinsp;W</div></td></tr>\n", ptr->base);
   ptr = list+5;  // Load Output State
   fprintf(html, "<tr><th class=\"solarth\" rowspan=2>Load</th>");
   fprintf(html, "<td class=solartd><div class=\"solarlbl\">%s</div>", ptr->lbl);
   fprintf(html, "<div class=\"solarval\">%s</div></td></tr>\n", ptr->val);
   ptr = list+4;  // Load Current
   fprintf(html, "<tr><td class=\"solartd\"><div class=\"solarlbl\">%s</div>", ptr->lbl);
   fprintf(html, "<div class=\"solarval\">%.2f&thinsp;A</div></td></tr>\n", ptr->base);
   fprintf(html, "</table>\n");

   /* -------------------------------------------------------- *
    *  Write the power balance output table                    *
    * -------------------------------------------------------- */
   fprintf(html, "<hr />\n");
   fprintf(html, "<table><tr>\n");
   ptr = list+2;  // Panel Power
   fprintf(html, "<td class=\"sensordata\">Solar Power IN:");
   fprintf(html, "<span class=\"sensorvalue\">%.2f&thinsp;W</span></td>\n", ptr->base);
   fprintf(html, "<td class=\"sensorspace\"></td>\n");
   ptr = list+0;  // Panel Power
   float vbat = ptr->base;
   ptr = list+3;  // Panel Power
   float ibat = ptr->base;
   float pbat = vbat * ibat;
   fprintf(html, "<td class=\"sensordata\">Power Balance +/-:");
   fprintf(html, "<span class=\"sensorvalue\">%+.2f&thinsp;W</span></td>\n", pbat);
   fprintf(html, "<td class=\"sensorspace\"></td>\n");
   ptr = list+4;  // Load Current
   float pload = vbat * ptr->base;
   fprintf(html, "<td class=\"sensordata\">Load Power OUT:");
   fprintf(html, "<span class=\"sensorvalue\">%.2f&thinsp;W</span></td>\n", pload);
   fprintf(html, "</tr></table>\n");

   if(verbose == 1) printf("Debug: Finished writing to file [%s]\n", file);
   fclose(html);
}

int main(int argc, char *argv[]) {
   time_t tsnow = time(NULL);
   /* ----------------------------------------------------------- *
    * Process the cmdline parameters                              *
    * ----------------------------------------------------------- */
   parseargs(argc, argv);
   if(verbose == 1) printf("Debug: Started getvictron at date %s", ctime(&tsnow));
   if(verbose == 1) printf("Debug: arg -s, value [%s]\n", device);
   if(verbose == 1) printf("Debug: arg -o, value [%s]\n", htmfile);

   /* ----------------------------------------------------------- *
    * Get the serial data from Victrons ve.direct interface       *
    * ----------------------------------------------------------- */
   get_serial(device, serbuf, verbose);

   /* ------------------------------------------------------------ *
    * startstring is defined "PID" followed by a TAB               *
    * ------------------------------------------------------------ */
   char startstring[] = { 0x50, 0x49, 0x44, 0x09, 0x00 };
   char *startptr = strstr_last(serbuf, startstring);
   int startpos = 0;

   if(startptr == NULL) {
      printf("Error: could not find start marker [%s].\n", startstring);
      exit(-1);
   }
   
   if(startptr == serbuf) {
      if(verbose == 1) printf("Debug: Polling caught transmission start.\n");
   }

   /* ------------------------------------------------------------ *
    * endstring "CHECKSUM" followed by a TAB and the final CS byte *
    * ------------------------------------------------------------ */
   char endstring[] = { 0x43, 0x68, 0x65, 0x63, 0x6b, 0x73, 0x75, 0x6d,  0x09, 0x00 };
   char *endptr = strstr_last(serbuf, endstring);
   int endpos = endptr-serbuf;
   startpos = startptr-serbuf;

   if(endptr == NULL) {
      printf("Error: could not find end marker \"CHECKSUM\".\n");
      exit(-1);
   }
   if(endptr < startptr) {
      if(verbose == 1) printf("Debug: End position [%d] comes before start [%d].\n", endpos, startpos);
      serbuf[endpos+10] = '\0';
      startptr = strstr_last(serbuf, startstring);
      startpos = startptr-serbuf;
   }
   if(startptr == NULL) {
      printf("Error: could not find start marker [%s].\n", startstring);
      exit(-1);
   }
   if(verbose == 1) printf("Debug: Polling startptr [%d].\n", startpos);

   if(endptr+10 == '\0') {
      if(verbose == 1) printf("Debug: Polling finished at transmission end.\n");
   }
   
   if(verbose == 1) printf("Debug: Polling endptr [%d], string [%s].\n", endpos, &serbuf[endpos]);
   strncpy(blockbuf, &serbuf[startpos], sizeof(blockbuf));
   if(verbose == 1) printf("Debug: ve.direct block:\n%s\n", blockbuf);

   /* -------------------------------------------------------- *
    * Parse serial block data into the bsolar structured list  *
    * -------------------------------------------------------- */
   parse_block(blockbuf);

   /* -------------------------------------------------------- *
    * Converts received unit values into base SI units, e.g.   *
    * mV->Volt, mA->A, kWh -> Wh, with 0.4 digits precision.   *
    * -------------------------------------------------------- */
   convert_units(bsolar);

   /* -------------------------------------------------------- *
    * Create RRD database update string from serial block data *
    * -------------------------------------------------------- */
   char rrdstr[255];
   create_rrdstr(bsolar, rrdstr);
   if(verbose == 1) printf("Debug: RRD update string [%s]\n", rrdstr);
   printf("%s\n", rrdstr);

   /* -------------------------------------------------------- *
    * with arg -o, write the html table data to file           *
    * -------------------------------------------------------- */
   if(outflag == 1) write_html(htmfile, bsolar);

   exit(retcode);
}
