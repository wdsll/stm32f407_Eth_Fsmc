#ifndef UTILITY_H_
#define UTILITY_H_

#include "board_config.h"


u16 atoi16(char* str,u16 base); 			/* Convert a string to integer number */
uint32 atoi32(char* str,u16 base); 			/* Convert a string to integer number */
//void itoa(u16 n,u8* str, u8 len);
int validatoi(char* str, int base, int* ret); 		/* Verify character string and Convert it to (hexa-)decimal. */
char c2d(u8 c); 					/* Convert a character to HEX */

u16 swaps(u16 i);
uint32 swapl(uint32 l);

void replacetochar(char * str, char oldchar, char newchar);

void mid(char* src, char* s1, char* s2, char* sub);
void inet_addr_(unsigned char* addr,unsigned char *ip);


char* inet_ntoa(unsigned long addr);			/* Convert 32bit Address into Dotted Decimal Format */
char* inet_ntoa_pad(unsigned long addr);

uint32 inet_addr(unsigned char* addr);		/* Converts a string containing an (Ipv4) Internet Protocol decimal dotted address into a 32bit address */

char verify_ip_address(char* src, u8 * ip);/* Verify dotted notation IP address string */

u16 htons( unsigned short hostshort);	/* htons function converts a unsigned short from host to TCP/IP network byte order (which is big-endian).*/

uint32 htonl(unsigned long hostlong);		/* htonl function converts a unsigned long from host to TCP/IP network byte order (which is big-endian). */

uint32 ntohs(unsigned short netshort);		/* ntohs function converts a unsigned short from TCP/IP network byte order to host byte order (which is little-endian on Intel processors). */

uint32 ntohl(unsigned long netlong);		/* ntohl function converts a u_long from TCP/IP network order to host byte order (which is little-endian on Intel processors). */

u16 checksum(unsigned char * src, unsigned int len);		/* Calculate checksum of a stream */

u8 check_dest_in_local(uint32 destip);			/* Check Destination in local or remote */

#endif
