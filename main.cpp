/* 
 * File:   main.cpp
 * Author: kamyshev.a
 *
 * Created on 28 June 2018, 11: 35
 */
#include <stdio.h>
#include <string.h>
#include "source/octo_pdu.h"

int main( int argc, char** argv )
{ 
  TOctoPDU pdu( TOctoPDU::SMS_SUBMIT );
  pdu.SetSMSCenterAddress( "+79107899999" );
  pdu.SetAddress( "+70000000000" );
  pdu.SetUserData( "Hello however" );
  //pdu.SetUserData( L"Hello however" );
  //std::wstring user_data_w = pdu.GetUserDataW();
  std::string spdu = pdu.GetPDUString();
  printf( "pdu: %s\r\n", spdu.c_str() );
  pdu.SetPDUString( spdu );
  std::string center_address = pdu.GetSMSCenterAddress();
  std::string address = pdu.GetAddress();
  std::string user_data = pdu.GetUserData();
  printf( "sms center address: %s\r\n", center_address.c_str() );
  printf( "address: %s\r\n", address.c_str() ); 
  printf( "user data: %s\r\n", user_data.c_str() );
  return 0;
}

