/* 
 * File:   octo_pdu.cpp
 * Author: kamyshev.a
 * 
 * Created on 28 June 2018, 9: 17
 */

#include "octo_pdu.h"

#include <algorithm>
#include <string.h>
#include <memory>

#define MAX_HDR_SIZE 56
#define MIN_HDR_SIZE 16

typedef std::unique_ptr<uint8_t> u8up_t;

#pragma pack( push, 1 )

struct my_wc_t
{
  uint8_t c1;
  uint8_t c0;
};
#pragma pack( pop )

char buf[3] = {0, 0, 0};

static bool not_digit( uint8_t v )
{
  return !isdigit( v );
}

static void my_swab( std::string s )
{
  for ( uint32_t i = 1; i < s.size( ); i += 2 )
    std::swap( s[i - 1], s[i] );
}

static void my_swab( std::string &src, std::string &dst )
{
  dst.resize( src.size( ) );
  for ( uint32_t i = 1; i < src.size( ); i += 2 )
  {
    dst[i - 1] = src[i];
    dst[i] = src[i - 1];
  }
}

static uint8_t read_byte_from_hex( char *hex )
{
  memcpy( buf, hex, 2 );
  return strtol( (char*)buf, 0, 16 );
}

static u8up_t hex_to_bin( const uint8_t* hex, uint32_t len )
{
  u8up_t bin( new uint8_t[len] );
  for ( uint32_t i = 0; i < len; i++ )
  {
    memcpy( buf, hex + i * 2, 2 );
    bin.get( )[i] = strtol( (char*)buf, 0, 16 );
  }
  return std::move( bin );
}

static void dcs8_to_dcs7( std::string &s )
{
  if ( !s.size( ) ) return;
  //uint8_t *b = (uint8_t*)s.c_str( );
  uint8_t len = s.size( );
  uint8_t i1 = 0, i2 = 1, j = 1;
  for (; i2 < len; i1++, i2++, j++ )
  {
    if ( j == 8 ) break;
    s[i1] |= s[i2] << 8 - j;
    s[i2] >>= j;
  }
  for (; i2 < len; i1++, i2++, j++ )
  {
    if ( j == 8 )
    {
      s[i1] = s[i2++];
      j = 1;
    }
    s[i1 + 1] = s[i2];
    s[i1] |= s[i1 + 1] << 8 - j;
    s[i1 + 1] >>= j;
  }
  s.resize( len - ( len >> 3 ) );
}

static void dcs7_to_dcs8( std::string &s )
{
  if ( !s.size( ) ) return;
  uint8_t i1 = s.size( );
  uint8_t i2 = i1 + i1 / 7;
  s.resize( i2, 0 );
  uint8_t *b = (uint8_t*)s.c_str( );
  uint8_t j = i1 % 7;
  while ( i1 )
  {
    i1--;
    i2--;
    if ( j == 0 )
    { // begin octet
      j = 7;
      b[i2] = b[i1] >> 1;
      i1++;
      continue;
    }
    if ( j == 1 )
    { // end octet
      j--;
      b[i2] = b[i1];
      b[i2] &= 0x7F;
      continue;
    }
    b[i2] = b[i1] << --j;
    b[i2] |= b[i1 - 1] >> 8 - j;
    b[i2] &= 0x7F;
  }
}

TOctoPDU::TOctoPDU( pdu_type_t type )
{
  pdu_type = type;
  memset( &TPDU, 0, sizeof( TPDU ) );
}

TOctoPDU::TOctoPDU( const TOctoPDU& orig ) { }

TOctoPDU::~TOctoPDU( ) { }

bool TOctoPDU::SetSMSCenterAddress( std::string num )
{
  num.erase( std::remove_if( num.begin( ), num.end( ), not_digit ), num.end( ) );
  if ( !num.size( ) ) return 0;
  if ( num.size( ) % 2 ) num.append( "F" );
  if ( num.size( ) > 12 ) return 0;
  SCA.type = NUM_TYPE_INTERNATIONAL;
  SCA.len = num.size( ) / 2 + 1; // bytes ammount + 1 byte of type (facepalm)
  my_swab( num, SCA.num );
  return 1;
}

std::string TOctoPDU::GetSMSCenterAddress( )
{
  if ( !SCA.num.size( ) ) return "";
  std::string s;
  my_swab( SCA.num, s );
  return s;
}

bool TOctoPDU::SetAddress( std::string num )
{
  num.erase( std::remove_if( num.begin( ), num.end( ), not_digit ), num.end( ) );
  if ( !num.size( ) ) return 0;
  uint8_t digit_count = num.size( );
  if ( num.size( ) % 2 ) num.append( "F" );
  if ( num.size( ) > 12 ) return 0;
  address.type = NUM_TYPE_INTERNATIONAL;
  address.len = digit_count; // digit count without byte of type (double facepalm)
  my_swab( num, address.num );
  return 1;
}

std::string TOctoPDU::GetAddress( )
{
  if ( !address.num.size( ) ) return "";
  if ( address.type == NUM_TYPE_ALPHABET )
  {
    std::string s;
    s.resize( address.num.size( ) / 2 );
    for ( uint32_t i = 0; i < s.size( ); i++ )
    {
      s[i] = 0;
      read_byte_from_hex( &address.num[i * 2] );
    }
    dcs7_to_dcs8( s );
    return s;
  }
  if ( address.type == NUM_TYPE_INTERNATIONAL )
  {
    std::string s;
    my_swab( address.num, s );
    return s;
  }

  return address.num;
}

void TOctoPDU::SetUserData( std::string s )
{
  TPDU.dcs |= DCS_7;
  TPDU.udl = s.size( );
  dcs8_to_dcs7( s );
  UD.resize( s.size( ) * 2 + 1 );
  for ( uint32_t i = 0; i < s.size( ); i++ )
    snprintf( &UD[i * 2], 3, "%02X", (uint8_t)s[i] );
}

void TOctoPDU::SetUserData( std::wstring s )
{
  TPDU.dcs |= DCS_UCS2;
  TPDU.udl = s.size( );
  UD.resize( s.size( ) * 4 + 1 );
  //uint8_t *data = (uint8_t*)UD.c_str( );
  for ( uint32_t i = 0; i < s.size( ); i++ )
  {
    my_wc_t *wc = ( my_wc_t* ) & s[i];
    snprintf( &UD[i * 4], 3, "%02X", wc->c0 );
    snprintf( &UD[i * 4 + 2], 3, "%02X", wc->c1 );
  }
}

std::string TOctoPDU::GetUserData( )
{
  std::string ud;
  ud.resize( UD.size( ) / 2 );
  for ( uint32_t i = 0; i < ud.size( ); i++ )
    ud[i] = read_byte_from_hex( &UD[i * 2] );
  if ( ( TPDU.dcs & 0x0F ) == DCS_7 )
  {
    dcs7_to_dcs8( ud );
    ud.resize( TPDU.udl );
    return ud;
  }
  return UD;
}

std::wstring TOctoPDU::GetUserDataW( )
{
  std::wstring ws;
  ws.resize( UD.size( ) / 2 );
  for ( uint32_t i = 0; i < ws.size( ); i++ )
  {
    my_wc_t *wc = ( my_wc_t* ) & ws[i];
    wc->c0 = read_byte_from_hex( &UD[i * 4] );
    wc->c1 = read_byte_from_hex( &UD[i * 4 + 2] );
  }
  return ws;
}

std::string TOctoPDU::GetPDUString( )
{
  std::string pdu( MAX_HDR_SIZE, '0' );
  uint32_t pos = 2;
  char *data = (char*)pdu.c_str( );
  if ( SCA.len )
  {
    snprintf( data, MAX_HDR_SIZE, "%02X%02X", SCA.len, SCA.type );
    memcpy( data + 4, SCA.num.c_str( ), SCA.num.size( ) );
    pos = 4 + SCA.num.size( );
  }
  TPDU.type.mti = 0x01; // outgoing
  snprintf( data + pos, MAX_HDR_SIZE - pos, "%02X", *( uint8_t* ) & TPDU.type );
  pos += 2;
  if ( pdu_type == SMS_SUBMIT )
  {
    snprintf( data + pos, MAX_HDR_SIZE - pos, "%02X", TPDU.mr );
    pos += 2;
  }
  snprintf( data + pos, MAX_HDR_SIZE - pos, "%02X%02X", address.len, address.type );
  pos += 4;
  memcpy( data + pos, address.num.c_str( ), address.num.size( ) );
  pos += address.num.size( );
  snprintf( data + pos, MAX_HDR_SIZE - pos, "%02X%02X", TPDU.pid, TPDU.dcs );
  pos += 4;

  if ( pdu_type == SMS_SUBMIT )
  {
    if ( TPDU.type.vpf )
    {
      memcpy( data + pos, VP.c_str( ), VP.size( ) );
      if ( TPDU.type.vpf == 0x02 ) pos += 2;
      else pos += 14;
    }
  }
  else
  {
    memcpy( data + pos, STSC.c_str( ), STSC.size( ) );
    pos += 14;
  }
  snprintf( data + pos, MAX_HDR_SIZE - pos, "%02X", TPDU.udl );
  pdu.resize( pos + 2 );
  pdu.append( UD );
  return pdu;
}

uint32_t TOctoPDU::SetPDUString( std::string pdu )
{
  if ( pdu.size( ) < MIN_HDR_SIZE ) return 0;
  char *data = (char*)pdu.c_str( );
  uint32_t pos = 2;
  if ( data[0] != '0' || data[1] != '0' )
  { // SCA field present
    SCA.len = read_byte_from_hex( data );
    SCA.type = read_byte_from_hex( data + 2 );
    uint32_t num_len = ( SCA.len - 1 ) * 2;
    if ( num_len > pdu.size( ) + 4 ) return 0;
    SCA.num = pdu.substr( 4, num_len );
    pos = 4 + num_len;
  }
  else SCA.Clear( );
  ( *( uint8_t* )&TPDU.type ) = read_byte_from_hex( data + pos );
  pos += 2;
  if ( pdu_type == SMS_SUBMIT )
  {
    TPDU.mr = read_byte_from_hex( data + pos );
    pos += 2;
  }
  address.len = read_byte_from_hex( data + pos );
  address.type = read_byte_from_hex( data + pos + 2 );
  pos += 4;
  uint32_t num_len = address.len;
  if ( num_len % 2 ) num_len++;
  if ( num_len + pos > pdu.size( ) ) return 0;
  address.num = pdu.substr( pos, num_len );
  pos += num_len;
  if ( 4 + pos > pdu.size( ) ) return 0;
  TPDU.pid = read_byte_from_hex( data + pos );
  TPDU.dcs = read_byte_from_hex( data + pos + 2 );
  pos += 4;
  if ( pdu_type == SMS_SUBMIT )
  {
    if ( TPDU.type.vpf )
    { // VP field present
      memcpy( data + pos, VP.c_str( ), VP.size( ) );
      if ( TPDU.type.vpf == 0x02 )
      {
        if ( 2 + pos > pdu.size( ) ) return 0;
        VP = pdu.substr( pos, 2 );
        pos += 2;
      }
      else
      {
        if ( 14 + pos > pdu.size( ) ) return 0;
        VP = pdu.substr( pos, 14 );
        pos += 14;
      }
    }
  }
  else
  {
    if ( 14 + pos > pdu.size( ) ) return 0;
    STSC = pdu.substr( pos, 14 );
    pos += 14;
  }
  if ( 2 + pos > pdu.size( ) ) return 0;
  TPDU.udl = read_byte_from_hex( data + pos );
  pos += 2;
  uint32_t len = TPDU.udl;
  if ( ( TPDU.dcs & 0x0F ) == DCS_7 )
  {
    len -= len >> 3;
    len *= 2;
  }
  if ( len + pos > pdu.size( ) ) return 0;
  UD = pdu.substr( pos, len );
  return pos + 2;
}

