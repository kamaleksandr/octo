/* 
 * File:   octo_pdu.h
 * Author: kamyshev.a
 *
 * Created on 28 June 2018, 9: 17
 */
#ifndef OCTO_PDU_H
#define OCTO_PDU_H

#include <stdint.h>
#include <string>

class TOctoPDU
{
public:
  enum pdu_type_t
  {
    SMS_SUBMIT,
    SMS_DELIVER
  };
  enum
  { // number format
    NUM_TYPE_NATIONAL = 0xA1,
    NUM_TYPE_INTERNATIONAL = 0x91,
    NUM_TYPE_ALPHABET = 0xD0,
    NUM_TYPE_NETWORK = 0xB1
  };
  enum
  { // data coding scheme
    DCS_7 = 0x00,
    DCS_8 = 0x04,
    DCS_UCS2 = 0x08,
    DCS_FLASH = 0x10,
    DCS_AUTO = 0xFF
  };
private:
  pdu_type_t pdu_type;
  struct pdu_number_t
  {
    uint8_t len;
    uint8_t type;
    std::string num;
    pdu_number_t(){ Clear(); }
    void Clear(){ len = 0; type = 0; num.clear(); }
  };
  pdu_number_t SCA; // Service Center Address
  pdu_number_t address; // originator/destination Address
  std::string STSC; // Service Center Time Stamp
  std::string VP; // Validity-Period
  std::string UD;
  struct
  {
#pragma pack( push, 1 )
    struct
    {
      uint8_t mti : 2; // Message Type Indicator, 0b01 - outgoing
      uint8_t rd : 1; // Reject duplicates
      uint8_t vpf : 2; // Validity Period Format, 0b00 - no vp field
      uint8_t srr : 1; // Status report request
      uint8_t udhi : 1; // User data header indicator
      uint8_t rp : 1; // Reply path
    } type;
#pragma pack( pop )
    uint8_t mr;
    uint8_t pid;
    uint8_t dcs;
    uint8_t udl;
  } TPDU; // Transport Protocol Data Unit

public:
  TOctoPDU( pdu_type_t type = SMS_SUBMIT );
  TOctoPDU( const TOctoPDU& orig );
  virtual ~TOctoPDU( );

  bool SetSMSCenterAddress( std::string );
  std::string GetSMSCenterAddress( );
  bool SetAddress( std::string );
  std::string GetAddress( );
  void SetUserData( std::string );
  std::string GetUserData( );
  void SetUserData( std::wstring );
  std::wstring GetUserDataW( );

  std::string GetPDUString( );
  uint32_t SetPDUString( std::string );
};
#endif /* OCTO_PDU_H */

