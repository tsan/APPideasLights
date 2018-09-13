#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>

/**
 * A sketch to control two strings of RGBW LED lights from one ESP8266
 * 
 * @author costmo
 * @since  20180902
 */

// Setup a wireless access point so the user can be prompted to connect to a network
String  ssidPrefix = "appideas-";
int serverPort = 5050;
String ssid = "";
String softIP  = "";
bool wifiConnected = false;

// Setup NTP parameters for syncing with network time
const int TIMEZONE_OFFSET = 3600; // Number of seconds in an hour
int inputTimezoneOffset = 0;
const char* ntpServerName = "time.nist.gov";
const int checkInterval = 60 * 30; // how often (in seconds) to query the NTP server for updates (30 minutes)
// NTP parameters that shouldn't be changed
unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

// For reference (pin mapping for ESP8266-12E):
// D0/GPIO16      = 16;
// D1/GPIO5       = 5;
// D2/GPIO4       = 0;
// D3/GPIO0       = 4;

// D4/GPIO2       = 2;
// D5/GPIO14      = 14;
// D6/GPIO12      = 13;
// D7/GPIO13      = 12;


// First string of lights. Pins D0 - D3
#define WHITE_LED_FIRST     16
#define BLUE_LED_FIRST      5
#define RED_LED_FIRST       0
#define GREEN_LED_FIRST     4

// Second string of lights. Pins D4 - D7
#define WHITE_LED_SECOND     2
#define BLUE_LED_SECOND      14
#define RED_LED_SECOND       13
#define GREEN_LED_SECOND     12

int maxBrightness = 1024;
int offBrightness = 0;

int status = WL_IDLE_STATUS;
IPAddress ip;

ESP8266WebServer server( serverPort );

// Status variables
int currentR = 0;
int currentG = 0;
int currentB = 0;
int currentW = 0;

int currentRFirst = 0;
int currentGFirst = 0;
int currentBFirst = 0;
int currentWFirst = 0;

int currentRSecond = 0;
int currentGSecond = 0;
int currentBSecond = 0;
int currentWSecond = 0;

String fullTime = String( "" );
String humanTime = String( "" );
int intTime = 0;
int currentHour = 0;
int currentMinute = 0;
int currentSecond = 0;
int currentMeridiem = 0;

/**
 * Run on device power-on
 * 
 * @author costmo
 * @since  20180902
 */
void setup() 
{
  Serial.begin( 57600 );
  delay( 100 );

  pinMode( WHITE_LED_FIRST, OUTPUT );
  pinMode( BLUE_LED_FIRST, OUTPUT );
  pinMode( RED_LED_FIRST, OUTPUT );
  pinMode( GREEN_LED_FIRST, OUTPUT );

  pinMode( WHITE_LED_SECOND, OUTPUT );
  pinMode( BLUE_LED_SECOND, OUTPUT );
  pinMode( RED_LED_SECOND, OUTPUT );
  pinMode( GREEN_LED_SECOND, OUTPUT );
  
  startSoftAP();
  startWebServer();

  // turn all the lights on when the ESP first powers on
  turnOn( "first" );
  turnOn( "second" );
}

/**
 * Turn all lights to max brightness
 * 
 * @return void
 * @author costmo
 * @since  20180902
 * @param  String   whichPosition     first|second
 */
void turnOn( String whichPosition ) 
{ 
  setColorToLevel( whichPosition, "all", maxBrightness );
}

/**
 * Turn all lights off
 * 
 * @return void
 * @author costmo
 * @since  20180902
 * @param  String   whichPosition     first|second
 */
void turnOff( String whichPosition ) 
{
  setColorToLevel( whichPosition, "all", offBrightness );
}

/**
 * Set a color at a position to a specific level between 0 and 1024
 * 
 * @return void
 * @author costmo
 * @since  20180902
 * @param  String   whichPosition     first|second
 * @param  String   color             red|green|blue|white|all
 * @param  intng    level             0 - 1024
 */
void setColorToLevel( String whichPosition, String color, int level ) 
{ 

  int redPin = RED_LED_FIRST;
  int greenPin = GREEN_LED_FIRST;
  int bluePin = BLUE_LED_FIRST;
  int whitePin = WHITE_LED_FIRST;

  if( whichPosition == "second" )
  {
    redPin = RED_LED_SECOND;
    greenPin = GREEN_LED_SECOND;
    bluePin = BLUE_LED_SECOND;
    whitePin = WHITE_LED_SECOND;
  }
  
  if( color == "red" )
  {
    analogWrite( redPin, level );
    currentR = level;
  }
  if( color == "green" )
  {
    analogWrite( greenPin, level );
    currentG = level;
  }
  if( color == "blue" )
  {
    analogWrite( bluePin, level );
    currentB = level;
  }
  if( color == "white" )
  {
    analogWrite( whitePin, level );
    currentW = level;
  }
  if( color == "all" )
  {
    analogWrite( redPin, level );
    analogWrite( greenPin, level );
    analogWrite( bluePin, level );
    analogWrite( whitePin, level );
    currentR = level;
    currentG = level;
    currentB = level;
    currentW = level;
  }

  if( whichPosition == "second" )
  {
    currentRSecond = currentR;
    currentGSecond = currentG;
    currentBSecond = currentB;
    currentWSecond = currentW;
  }
  else
  {
    currentRFirst = currentR;
    currentGFirst = currentG;
    currentBFirst = currentB;
    currentWFirst = currentW;
  }
}

/**
 * Run while the sketch is active
 * 
 * @return void
 * @author costmo
 * @since  20180902
 */
void loop() 
{
  //nodeServer();
  server.handleClient();
}

void handleRoot()
{
  //server.send( 200, "text/html", connectionHtml() );
  if( !wifiConnected )
  {
    Serial.println( "Connecting" );
    server.send( 200, "text/html", connectionHtml() );
  }
  else
  {
    Serial.println( "Already connected" );
    server.send( 200, "text/html", connectedHtml() );
  }
}

void handleConnect()
{
  String hardSSID = server.arg( "ssid" );
  String hardPassword = server.arg( "password" );
  int tmpInputTimezoneOffset = server.arg( "timezone" ).toInt();

  if( tmpInputTimezoneOffset != 0 )
  {
    inputTimezoneOffset = tmpInputTimezoneOffset;
  }

  if( !wifiConnected )
  {
    connectToWifi( hardSSID, hardPassword );
  }

  // Get and show the time
  Serial.println( "Starting UDP" );
  udp.begin( localPort );
  fullTime = getTime();
  timeToVars();
  Serial.println( "Current time: " + humanTime );
  
  server.send( 200, "text/html", connectedHtml() );
  Serial.println( "Connected to WiFi" );
}

void startSoftAP()
{
  String mac = WiFi.macAddress();
  String macPartOne = mac.substring( 12, 14 );
  String macPartTwo = mac.substring( 15 );
  ssid = ssidPrefix + macPartOne + macPartTwo;
  
  WiFi.softAP( ssid.c_str() );
  softIP = WiFi.softAPIP().toString();
 
  Serial.println( getSoftAPStatus() );
}

void startWebServer()
{
  server.on( "/", handleRoot );
  server.on( "/connect", handleConnect );
  server.begin();
}

/**
 * Connect to a wifi network and print the status
 * 
 * @return void
 * @author costmo
 * @since  20180902
 */
void connectToWifi( String wifiSSID, String wifiPassword )
{
  Serial.print( "Connecting to " );
  Serial.println( wifiSSID );

  WiFi.mode( WIFI_STA );
  WiFi.begin( wifiSSID.c_str(), wifiPassword.c_str() );

  while( WiFi.status() != WL_CONNECTED )
  {
      delay( 500 );
      Serial.print( "." );
  }
  Serial.println( "." );
  Serial.println( "Connected" );
  wifiConnected = true;
  printWifiStatus();
}

/**
 * Print the current IP address
 * 
 * @return void
 * @author costmo
 * @since  20180902
 */
void printWifiStatus() 
{
  // print your WiFi IP address:
  ip = WiFi.localIP();
  Serial.print( "IP Address: " );
  Serial.println( ip );
}

/**
 * Prints the HTML header for output
 * 
 * @return void
 * @author costmo
 * @since  20180902
 * @param  WifiClient   client    The wifi client
 */
void htmlHeader( WiFiClient client )
{
    client.println( "HTTP/1.1 200 OK" );
    client.println ("Content-Type: text/html" );
    client.println( "Connection: close" );  // the connection will be closed after completion of the response
    client.println( "Cache-Control: no-store" );
    client.println();
    client.println( "<!DOCTYPE HTML>" );
    client.println( "<html>" );
}

 /**
 * Prints the HTML footer for output
 * 
 * @return void
 * @author costmo
 * @since  20180902
 * @param  WifiClient   client    The wifi client
 */
void htmlFooter( WiFiClient client )
{
    client.println( "</html>" );
}
 
/**
 * Gets the current level (0 - 1024) for the given color and position
 * 
 * @return int
 * @author costmo
 * @since  20180902
 * @param  String   whichPosition     first|second
 * @param  String   color             red|green|blue|white|all
 */
int getLevelForColor( String whichPostiion, String color )
{
  
  if( color == "red" )
  {
    if( whichPostiion == "second" )
    {
      return currentRSecond;
    }
    else
    {
      return currentRFirst;
    } 
  }
  else if( color == "green" )
  {
    if( whichPostiion == "second" )
    {
      return currentGSecond;
    }
    else
    {
      return currentGFirst;
    }
  }
  else if( color == "blue" )
  {
    if( whichPostiion == "second" )
    {
      return currentBSecond;
    }
    else
    {
      return currentBFirst;
    }
  }
  else if( color == "white" )
  {
    if( whichPostiion == "second" )
    {
      return currentWSecond;
    }
    else
    {
      return currentWFirst;
    }
  }
  else
  {
    return 0;
  }
}

// 
/**
 * Gets the current ratio (0.0 - 1.0) for the given color and position
 * 
 * @return float
 * @author costmo
 * @since  20180902
 * @param  String   whichPosition     first|second
 * @param  String   color             red|green|blue|white|all
 */
float getRatioForColor( String whichPosition, String color )
{
  int  currentValue = getLevelForColor( whichPosition, color );
  return (float) ((float)currentValue / (float)1024);
}

/**
 * A tiny API server for receiving and handling requests
 * 
 * @return void
 * @author costmo
 * @since  20180902
 */
void nodeServer()
{
//   WiFiClient client = server.available();
//   if( client )
//   {
//      String requestString; 
//      boolean currentLineIsBlank = true;
//      while( client.connected() )
//      {
//        if( client.available() )
//        {
//          char c = client.read();
//          requestString += c;
//
//          // request is complete, parse it
//          if( c == '\n' && currentLineIsBlank )
//          {
//              htmlHeader( client );
//
//              String requestPosition = "first";
//              String requestColor = "all";
//              int requestLevel = 1024;
//
//              if( requestString.indexOf( "?action=id" ) > 0 )
//              {
//                byte mac[6];
//                WiFi.macAddress( mac );
//                // Using client.write multiple times caused scrolling output. Building a string causes instant output on completion, which is preferable
////                String output = nodeType;
////                output += "|";
//                String output = String( mac[0], HEX );
//                output +=  ":" ;
//                output += String( mac[1], HEX );
//                output +=  ":" ;
//                output += String( mac[2], HEX );
//                output +=  ":" ;
//                output += String( mac[3], HEX );
//                output +=  ":" ;
//                output += String( mac[4], HEX );
//                output +=  ":" ;
//                output += String( mac[5], HEX );
//                output += "|";
//                output += WiFi.localIP();
////                output += "|";
////                output += nodeVersion;
//                client.println( output );
//                return; // We don't want to go below this if the request was for "id"
//              }
//              
//              // parse the incoming request and do the work
//              // p = "position" = first|second
//              // c = "color" = red|green|blue|white|all
//              // l ="level" = 0|25|50|75|100
//              if( requestString.indexOf( "?p=second" ) > 0 || requestString.indexOf( "&p=second" ) > 0 )
//              {
//                requestPosition = "second";
//              }
//              if( requestString.indexOf( "?c=r" ) > 0 || requestString.indexOf( "&c=r" ) > 0 )
//              {
//                requestColor = "red";
//              }
//              if( requestString.indexOf( "?c=g" ) > 0 || requestString.indexOf( "&c=g" ) > 0 )
//              {
//                requestColor = "green";
//              }
//              if( requestString.indexOf( "?c=b" ) > 0 || requestString.indexOf( "&c=b" ) > 0 )
//              {
//                requestColor = "blue";
//              }
//              if( requestString.indexOf( "?c=w" ) > 0 || requestString.indexOf( "&c=w" ) > 0 )
//              {
//                requestColor = "white";
//              }
//              
//              if( requestString.indexOf( "?l=0" ) > 0 || requestString.indexOf( "&l=0" ) > 0 )
//              {
//                requestLevel = 0;
//              }
//              if( requestString.indexOf( "?l=25" ) > 0 || requestString.indexOf( "&l=25" ) > 0 )
//              {
//                requestLevel = 256;
//              }
//              if( requestString.indexOf( "?l=50" ) > 0 || requestString.indexOf( "&l=50" ) > 0 )
//              {
//                requestLevel = 512;
//              }
//              if( requestString.indexOf( "?l=75" ) > 0 || requestString.indexOf( "&l=75" ) > 0 )
//              {
//                requestLevel = 768;
//              }
//
//
//              if( requestColor == "all"  )
//              {
//                if( requestLevel == 100 )
//                {
//                  turnOn( requestPosition );
//                }
//                else if( requestLevel == 0 )
//                {
//                  turnOff( requestPosition );
//                }
//                else
//                {
//                  setColorToLevel( requestPosition, "red", requestLevel );
//                  setColorToLevel( requestPosition, "green", requestLevel );
//                  setColorToLevel( requestPosition, "blue", requestLevel );
//                  setColorToLevel( requestPosition, "white", requestLevel );
//                }
//              }
//              else
//              {
//                setColorToLevel( requestPosition, requestColor, requestLevel );
//              }
//
//              client.println( getValues() );
//              
//              htmlFooter( client );
//              break;
//          } // if( c == '\n' && currentLineIsBlank )
//          if( c == '\n' ) 
//          {
//            currentLineIsBlank = true;
//          } 
//          else if( c != '\r' ) 
//          {
//            // you've gotten a character on the current line
//            currentLineIsBlank = false;
//          }
//          
//        } // if( client.available() )
//      } // while( client.connected() )
//
//    // give the web browser time to receive the data
//    delay( 1 );
//
//    // close the connection:
//    client.stop();
//   } // if( client )
} // void nodeServer()

/**
 * Gets an HTML string with the values for all lights and positions
 * 
 * @return String
 * @author costmo
 * @since  20180902
 */
String getValues()
{
  String output = "<p>First Red value: ";
                output += getLevelForColor( "first", "red" );
                output += "<br>";
                output += "First Red ratio: ";
                output += getRatioForColor( "first", "red" );
                output += "&nbsp;</p>";

                output += "<p>First Green value: ";
                output += getLevelForColor( "first", "green" );
                output += "<br>";
                output += "First Green ratio: ";
                output += getRatioForColor( "first", "green" );
                output += "&nbsp;</p>";

                output += "<p>First Blue value: ";
                output += getLevelForColor( "first", "blue" );
                output += "<br>";
                output += "First Blue ratio: ";
                output += getRatioForColor( "first", "blue" );
                output += "&nbsp;</p>";

                output += "<p>First White value: ";
                output += getLevelForColor( "first", "white" );
                output += "<br>";
                output += "First White ratio: ";
                output += getRatioForColor( "first", "white" );
                output += "&nbsp;</p>";

                output += "<p>&nbsp;</p><p>Second Red value: ";
                output += getLevelForColor( "second", "red" );
                output += "<br>";
                output += "Second Red ratio: ";
                output += getRatioForColor( "second", "red" );
                output += "&nbsp;</p>";

                output += "<p>Second Green value: ";
                output += getLevelForColor( "second", "green" );
                output += "<br>";
                output += "Second Green ratio: ";
                output += getRatioForColor( "second", "green" );
                output += "&nbsp;</p>";

                output += "<p>Secondirst Blue value: ";
                output += getLevelForColor( "second", "blue" );
                output += "<br>";
                output += "Second Blue ratio: ";
                output += getRatioForColor( "second", "blue" );
                output += "&nbsp;</p>";

                output += "<p>Second White value: ";
                output += getLevelForColor( "second", "white" );
                output += "<br>";
                output += "Second White ratio: ";
                output += getRatioForColor( "second", "white" );
                output += "&nbsp;</p>";

  return output;
}

/**
 * Get the current time from an NTP server
 * 
 * @return String
 * @author costmo
 * @since  20180902
 */
String getTime()
{
  
  //Serial.println( "Getting time" );
  String returnValue = String();

  //get a random server from the pool
  WiFi.hostByName( ntpServerName, timeServerIP );

  sendNTPpacket( timeServerIP ); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay( 2000 );
  
  int cb = udp.parsePacket();
  // loop until we have a good connection
  while( !cb ) 
  {
    //Serial.println( "Problem reading from NTP server. Retrying in 10 seconds..." );
    delay( 10000);
    //Serial.println( "Trying again..." );
    cb = udp.parsePacket();
  }

    // We've received a packet, read the data from it
    udp.read( packetBuffer, NTP_PACKET_SIZE ); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word( packetBuffer[40], packetBuffer[41] );
    unsigned long lowWord = word( packetBuffer[42], packetBuffer[43] );
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;

    epoch = epoch + (TIMEZONE_OFFSET * (inputTimezoneOffset + 1));

    int hour = ((epoch  % 86400L) / 3600);
    int minute = ((epoch  % 3600) / 60);
    int second = (epoch % 60);
    
    // make a 12 hour clock
    bool isPM = false;
    if( hour >= 12 )
    {
      isPM = true;
      if( hour > 12 )
      {
        hour = hour - 12;
      }
    }
    if( hour == 0 )
    {
      hour = 12;
    }
    
    returnValue = String( hour );
    returnValue = String( returnValue + " " );
    returnValue = String( returnValue + minute );
    returnValue = String( returnValue + " " );
    returnValue = String( returnValue + second );
    
    if( isPM )
    {
      returnValue = String( returnValue + " 1" );
    }
    else
    {
      returnValue = String( returnValue + " 0" );
    }

    humanTime = String( hour );
    humanTime = String( humanTime + ":" );
    humanTime = String( humanTime + minute );
    humanTime = String( humanTime + ":" );
    humanTime = String( humanTime + second );
    if( isPM )
    {
      humanTime = String( humanTime + " PM" );
    }
    else
    {
      humanTime = String( humanTime + " AM" );
    }

  return returnValue;
}

/**
 * Writes all time components to member variables for easier parsing and display.
 * 
 * This should only be run immediately following a call to getTime()
 * 
 * @return void
 * @author costmo
 * @since  20180902
 */
void timeToVars()
{
  int separator1 = fullTime.indexOf( ' ' );
  int separator2 = fullTime.indexOf( ' ', (separator1 + 1) );
  int separator3 = fullTime.indexOf( ' ', (separator2 + 1) );

  String hourAsString = fullTime.substring( 0, separator1 );
  String minuteAsString = fullTime.substring( separator1, separator2 );
  String secondAsString = fullTime.substring( separator2, separator3 );
  String meridiemAsString = fullTime.substring( separator3 );


  currentHour = hourAsString.toInt();
  currentMinute = minuteAsString.toInt();
  int second = secondAsString.toInt();

  currentSecond = second;
  if( currentSecond >= 60 )
  {
    currentSecond = (currentSecond % 60);
    currentMinute += 1;
  }

  if( currentMinute >= 60 )
  {
    currentMinute = (currentMinute % 60);
    currentHour += 1;
  }

  if( currentHour >= 12 )
  {
    currentMeridiem = 1;
    currentHour = (currentHour % 12);
    if( currentHour == 0 )
    {
      currentHour = 11;
    }
    currentHour += 1;
  }
  else
  {
    currentMeridiem = 0;
  }

  if( currentHour < 12 && currentMeridiem > 0 )
  {
    currentHour = currentHour + 12;
  }

  String stringMinute = String( currentMinute );
  if( currentMinute < 10 )
  {
    stringMinute = "0" + stringMinute;
  }

  String timeString = String( currentHour );
  timeString = timeString + stringMinute;
  intTime = timeString.toInt();
}

 /**
 * Send an NTP request to the time server at the given address
 * 
 * @return long
 * @author costmo
 * @since  20180902
 * @param  IPAddress    address       A pointer to the IP address of the NTP server
 */
unsigned long sendNTPpacket( IPAddress& address )
{
  //Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

String getSoftAPStatus()
{
  String returnValue = "\n";
  returnValue += "SSID: " + ssid + "\n";
  returnValue += "IP  : " + softIP + "\n";

  return returnValue;
}

const String connectionHtml()
{
  const String returnValue =
    "<!DOCTYPE HTML>"
    "<html>"
    "<head>"
    "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
    "<title>Connect to WiFi</title>"
    "<link href='https://fonts.googleapis.com/css?family=Open+Sans' rel='stylesheet' type='text/css'>"
    "<style>"
    "body { background-color: #D3E3F1; font-family: \"Open Sans\", sans-serif; Color: #000000; }"
    "#header { width: 100%; text-align: center; }"
    "input[type=text], input[type=password] { width: 50%; height: 30px; padding-left: 10px; }"
    "</style>"
    "</head>"
    "<body>"
    "<div id='header'><h3>Connect to your WiFi network</h3></div>"
    "<FORM action=\"/connect\" method=\"post\">"
    "<P>"
    "<p><INPUT type=\"text\" name=\"ssid\" placeholder=\"SSID\" autocomplete=\"off\" autocorrect=\"off\" autocapitalize=\"off\" spellcheck=\"false\"></p>"
    "<p><INPUT type=\"password\" name=\"password\" placeholder=\"Password\" autocomplete=\"off\" autocorrect=\"off\" autocapitalize=\"off\" spellcheck=\"false\"></p>"
    "<p><INPUT type=\"text\" name=\"timezone\" placeholder=\"Timezone Offset (e.g. -8)\" autocomplete=\"off\" autocorrect=\"off\" autocapitalize=\"off\" spellcheck=\"false\"></p><br>"
    "<p><INPUT type=\"submit\" value=\"CONNECT\"></p>"
    "</P>"
    "</FORM>"
    "</body>"
    "</html>";

  return returnValue;
}

const String connectedHtml()
{
  const String returnValue =
    "<!DOCTYPE HTML>"
    "<html>"
    "<head>"
    "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
    "<title>Connect to WiFi</title>"
    "<link href='https://fonts.googleapis.com/css?family=Open+Sans' rel='stylesheet' type='text/css'>"
    "<style>"
    "body { background-color: #D3E3F1; font-family: \"Open Sans\", sans-serif; Color: #000000; }"
    "#header { width: 100%; text-align: center; }"
    "</style>"
    "</head>"
    "<body>"
    "You are connected to WiFi"
    "</body>"
    "</html>";

  return returnValue;
}