/*
 * A library for controlling RAK811 LoRa radio.
 *Modified Version of the original written by:
 * @Author Chace.cao
 * @Author john.zou
 * @Date 11/05/2017
 *
 */

#include "Arduino.h"
#include "RAK811.h"

extern "C" {
  #include "string.h"
  #include "stdlib.h"
  }

/*
  @param serial Needs to be an already opened Stream ({Software/Hardware}Serial) to write to and read from.
*/

// Convert Bytes To String Function by Rob Bricheno
const char *hexdigits = "0123456789ABCDEF";

char* convertBytesToString (uint8_t* inputBuffer, int inputSize) {
    int i, j;
    char* compositionBuffer = (char*) malloc(inputSize*2 + 1);
    for (i = j = 0; i < inputSize; i++) {
        unsigned char c;
        c = (inputBuffer[i] >> 4) & 0xf;
        compositionBuffer[j++] = hexdigits[c];
        c = inputBuffer[i] & 0xf;
        compositionBuffer[j++] = hexdigits[c];
    }
    compositionBuffer[inputSize*2] = '\0'; 
    return compositionBuffer;
}

RAK811::RAK811(Stream& serial,Stream& serial1):
_serial(serial),_serial1(serial1)
{
  _serial.setTimeout(2000);
  _serial1.setTimeout(1000);
}

String RAK811::rk_getVersion()
{
  String ret = sendRawCommand(F("at+version"));
  ret.trim();
  return ret;
}

String RAK811::rk_getBand()
{
  String ret = sendRawCommand(F("at+band"));
  ret.trim();
  return ret;
}

bool RAK811::rk_begin()
{
  String recv;
  delay(2000);
  while (_serial.available()) _serial.read();
  do
  {
    delay (7000);
    recv = "";
    recv = _serial.readString();
  } while((recv.indexOf(F("Join Success")) == -1) && recv.length() != 0);
  rk_wake();
  if (rk_sendBytes(111, (uint8_t *) "", 0)) return true;
  else return false;
  /*
  //wait for the very long startup message sent by the module
  delay(2000);
  //read what is available in the buffer so it doesn't get full
  while(_serial.available()) _serial.read();
  //waits for the Joins Success message with is sent by the module about 8 seconds later
  delay(7000);
  if (_serial.available())
  {
    _serial1.print("info received after waiting time:");
    String recv;
    while(_serial.available() > 25) _serial.read();
    recv = _serial.readString();
    _serial1.println(recv);
    if (recv == F("[LoRa]:Join Success\r\nOK\r\n"))
    {
      digitalWrite(8, HIGH);
      String ret = sendRawCommand(F("at+send=lora:111:"));
      if (ret.indexOf(F("send success")) != -1)
      {
        return true;
      }
      else return false;
    }
  }
  else
  {
    rk_wake();
    if (rk_join())
    {
      _serial.println("nothing received after waiting time");
      String ret = sendRawCommand(F("at+send=lora:111:"));
      if (ret.indexOf(F("send success")) != -1)
      {
        return true;
      }
      else return false;
    }
    else return false;
  }*/
}

String RAK811::rk_getSignal()
{
  String ret = sendRawCommand(F("at+signal"));
  ret.trim();
  return ret;
}

bool RAK811::rk_setRate(int rate)
{
  String ret = sendRawCommand("at+dr=" + (String)rate);
  if (ret.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

void RAK811::rk_sleep()
{
  sendRawCommand(F("at+set_config=device:sleep:1"));
}

void RAK811::rk_reload()
{
  sendRawCommand(F("at+reload"));
}

void RAK811::rk_wake()
{
  _serial.println("");
  delay(200);
}

void RAK811::rk_reset(int mode)
{
  if (mode == 1)
  {
    sendRawCommand(F("at+reset=1"));
  }
  else if(mode == 0)
  {
    sendRawCommand(F("at+reset=0"));
  }
  else
  {
    Serial.println("The mode set Error,the mode is '0'or '1'.");
  }
}

bool RAK811::rk_setWorkingMode(int mode)
{
  String ver;
  switch (mode)
  {
    case 0:
      ver = sendRawCommand(F("at+set_config=lora:work_mode:0")); //Set LoRaWAN Mode.
      break;
    case 1:
      ver = sendRawCommand(F("at+set_config=lora:work_mode:1")); //Set LoRaP2P Mode.
      break;
    default:
      return false;
  }
  if (ver.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool RAK811::rk_join()
{
  String ret;
  ret = sendRawCommand(F("at+join"), 6500);
  if (ret.indexOf(F("Join Success")) != -1) return true;
  else return false;
}

bool RAK811::rk_joinMode(int mode)
{
  String ver;
  switch (mode)
  {
    case 0:
      ver = sendRawCommand(F("at+set_config=lora:join_mode:0")); //join Network through OTAA mode.
      break;
    case 1:
      ver = sendRawCommand(F("at+set_config=lora:join_mode:1")); //join Network through ABP mode.
      break;
    default:
      return false;
  }
  if (ver.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool RAK811::rk_initOTAA(String devEUI, String appEUI, String appKEY)
{
  String command = "";
  if (devEUI.length() == 16)
  {
    _devEUI = devEUI;
  }
  else
  {
    String ret = sendRawCommand(F("at+get_config=lora:status"));
    const char* ver = ret.c_str();
    ret = &ver[2];
//  Serial.println(ret);
    if (ret.length() == 16)
    {
      _devEUI = ret;
    }
  }
  if (appEUI.length() == 16)
  {
    _appEUI = appEUI;
  }
  else
  {
    Serial.println("The parameter appEUI is set incorrectly!");
  }
  if (appKEY.length() == 32 )
  {
    _appKEY = appKEY;
  }
  else
  {
    Serial.println("The parameter appKEY is set incorrectly!");
  }
  command = "at+set_config=lora:dev_eui:" + _devEUI;
//  Serial.println(command);
  String ret = sendRawCommand(command);
  if (ret.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool RAK811::rk_initABP(String devADDR, String nwksKEY, String appsKEY)
{
  String command = "";
  if (devADDR.length() == 8)
  {
    _devADDR = devADDR;
  }
  else
  {
    Serial.println("The parameter devADDR is set incorrectly!");
  }
  if (nwksKEY.length() == 32)
  {
    _nwksKEY = nwksKEY;
  }
  else
  {
    Serial.println("The parameter nwksKEY is set incorrectly!");
  }
  if (appsKEY.length() == 32 )
  {
    _appsKEY = appsKEY;
  }
  else
  {
    Serial.println("The parameter appsKEY is set incorrectly!");
  }
  command = "at+set_config=dev_addr:" + _devADDR + "&nwks_key:" + _nwksKEY + "&apps_key:" + _appsKEY;
//  Serial.println(command);
  String ret = sendRawCommand(command);
  if (ret.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool RAK811::rk_sendData(int port, char* datahex)
{
  String command = "";
  command = "at+send=lora:" + String(port) + ":"+ datahex;
  String ret = sendRawCommand(command);
  if (ret.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool RAK811::rk_sendBytes(int port, uint8_t* buffer, int bufSize)
{
  //Send Bytes Command
  String command = "";
  command = "at+send=lora:" + String(port) + ":" + convertBytesToString(buffer,bufSize);
  String ret = sendRawCommand(command);
  if (ret.indexOf(F("send success")) != -1) return true;
  else return false;
}

String RAK811::rk_recvData(void)
{
  _serial.setTimeout(2000);
  String ret = _serial.readStringUntil('\n');
#if defined DEBUG_MODE
  _serial1.println("-> " + ret);
#endif
  ret.trim();
  return ret;
}

bool RAK811::rk_setConfig(String Key,String Value)
{
  String command = "";
  command = "at+set_config=" + Key + ":" + Value;
//  Serial.println(command);
  String ret = sendRawCommand(command);
  if (ret.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

String RAK811::rk_getConfig(String Key)
{
  String ret = sendRawCommand("at+get_config=" + Key);
  ret.trim();
  return ret;
}

String RAK811::rk_getP2PConfig(void)
{
  String ret = sendRawCommand("at+rf_config");
  ret.trim();
  return ret;
}

bool RAK811::rk_initP2P(String FREQ, int SF, int BW, int CR, int PRlen, int PWR)
{
  String command = "";
  command = "at+rf_config=" + FREQ + "," + SF + "," + BW + "," + CR + "," + PRlen + "," + PWR;
//  Serial.println(command);
  String ret = sendRawCommand(command);
  if (ret.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool RAK811::rk_recvP2PData(int report_en)
{
  String ver;
  switch (report_en)
  {
    case 0:
      ver = sendRawCommand(F("at+rxc=0")); //
      break;
    case 1:
      ver = sendRawCommand(F("at+rxc=1")); //
      break;
    default:
      return false;
  }
  if (ver.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool RAK811::rk_sendP2PData(int CNTS, String interver, char* DATAHex)
{
  String command = "";
  command = "at+txc=" + (String)CNTS + "," + interver + "," + DATAHex;
//  Serial.println(command);
  String ret = sendRawCommand(command);
  if (ret.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool RAK811::rk_stopSendP2PData(void)
{
  String ret = sendRawCommand(F("at+tx_stop"));
  if (ret.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool RAK811::rk_stopRecvP2PData(void)
{
  String ret = sendRawCommand(F("at+rx_stop"));
  if (ret.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

String RAK811::rk_checkStatusStatistics(void)
{
  String ret = sendRawCommand(F("at+get_config=device:status"));
  ret.trim();
  return ret;
}

bool RAK811::rk_cleanStatusStatistics(void)
{
  String ret = sendRawCommand(F("at+status=0"));
  if (ret.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool RAK811::rk_setUARTConfig(int Baud, int Data_bits, int Parity, int Stop_bits, int Flow_ctrl)
{
  String command = "";
  command = "at+uart=" + (String)Baud + "," + Data_bits + "," + Parity + "," + Stop_bits + "," + Flow_ctrl;
//  Serial.println(command);
  String ret = sendRawCommand(command);
  if (ret.startsWith("OK"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

String RAK811::sendRawCommand(String command, int waitTime)
{
  if (_serial.available())
  {
    #if defined DEBUG_MODE
    _serial1.print("-> ");
    do
    {
      _serial1.print((char)_serial.read());
    } while (_serial.available());
    #else
    do
    {
      _serial.read();
    } while (_serial.available());
    #endif
  }
  _serial.println(command);
  _serial.flush();
  #if defined DEBUG_MODE
  _serial1.println("<- " + command);
  #endif
  delay(200);
  //this is for reading the command echo sent by the module, the two extra bytes in the reading are for the CRLF
  for (int i = 0; i < (command.length() + 2); i++)
  {
    _serial.read();
  }
  //this is to avoid the buffer to get full and loose any important data, this is important for join commands
  while (_serial.available() > 14) _serial.read();
  delay(waitTime);//this is the receive window time
  String ret = _serial.readString();
  ret.trim();
#if defined DEBUG_MODE
  _serial1.println("-> " + ret);
  _serial1.flush();
#endif
  return ret;
}
