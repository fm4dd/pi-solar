<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link rel="stylesheet" type="text/css" href="style.css" media="screen" />
<link rel="stylesheet" type="text/css" href="solar.css" media="screen" />
<meta name="Title" content="Raspberry Pi Solar Power" />
<meta name="Description" content="Raspberry Pi Solar System" />
<meta name="Keywords" content="Raspberry Pi, Solar, Photovoltaik, Victron" />
<meta name="Classification" content="SOlar Power" />
<style type="text/css">
</style>
<title>Raspberry Pi Solar Power</title>
<script>
  function elementHideShow(element) {
    var el = document.getElementById(element);
    if (el.style.display == "block") { el.style.display = "none"; }
    else { el.style.display = "block"; }
  }
</script>
</head>

<body>
<div id="wrapper">
<div id="banner">
<h1>Pi-Weather Station <?php echo gethostname() ?></h1>
<h2>Solar Power Grid Monitoring</h2>
</div>

<?php include("./vmenu.htm"); ?>

<div id="content">
<?php if(file_exists("./getsolar.htm")) {      // check if file exists
    $tstamp = filemtime("./getsolar.htm");     // get file modification time
    $dt = new DateTime();                      // create new Date object
    $dt->setTimeStamp($tstamp);                // set Date object to tstamp
    $output = $dt->format('l F j Y,  H:i:s');  // format output string
  }
  echo "<h3>Last ve.direct update: $output</h3>\n";
  echo "<hr />\n";
?>

<img class="solarimg" src="images/mppt-75-10.jpg" alt="Victron BlueSolar MPPT 75-10">
<?php include("./getsolar.htm"); ?>

<div class="fullgraph"><img src="images/daily_vbat.png" alt="Battery Voltage"></div>
<div class="fullgraph"><img src="images/daily_vpnl.png" alt="Panel Voltage"></div>
<div class="fullgraph"><img src="images/daily_pbal.png" alt="Load Current"></div>
<div class="copyright"><a href="javascript:elementHideShow('s_term');">Expand or Hide Shortterm Details</a></div>
<h3>Shortterm View:</h3>
<hr />
<div class="showext" id="s_term" style="display: none;">
<?php include("./daypower.htm"); ?>
<p>
<div class="fullgraph"> <img src="images/monthly_vbat.png" alt="Weekly Battery Voltage"> </div>
<div class="fullgraph"> <img src="images/monthly_vpnl.png" alt="Weekly Panel Voltage"> </div>
<div class="fullgraph"> <img src="images/monthly_pbal.png" alt="Weekly Power Balance"> </div>
</div>
<div class="copyright"><a href="javascript:elementHideShow('m_term');">Expand or Hide Midterm Details</a></div>
<h3>Midterm View:</h3>
<hr />
<div class="showext" id="m_term" style="display: none;">
<?php include("./monpower.htm"); ?>
<div class="fullgraph"> <img src="images/yearly_vbat.png" alt="Yearly Battery Voltage"> </div>
<div class="fullgraph"> <img src="images/yearly_vpnl.png" alt="Yearly Panel Voltage"> </div>
<div class="fullgraph"> <img src="images/yearly_pbal.png" alt="Yearly Power Balance"> </div>
</div>

</div>

<div id="sidecontent">
  <h4>Station Health</h4>

<?php
// get system uptime
function Uptime() {
  $str   = @file_get_contents('/proc/uptime');
  $num   = floatval($str);
  $secs  = $num % 60;
  $num   = (int)($num / 60);
  $mins  = $num % 60;
  $num   = (int)($num / 60);
  $hours = $num % 24;
  $num   = (int)($num / 24);
  $days  = $num;
  $utstr = $days."d:".$hours."h:".$mins."m:".$secs."s";
  return $utstr;
}
$ut = Uptime();

// get ip address
$addr = $_SERVER['SERVER_ADDR'];

// get system cpu load
$cpu = exec('top -bn 1 | awk \'NR>7{s+=$9} END {print s/4"%"}\'');

// get system ram usage
$mem = exec('free | grep Mem | awk \'{printf("%.0fM of %.0fM\n", $3/1024, $2/1024)}\'');

// get system free disk space
$dfree = exec('df -h | grep \'/dev/root\' | awk {\'print $3 " of " $2\'}');

// get CPU temperature
$cputemp = exec('cat /sys/class/thermal/thermal_zone0/temp |  awk \'{printf("%.2f°C\n", $1/1000)}\'');

// write station health table to sidebar
echo "<table class\"station\">\n";
echo "<tr><th>Station Uptime:</th></tr>\n";
echo "<tr><td>".$ut."</td></tr>\n";
echo "<tr><th>IP Address:</th></tr>\n";
echo "<tr><td>".$addr."</td></tr>\n";
echo "<tr><th>CPU Usage:</th></tr>\n";
echo "<tr><td>".$cpu."</td></tr>\n";
echo "<tr><th>RAM Usage:</th></tr>\n";
echo "<tr><td>".$mem."</td></tr>\n";
echo "<tr><th>Disk Usage:</th></tr>\n";
echo "<tr><td>".$dfree."</td></tr>\n";
echo "<tr><th>CPU Temperature:</th></tr>\n";
echo "<tr><td>".$cputemp."</td></tr>\n";
echo "</table>\n";

// get pi-solar device data, and create info table
ini_set("auto_detect_line_endings", true);
$conf = array();
$fh=fopen("/home/pi/pi-solar/etc/pi-solar.conf", "r");

if($fh) {
  while ($line=fgets($fh, 80)) {
    if ((! preg_match('/^#/', $line)) &&    // show only lines w/o #
       (! preg_match('/^$/', $line))) {     // and who are not empty
      $line_a=explode("=", $line);          // explode at the '=' sign
      $line_a[1]=str_replace('"', '', $line_a[1]); // remove ""
      $conf[$line_a[0]]=$line_a[1];         // assign key/values
    }
  }
  echo "<h4>PV System</h4>\n";
  echo "<table class\"station\">\n";
  echo "<tr><th>Charge Controller:</th></tr>\n";
  echo "<tr><td>".$conf["pi-solar-charger"]."</td></tr>\n";
  echo "<tr><th>Controller Rating:</th></tr>\n";
  echo "<tr><td>".$conf["pi-solar-chrate"]."</td></tr>\n";
  echo "<tr><th>PV Panel:</th></tr>\n";
  echo "<tr><td>".$conf["pi-solar-pvtype"]."</td></tr>\n";
  echo "<tr><th>Panel Rating:</th></tr>\n";
  echo "<tr><td>".$conf["pi-solar-pvrate"]."</td></tr>\n";
  echo "<tr><th>Battery Type:</th></tr>\n";
  echo "<tr><td>".$conf["pi-solar-battype"]."</td></tr>\n";
  echo "<tr><th>Battery Rating:</th></tr>\n";
  echo "<tr><td>".$conf["pi-solar-batrate"]."</td></tr>\n";
  echo "</table>\n";
}
?>
  <h4>Solar System Images</h4>
  <a href="https://github.com/fm4dd/pi-solar"><img src="images/solar-system-01.jpg" height="160px" width="120px"></a>
</div>

<div id="footer">
  <span class="left">&copy; 2018, FM4DD.com</span>
  <span class="right">Raspberry Pi - running Raspbian</span>
</div>

</div>
</body>
</html>
