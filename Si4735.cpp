/* Arduino Si4735 Library
 * Written by Ryan Owens for SparkFun Electronics 5/17/11
 * Altered by Wagner Sartori Junior 09/13/11
 * Actively Being Developed by Jon Carrier
 * Minor refactoring and cosmetic changes by Radu - Eosif Mihailescu 3/15/12
 *
 * This library is for use with the SparkFun Si4735 Shield or Breakout Board
 * Released under the 'Buy Me a Beer' license
 * (If we ever meet, you buy me a beer)
 *
 * See the header file for better function documentation.
 *
 * See the example sketches to learn how to use the library in your code.
*/

#include "Si4735.h"
#include "Si4735-private.h"
#include <SPI.h>

void Si4735Translate::getTextForPTY(byte PTY, byte locale, char* text,
                                    byte textsize){
    const char* _PTY2Text_EU[32] = {"None/Undefined", "News",
                                    "Current affairs", "Information", "Sport",
                                    "Education", "Drama", "Culture",
                                    "Science", "Varied", "Pop music",
                                    "Rock music", "Easy listening",
                                    "Light classical", "Serious classical",
                                    "Other music", "Weather", "Finance",
                                    "Children's programmes", "Social affairs",
                                    "Religion", "Phone-in", "Travel",
                                    "Leisure", "Jazz music", "Country music",
                                    "National music", "Oldies music",
                                    "Fold music", "Documentary", "Alarm test",
                                    "Alarm"};
    const char* _PTY2Text_US[32] = {"None/Undefined", "News", "Information",
                                    "Sports", "Talk", "Rock", "Classic rock",
                                    "Adult hits", "Soft rock", "Top 40",
                                    "Country", "Oldies", "Soft", "Nostalgia",
                                    "Jazz", "Classical", "Rhythm and blues",
                                    "Soft rhythm and blues", "Language",
                                    "Religious music", "Religious talk",
                                    "Personality", "Public", "College",
                                    "Unassigned", "Unassigned", "Unassigned",
                                    "Unassigned", "Unassigned", "Weather",
                                    "Emergency test", "Emergency"};

    switch(locale){
        case SI4735_LOCALE_US:
            strncpy(text, _PTY2Text_US[PTY], textsize);
            break;
        case SI4735_LOCALE_EU:
            strncpy(text, _PTY2Text_EU[PTY], textsize);
            break;
    }
}

byte Si4735Translate::translatePTY(byte PTY, byte fromlocale, byte tolocale){
    const byte _PTY_EU2US[32] = {0, 1, 0, 2, 3, 23, 0, 0, 0, 0, 7, 5, 12, 15,
                                 15, 0, 29, 0, 0, 0, 20, 4, 0, 0, 14, 10, 0,
                                 11, 0, 0, 30, 31};
    const byte _PTY_US2EU[32] = {0, 1, 3, 4, 21, 11, 11, 10, 11, 10, 25, 27,
                                 12, 27, 24, 14, 15, 15, 0, 20, 20, 0, 0, 5,
                                 0, 0, 0, 0, 0, 16, 30, 31};
    if(fromlocale == tolocale) return PTY;
    else switch(fromlocale){
        case SI4735_LOCALE_US:
            return _PTY_US2EU[PTY];
            break;
        case SI4735_LOCALE_EU:
            return _PTY_EU2US[PTY];
            break;
    }
    
    //Never reached
    return 0;
}

void Si4735Translate::decodeCallSign(word programIdentifier, char* callSign){
    //TODO: read the standard and implement world-wide PI decoding
    if(programIdentifier >= 21672){
        callSign[0] = 'W';
        programIdentifier -= 21672;
    } else 
        if(programIdentifier < 21672 && programIdentifier >= 0x1000){
            callSign[0] = 'K';
            programIdentifier -= 0x1000;
        } else programIdentifier -= 1;
    if(programIdentifier >= 0){
        callSign[1] = char(programIdentifier / 676 + 'A');
        callSign[2] = char((programIdentifier - 676 * programIdentifier / 
                            676) / 26 + 'A');
        callSign[3] = char(((programIdentifier - 676 * programIdentifier /
                             676) % 26 ) + 'A');
        callSign[4] = '\0';        
    } else strcpy(callSign, "UNKN");
}

Si4735::Si4735(byte pinPower, byte pinReset, byte pinGPO2, byte pinSDIO,
               byte pinGPO1, byte pinSCLK, byte pinSEN){
    _status.mode = SI4735_MODE_FM;
    _rdstime.tm_year = 1970;
    _rdstime.tm_mday = 1;
    _rdstime.tm_mon = 1;
    _rdstime.tm_hour = 0;
    _rdstime.tm_min = 0;
    resetRDS();
    _pinPower = pinPower;
    _pinReset = pinReset;
    _pinGPO2 = pinGPO2;
    _pinSDIO = pinSDIO;
    _pinGPO1 = pinGPO1;
    _pinSCLK = pinSCLK;
    _pinSEN = pinSEN;
}

void Si4735::begin(byte mode){
    _status.mode = mode;

    //Start by resetting the Si4735 and configuring the communication protocol
    //to SPI
    //TODO: implement 2-wire and 3-wire versions as well, 2-wire comes in
    //      especially handy for pin count-constrained applications
    if(_pinPower != 0xFF) pinMode(_pinPower, OUTPUT);
    pinMode(_pinReset, OUTPUT);
    //GPO1 must be driven high after reset to select SPI
    pinMode(_pinGPO1, OUTPUT);
    //GPO2 must be driven high after reset to select SPI
    pinMode(_pinGPO2, OUTPUT);
    pinMode(_pinSCLK, OUTPUT);

    //Sequence the power to the Si4735
    if(_pinPower != 0xFF) digitalWrite(_pinPower, LOW);
    digitalWrite(_pinReset, LOW);

    //Configure the device for SPI communication
    digitalWrite(_pinGPO1, HIGH);
    digitalWrite(_pinGPO2, HIGH);
    //Use the longest of delays given in the datasheet
    delayMicroseconds(100);
    if(_pinPower != 0xFF) {
        digitalWrite(_pinPower, HIGH);
        //datasheet calls for 250us between VIO and RESET
        delayMicroseconds(250);
    };
    digitalWrite(_pinSCLK, LOW);
    //datasheet calls for no rising SCLK edge 300ns before RESET rising edge
    //but Arduino can only go as low as 3us
    delayMicroseconds(5);
    digitalWrite(_pinReset, HIGH);

    //Now configure the I/O pins properly
    pinMode(_pinGPO1, INPUT);
    pinMode(_pinGPO2, INPUT); 

    //Configure the SPI hardware
    SPI.begin();
    //datahseet says Si4735 can't do more than 2.5MHz on SPI
    SPI.setClockDivider(SPI_CLOCK_DIV8);
    //SCLK idle LOW, SDIO sampled on RISING edge
    SPI.setDataMode(SPI_MODE0);
    //datasheet says Si4735 is big endian (MSB first)
    SPI.setBitOrder(MSBFIRST);

    setMode(_status.mode, false);
}

void Si4735::sendCommand(byte command, byte arg1, byte arg2, byte arg3, 
                         byte arg4, byte arg5, byte arg6, byte arg7){
    byte status;

#if defined(SI4735_DEBUG)
    Serial.print("Si4735 CMD 0x");
    Serial.print(command, HEX);
    Serial.print(" (0x");
    Serial.print(arg1, HEX);
    Serial.print(" [");
    Serial.print(arg1, BIN);
    Serial.print("], 0x");    
    Serial.print(arg2, HEX);
    Serial.print(" [");
    Serial.print(arg2, BIN);
    Serial.print("], 0x");
    Serial.print(arg3, HEX);
    Serial.print(" [");
    Serial.print(arg3, BIN);
    Serial.println("],");
    Serial.print("0x");
    Serial.print(arg4, HEX);
    Serial.print(" [");
    Serial.print(arg4, BIN);
    Serial.print("], 0x");
    Serial.print(arg5, HEX);
    Serial.print(" [");
    Serial.print(arg5, BIN);
    Serial.print("], 0x");
    Serial.print(arg6, HEX);
    Serial.print(" [");
    Serial.print(arg6, BIN);
    Serial.print("], 0x");
    Serial.print(arg7, HEX);
    Serial.print(" [");
    Serial.print(arg7, BIN);
    Serial.println("])");
#endif

    digitalWrite(_pinSEN, LOW);
    //datasheet calls for 30ns, Arduino can only go as low as 3us
    delayMicroseconds(5);
    SPI.transfer(SI4735_CP_WRITE8);
    SPI.transfer(command);
    SPI.transfer(arg1);
    SPI.transfer(arg2);
    SPI.transfer(arg3);
    SPI.transfer(arg4);
    SPI.transfer(arg5);
    SPI.transfer(arg6);
    SPI.transfer(arg7);
    //datahseet calls for 5ns, Arduino can only go as low as 3us
    delayMicroseconds(5);
    digitalWrite(_pinSEN, HIGH);
    
    //Each command takes a different time to decode inside the chip; readiness
    //for next command and, indeed, availability/validity of reponse data is
    //being signalled by CTS in status byte.
    //Furthermore, the datasheet specifically mandates waiting for CTS to come
    //back up before doing anything else, *including* attempting to read back
    //the response from the last command sent.
    //Therefore, we poll for CTS coming back up after we send the command.
    do {
        status = getStatus();
    } while(!(status & SI4735_STATUS_CTS));
}

void Si4735::setFrequency(word frequency){    
    _status.frequency = frequency;
    switch(_status.mode){
        case SI4735_MODE_FM:
            sendCommand(SI4735_CMD_FM_TUNE_FREQ, 0x00, 
                        highByte(_status.frequency), 
                        lowByte(_status.frequency));
            break;
        case SI4735_MODE_AM:
        case SI4735_MODE_SW:
        case SI4735_MODE_LW:
            sendCommand(SI4735_CMD_AM_TUNE_FREQ, 0x00,
                        highByte(_status.frequency), 
                        lowByte(_status.frequency), 0x00, 
                        ((_status.mode == SI4735_MODE_SW) ? 0x01 : 0x00));
            break;
    }
    waitForInterrupt(SI4735_STATUS_STCINT);
    if(_status.mode == SI4735_MODE_FM) enableRDS();
}

void Si4735::getRevision(char* FW, char* CMP, char* REV){
    sendCommand(SI4735_CMD_GET_REV);
    getResponse(_response);    

    if(FW) {
        FW[0] = _response[2];
        FW[1] = _response[3];
        FW[2] = '\0';
    }
    if(CMP) {
        CMP[0] = _response[6];
        CMP[1] = _response[7];
        CMP[2] = '\0';
    }
    if(REV) *REV = _response[8];    
}

word Si4735::getFrequency(boolean* valid){
    switch(_status.mode){
        case SI4735_MODE_FM:            
            sendCommand(SI4735_CMD_FM_TUNE_STATUS, SI4735_FLG_INTACK);
            break;
        case SI4735_MODE_AM:
        case SI4735_MODE_SW:
        case SI4735_MODE_LW:
            sendCommand(SI4735_CMD_AM_TUNE_STATUS, SI4735_FLG_INTACK);
            break;
    }    
    getResponse(_response);    

    //Since we get these for free, update our data structures too
    _status.frequency = word(_response[2], _response[3]);
    _status.signalQuality.RSSI = _response[4];
    _status.signalQuality.SNR = _response[5];
    if(_status.mode == SI4735_MODE_FM)
        _status.signalQuality.MULT = _response[6];

    if(valid) *valid = (_response[1] & SI4735_STATUS_VALID);
    return _status.frequency;
}

void Si4735::seekUp(boolean wrap){
    switch(_status.mode){
        case SI4735_MODE_FM:
            sendCommand(SI4735_CMD_FM_SEEK_START, 
                        (SI4735_FLG_SEEKUP | 
                         (wrap ? SI4735_FLG_WRAP : 0x00)));
            break;
        case SI4735_MODE_AM:
        case SI4735_MODE_SW:
        case SI4735_MODE_LW:
            sendCommand(SI4735_CMD_AM_SEEK_START, 
                        (SI4735_FLG_SEEKUP | (wrap ? SI4735_FLG_WRAP : 0x00)),
                        0x00, 0x00, 0x00, 
                        ((_status.mode == SI4735_MODE_SW) ? 0x01 : 0x00));
            break;
    }
    waitForInterrupt(SI4735_STATUS_STCINT);
    if(_status.mode == SI4735_MODE_FM) enableRDS();
}

void Si4735::seekDown(boolean wrap){
    switch(_status.mode){
        case SI4735_MODE_FM:
            sendCommand(SI4735_CMD_FM_SEEK_START, 
                        (wrap ? SI4735_FLG_WRAP : 0x00));
            break;
        case SI4735_MODE_AM:
        case SI4735_MODE_SW:
        case SI4735_MODE_LW:
            sendCommand(SI4735_CMD_AM_SEEK_START, 
                        (wrap ? SI4735_FLG_WRAP : 0x00), 0x00, 0x00, 0x00, 
                        ((_status.mode == SI4735_MODE_SW) ? 0x01 : 0x00));
            break;
    }
    waitForInterrupt(SI4735_STATUS_STCINT);
    if(_status.mode == SI4735_MODE_FM) enableRDS();
}

void Si4735::setSeekThresholds(byte SNR, byte RSSI){
    switch(_status.mode){
        case SI4735_MODE_FM:
            setProperty(SI4735_PROP_FM_SEEK_TUNE_SNR_THRESHOLD, 
                        word(0x00, constrain(SNR, 0, 127)));
            setProperty(SI4735_PROP_FM_SEEK_TUNE_RSSI_THRESHOLD,
                        word(0x00, constrain(RSSI, 0, 127)));                
            break;
        case SI4735_MODE_AM:
        case SI4735_MODE_SW:
        case SI4735_MODE_LW:    
            setProperty(SI4735_PROP_AM_SEEK_TUNE_SNR_THRESHOLD, 
                        word(0x00, constrain(SNR, 0, 63)));
            setProperty(SI4735_PROP_AM_SEEK_TUNE_RSSI_THRESHOLD,
                        word(0x00, constrain(RSSI, 0, 63)));                
            break;
    }
}

void Si4735::updateRDS(void){
    byte grouptype;
    word groups[4], fourchars[2];

    //See if there's anything for us to do
    if(!(_status.mode == SI4735_MODE_FM &&
         (getStatus() & SI4735_STATUS_RDSINT))) return;
    
    _haverds = true;
    //Grab the next available RDS group from the chip
    sendCommand(SI4735_CMD_FM_RDS_STATUS, SI4735_FLG_INTACK);
    getResponse(_response);
    groups[0] = word(_response[4], _response[5]);
    groups[1] = word(_response[6], _response[7]);
    groups[2] = word(_response[8], _response[9]);
    groups[3] = word(_response[10], _response[11]);
 
    _status.programIdentifier = groups[0];
    grouptype = lowByte((groups[1] & SI4735_RDS_TYPE_MASK) >>
                        SI4735_RDS_TYPE_SHR);
    _status.TP = groups[1] & SI4735_RDS_TP;
    _status.PTY = lowByte((groups[1] & SI4735_RDS_PTY_MASK) >>
                          SI4735_RDS_PTY_SHR);
    
    switch(grouptype){
        case SI4735_GROUP_0A:
        case SI4735_GROUP_0B:
        case SI4735_GROUP_15B:
            byte DIPSA;
            word twochars;
            
            _status.TA = groups[1] & SI4735_RDS_TA;
            _status.MS = groups[1] & SI4735_RDS_MS;
            DIPSA = lowByte(groups[1] & SI4735_RDS_DIPS_ADDRESS);
            bitWrite(_status.DICC, 3 - DIPSA, groups[1] & SI4735_RDS_DI);
            twochars = switchEndian(groups[3]);
            strncpy(&_status.programService[DIPSA * 2], (char *)&twochars, 2);
            if(grouptype == SI4735_GROUP_0A) {
                //TODO: read the standard and do AF list decoding
            }
            break;
        case SI4735_GROUP_1A:
        case SI4735_GROUP_1B:
            //TODO: read the standard and do PIN and slow labeling codes
            break;
        case SI4735_GROUP_2A:
        case SI4735_GROUP_2B:
            byte RTA, RTAW;
            
            if((groups[1] & SI4735_RDS_TEXTAB) != _rdstextab) {
                _rdstextab = !_rdstextab;
                memset(_status.radioText, ' ', 64);
            }
            RTA = lowByte(groups[1] & SI4735_RDS_TEXT_ADDRESS);
            RTAW = (grouptype == SI4735_GROUP_2A) ? 4 : 2;
            fourchars[0] = switchEndian(
                groups[(grouptype == SI4735_GROUP_2A) ? 2 : 3]);
            if(grouptype == SI4735_GROUP_2A) 
                fourchars[1] = switchEndian(groups[3]);
            strncpy(&_status.radioText[RTA * RTAW], (char *)fourchars, RTAW);
            break;
        case SI4735_GROUP_3A:
            //TODO: read the standard and do AID listing
            break;
        case SI4735_GROUP_3B:
        case SI4735_GROUP_4B:
        case SI4735_GROUP_6A:
        case SI4735_GROUP_6B:
        case SI4735_GROUP_7B:
        case SI4735_GROUP_8B:
        case SI4735_GROUP_9B:
        case SI4735_GROUP_10B:
        case SI4735_GROUP_11A:
        case SI4735_GROUP_11B:
        case SI4735_GROUP_12A:
        case SI4735_GROUP_12B:
        case SI4735_GROUP_13B:
            //Application data payload (ODA), ignore for now
            break;
        case SI4735_GROUP_4A:
            unsigned long MJD, CT, ys;
            word yp;
            byte k, mp;
            
            CT = ((unsigned long)groups[2] << 16) | groups[3];
            //The standard mandates that CT must be all zeros if no time
            //information is being provided by the current station
            if(!CT) break;

            _havect = true;            
            MJD = (unsigned long)(groups[1] & SI4735_RDS_MJD_MASK) <<
                  SI4735_RDS_MJD_SHL;
            MJD |= (CT & SI4735_RDS_TIME_MJD_MASK) >> SI4735_RDS_TIME_MJD_SHR;

            //We report UTC for now, better not fiddle with locales
            _rdstime.tm_hour = (CT & SI4735_RDS_TIME_HOUR_MASK) >>
                               SI4735_RDS_TIME_HOUR_SHR;
            _rdstime.tm_min = (CT & SI4735_RDS_TIME_MINUTE_MASK) >>
                              SI4735_RDS_TIME_MINUTE_SHR;
            //Use integer arithmetic at all costs, Arduino lacks an FPU
            yp = (MJD * 10 - 150782) * 10 / 36525;
            ys = yp * 36525 / 100;
            mp = (MJD * 10 - 149561 - ys * 10) * 1000 / 306001;
            _rdstime.tm_mday = MJD - 14956 - ys - mp * 306001 / 10000;
            k = (mp == 14 || mp == 15) ? 1 : 0;
            _rdstime.tm_year = 1900 + yp + k;
            _rdstime.tm_mon = mp - 1 - k * 12;
            _rdstime.tm_wday = (MJD + 2) % 7 + 1;
            break;
        case SI4735_GROUP_5A:
        case SI4735_GROUP_5B:
            //TODO: read the standard and do TDC listing
            break;
        case SI4735_GROUP_7A:
            //TODO: read the standard and do Radio Paging
            break;
        case SI4735_GROUP_8A:
            //TODO: read the standard and do TMC listing
            break;
        case SI4735_GROUP_9A:
            //TODO: read the standard and do EWS listing
            break;
        case SI4735_GROUP_10A:
            if((groups[1] & SI4735_RDS_PTYNAB) != _rdsptynab) {
                _rdsptynab = !_rdsptynab;
                memset(_status.programTypeName, ' ', 8);
            }
            fourchars[0] = switchEndian(groups[2]);
            fourchars[1] = switchEndian(groups[3]);
            strncpy(&_status.programTypeName[(groups[1] & 
                                              SI4735_RDS_PTYN_ADDRESS) * 4], 
                    (char *)&fourchars, 4);            
            break;
        case SI4735_GROUP_13A:
            //TODO: read the standard and do Enhanced Radio Paging
            break;
        case SI4735_GROUP_14A:
        case SI4735_GROUP_14B:
            //TODO: read the standard and do EON listing
            break;
        case SI4735_GROUP_15A:
            //Withdrawn and currently unallocated, ignore
            break;
    }
}

void Si4735::getStationInfo(Si4735_Station* tunedStation) {
    makePrintable(_status.programService);
    makePrintable(_status.programTypeName);
    makePrintable(_status.radioText);
    *tunedStation = _status;
}

boolean Si4735::isRDSCapable(void){
    return _haverds;
}

boolean Si4735::getRDSTime(Si4735_RDS_Time* timedate){
    if(!(_haverds && _havect)) return false;
    if(timedate) *timedate = _rdstime;
    
    return _havect;
}

void Si4735::getRSQ(Si4735_RX_Metrics* RSQ){
    //This function gets the Received Signal Quality information
    switch(_status.mode){
        case SI4735_MODE_FM:            
            sendCommand(SI4735_CMD_FM_RSQ_STATUS, SI4735_FLG_INTACK);            
            break;
        case SI4735_MODE_AM:
        case SI4735_MODE_SW:
        case SI4735_MODE_LW:
            sendCommand(SI4735_CMD_AM_RSQ_STATUS, SI4735_FLG_INTACK);            
            break;
    }    
    //Now read the response    
    getResponse(_response);    

    //Pull the response data into their respecive fields and update our copy
    _status.signalQuality.RSSI = _response[4];
    _status.signalQuality.SNR = _response[5];
    if(_status.mode == SI4735_MODE_FM){
        _status.signalQuality.PILOT = _response[3] & SI4735_STATUS_PILOT;
        _status.signalQuality.STBLEND = (_response[3] & 
                                         (~SI4735_STATUS_PILOT));
        _status.signalQuality.MULT = _response[6];
        _status.signalQuality.FREQOFF = _response[7];
    }

    *RSQ = _status.signalQuality;
}

boolean Si4735::volumeUp(void){
    byte volume;

    volume = getVolume();
    if(volume < 63) {
        setVolume(++volume);
        return true;
    } else return false;
}

boolean Si4735::volumeDown(boolean alsomute){
    byte volume;

    volume = getVolume();
    if(volume > 0) {
        setVolume(--volume);
        return true;
    } else {
        if(alsomute) mute();
        return false;
    };
}

void Si4735::setVolume(byte value){
    setProperty(SI4735_PROP_RX_VOLUME, word(0x00, constrain(value, 0, 63)));
}

byte Si4735::getVolume(void){    
    return lowByte(getProperty(SI4735_PROP_RX_VOLUME));
}

void Si4735::mute(void){
    setProperty(SI4735_PROP_RX_HARD_MUTE, word(0x00, 0x03));
}

void Si4735::unMute(boolean minvol){
    if(minvol) setVolume(0);
    setProperty(SI4735_PROP_RX_HARD_MUTE, word(0x00, 0x00));
}

byte Si4735::getStatus(void){
    byte response;

    digitalWrite(_pinSEN, LOW);
    //datasheet calls for 30ns, Arduino can only go as low as 3us
    delayMicroseconds(5);
    SPI.transfer(SI4735_CP_READ1_GPO1);
    response = SPI.transfer(0x00);
    //datahseet calls for 5ns, Arduino can only go as low as 3us
    delayMicroseconds(5);
    digitalWrite(_pinSEN, HIGH);

    return response;
}

void Si4735::getResponse(byte* response){
    digitalWrite(_pinSEN, LOW);
    //datasheet calls for 30ns, Arduino can only go as low as 3us
    delayMicroseconds(5);
    SPI.transfer(SI4735_CP_READ16_GPO1);
    for(int i = 0; i < 16; i++) response[i] = SPI.transfer(0x00);
    //datahseet calls for 5ns, Arduino can only go as low as 3us
    delayMicroseconds(5);
    digitalWrite(_pinSEN, HIGH);

#if defined(SI4735_DEBUG)
    Serial.print("Si4735 RSP");
    for(int i = 0; i < 4; i++) {
        if(i) Serial.print("           ");
        else Serial.print(" ");
        for(int j = 0; j < 4; j++) {
            Serial.print("0x");
              Serial.print(response[i * 4 + j], HEX);
            Serial.print(" [");
            Serial.print(response[i * 4 + j], BIN);
            Serial.print("]");
            if(j != 3) Serial.print(", ");
            else
                if(i != 3) Serial.print(",");
        };
        Serial.println("");
    };
#endif
}

void Si4735::end(boolean hardoff){
    sendCommand(SI4735_CMD_POWER_DOWN);
    if(hardoff) {
        //datasheet calls for 10ns, Arduino can only go as low as 3us
        delayMicroseconds(5);
        SPI.end();
        digitalWrite(_pinReset, LOW);
        if(_pinPower != 0xFF) digitalWrite(_pinPower, LOW);
    };
}

void Si4735::setDeemphasis(byte deemph){
    switch(_status.mode){
        case SI4735_MODE_FM:            
            setProperty(SI4735_PROP_FM_DEEMPHASIS, word(0x00, deemph));        
            break;
        case SI4735_MODE_AM:
        case SI4735_MODE_LW:
        case SI4735_MODE_SW:        
            setProperty(SI4735_PROP_AM_DEEMPHASIS, word(0x00, deemph));        
            break;
    }
}

void Si4735::setMode(byte mode, boolean powerdown){
    if(powerdown) end(false);
    _status.mode = mode;
    
    switch(_status.mode){
        case SI4735_MODE_FM:
            sendCommand(SI4735_CMD_POWER_UP, SI4735_FLG_GPO2IEN | 
                        SI4735_FLG_XOSCEN | SI4735_FUNC_FM,
                        SI4735_OUT_ANALOG);
            break;
        case SI4735_MODE_AM:
        case SI4735_MODE_SW:
        case SI4735_MODE_LW:
            sendCommand(SI4735_CMD_POWER_UP, SI4735_FLG_GPO2IEN | 
                        SI4735_FLG_XOSCEN | SI4735_FUNC_AM,
                        SI4735_OUT_ANALOG);
            break;
    }

    //Configure GPO lines to maximize stability
    sendCommand(SI4735_CMD_GPIO_CTL, SI4735_FLG_GPO1OEN | SI4735_FLG_GPO2OEN);
    sendCommand(SI4735_CMD_GPIO_SET, SI4735_FLG_GPO2LEVEL);

    //Disable Mute
    unMute();

    //Set the seek band for the desired mode (AM and FM can use defaults)
    switch(_status.mode){
        case SI4735_MODE_SW:
            //Set the lower band limit for Short Wave Radio to 2.3 MHz
            setProperty(SI4735_PROP_AM_SEEK_BAND_BOTTOM, 0x08FC);            
            //Set the upper band limit for Short Wave Radio to 23 MHz
            setProperty(SI4735_PROP_AM_SEEK_BAND_TOP, 0x59D8);
            break;
        case SI4735_MODE_LW:
            //Set the lower band limit for Long Wave Radio to 152 kHz
            setProperty(SI4735_PROP_AM_SEEK_BAND_BOTTOM, 0x0099);            
            //Set the upper band limit for Long Wave Radio to 279 kHz
            setProperty(SI4735_PROP_AM_SEEK_BAND_BOTTOM, 0x0117);                            
            break;
    }
    
    //Enable end-of-seek and RDS interrupts
    //TODO: write interrupt handlers for STCINT and RDSINT
    setProperty(
        SI4735_PROP_GPO_IEN, 
        word(0x00, 
             ((_status.mode == SI4735_MODE_FM) ? SI4735_FLG_RDSIEN : 0x00) | 
             SI4735_FLG_STCIEN));
}

byte Si4735::getMode(void){
    return _status.mode;
}

void Si4735::setProperty(word property, word value){
    sendCommand(SI4735_CMD_SET_PROPERTY, 0x00, highByte(property), 
                lowByte(property), highByte(value), lowByte(value));
}

word Si4735::getProperty(word property){    
    sendCommand(SI4735_CMD_GET_PROPERTY, 0x00, highByte(property), 
                lowByte(property));
    getResponse(_response);

    return word(_response[2], _response[3]);
}

/*******************************************
*
* Private Functions
*
*******************************************/

void Si4735::resetRDS(void){
    _status.callSign[0] = '\0';
    memset(_status.programService, ' ', 8);
    _status.programService[8] = '\0';
    memset(_status.programTypeName, ' ', 8);
    _status.programTypeName[8] = '\0';
    memset(_status.radioText, ' ', 64);
    _status.radioText[64] = '\0';    
    _status.DICC = 0;
    _rdstextab = false;
    _rdsptynab = false;
    _haverds = false;
    _havect = false;
}

void Si4735::makePrintable(char* str){
    for(byte i = 0; i < strlen(str); i++) {
        if(str[i] == 0x0D) {
            str[i] = '\0';
            break;
        }
        if(str[i] < 32 || str[i] > 126) str[i] = '?';
    }
}

void Si4735::enableRDS(void){
    //Enable and configure RDS reception
    if(_status.mode == SI4735_MODE_FM) {
        setProperty(SI4735_PROP_FM_RDS_INT_SOURCE, word(0x00, 
                                                        SI4735_FLG_RDSRECV));
        setProperty(SI4735_PROP_FM_RDS_INT_FIFO_COUNT, word(0x00, 0x01));
        setProperty(SI4735_PROP_FM_RDS_CONFIG, word(SI4735_FLG_BLETHA_35 | 
                    SI4735_FLG_BLETHB_35 | SI4735_FLG_BLETHC_35 | 
                    SI4735_FLG_BLETHD_35, SI4735_FLG_RDSEN));
    };
    resetRDS();
}

void Si4735::waitForInterrupt(byte which){
    while(!(getStatus() & which)){
        //Balance being snappy with hogging the chip
        delay(125);
        sendCommand(SI4735_CMD_GET_INT_STATUS);
    }
}

word Si4735::switchEndian(word value){
    return (value >> 8) | (value << 8);
}