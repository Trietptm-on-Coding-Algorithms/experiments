// compile this for Linux and Android

#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h> // struct ifconf, struct ifreq
#include <net/if_dl.h> // link_ntoa()

/* MACOS: defined in /usr/include/net/if.h */
void if_flags_to_str(int flags, char *result)
{
	strcpy(result, "");

	if(flags & IFF_UP) strcat(result, "UP|");
	if(flags & IFF_BROADCAST) strcat(result, "BROADCAST|");
	if(flags & IFF_DEBUG) strcat(result, "DEBUG|");
	if(flags & IFF_LOOPBACK) strcat(result, "LOOPBACK|");
	if(flags & IFF_POINTOPOINT) strcat(result, "POINTOPOINT|");
	if(flags & IFF_NOTRAILERS) strcat(result, "NOTRAILERS|");
	if(flags & IFF_RUNNING) strcat(result, "RUNNING|");
	if(flags & IFF_NOARP) strcat(result, "NOARP|");
	if(flags & IFF_PROMISC) strcat(result, "PROMISC|");
	if(flags & IFF_ALLMULTI) strcat(result, "ALLMULTI|");
	if(flags & IFF_OACTIVE) strcat(result, "OACTIVE|");
	if(flags & IFF_SIMPLEX) strcat(result, "SIMPLEX|");
	if(flags & IFF_LINK0) strcat(result, "LINK0|");
	if(flags & IFF_LINK1) strcat(result, "LINK1|");
	if(flags & IFF_LINK2) strcat(result, "LINK2|");
	if(flags & IFF_ALTPHYS) strcat(result, "ALTPHYS|");
	if(flags & IFF_MULTICAST) strcat(result, "MULTICAST|");

	int n = strlen(result);
	if(result[n-1] == '|')
		result[n-1] = '\0';
}

void dump_bytes(uint8_t *buf, int len, uint32_t addr)
{
    int i, j;
    char ascii[17];
    i = 0;
    while(i < len) {
        printf("%08X: ", addr);
        for(j=0; j<16; ++j) {
            if(i < len) {
                printf("%02X ", buf[i]);
                if((buf[i] >= ' ') && (buf[i] < '~'))
                    ascii[j] = buf[i];
                else
                    ascii[j] = '.';
            }
            else {
                printf("   ");
                ascii[j] = ' ';
            }

            i++;
        }
        ascii[sizeof(ascii)-1] = '\0';
        printf(" %s\n", ascii);
        addr += 16;
    }
}

void method_ioctls(void)
{
    uint8_t buf[4096] = {0};
    int i;

	int sck = socket(AF_INET, SOCK_DGRAM, 0);

	memset(buf, 0, sizeof(buf));
	struct ifconf ifc;
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (char *)buf;
    if(ioctl(sck, SIOCGIFCONF, &ifc) != 0)
		{ perror("ioctl()"); goto cleanup; }

    // Iterate through the list of interfaces
	int iface_n = ifc.ifc_len / sizeof(struct ifreq);

	printf("each ifr is %lu (0x%lX) bytes long\n", sizeof(struct ifreq), sizeof(struct ifreq));
	printf("each ifr.ifr_name is %d (0x%X) bytes long\n", IF_NAMESIZE, IF_NAMESIZE);
	printf("it returned to us %d (0x%X) bytes\n",
		ifc.ifc_len, ifc.ifc_len);
	printf("there are %d interfaces\n", iface_n);
	struct ifreq *ifr = ifc.ifc_req;
	dump_bytes((void *)buf, 400, 0);

	uint8_t *cursor = buf;
	printf("cursor starts at: %p\n", cursor);
	while(cursor < (buf + ifc.ifc_len)) {
		struct ifreq *ifr = (struct ifreq *)cursor;
		struct sockaddr *saddr = &(ifr->ifr_addr);
		uint16_t family = saddr->sa_family;
		
		printf("name: %s (%d==", ifr->ifr_name, saddr->sa_family);
		//printf("size: %d\n", saddr->sa_len);

		switch(family) {
			case AF_UNSPEC: printf("AF_UNSPEC"); break;
			case AF_UNIX: printf("AF_UNIX"); break;
			case AF_INET: printf("AF_INET"); break;
			//case AF_LINK: printf("AF_LINK"); break; 
			case AF_INET6: printf("AF_INET6"); break; 
			default: printf("AF_UNKNOWN (%d)", family); break;
		}
		
		printf(")\n");

		if(family == AF_INET) {
			struct sockaddr_in *addri = (void *)saddr;
			printf("inet_ntoa(): %s\n", inet_ntoa(addri->sin_addr));
		}
		if(family == AF_INET6) {
			char buf[256];
			struct sockaddr_in6 *addri = (void *)saddr;
			inet_ntop(AF_INET6, &(addri->sin6_addr), buf, 256);
			printf("inet_ntop(): %s\n", buf);
		}


		cursor += IF_NAMESIZE; /* skip over .ifr_name */
		cursor += saddr->sa_len; /* skip over .ifru_addr */
		//printf("cursor is now: %p\n", cursor);
	}

	cleanup:
	return;
}

/* this uses the platform specific API's

	on Windows: GetAdaptersInfo() and GetAdaptersAddresses()
	on Linux: getifaddrs()
*/
int method_getifaddrs(void)
{
	int rc = -1;
	struct ifaddrs *ifap = NULL, *ifap_cur;
	char buf[1024];
	
	sa_family_t family;

	struct sockaddr *saddr; /* sys/socket.h */
	struct sockaddr_in *addr4; /* netinet/in.h */
	struct sockaddr_in6 *addr6; /* netinet6/in6.h */
	struct sockaddr_dl *addr_dl; /* net/if_dl.h */
	struct ether_addr *addr_eth;

	if(0 != getifaddrs(&ifap)) {
		printf("ERROR: getifaddrs()\n");
		goto cleanup;
	}

	ifap_cur = ifap;
	while(ifap_cur) {
		char flags_str[256];

		if_flags_to_str(ifap_cur->ifa_flags, flags_str);

		printf("name:%s flags:0x%X(%s) ",
		  ifap_cur->ifa_name, ifap_cur->ifa_flags, flags_str);

		family = ifap_cur->ifa_addr->sa_family;
		printf("family:%d\n", family);
		switch(family) {
			case AF_INET:
				addr4 = (void *)ifap_cur->ifa_addr;
				if(NULL == inet_ntop(family, &(addr4->sin_addr), buf, 1024)) {
					perror("ERROR: inet_ntop()\n");
					goto cleanup;
				}
				printf(" inet4 addr: %s\n", buf);
				break;
			case AF_INET6:
				addr6 = (void *)ifap_cur->ifa_addr;
				if(NULL == inet_ntop(family, &(addr6->sin6_addr), buf, 1024)) {
					perror("ERROR: inet_ntop()\n");
					goto cleanup;
				}
				printf(" inet6 addr: %s\n", buf);
				break;
			case AF_LINK:
				addr_dl = (void *)ifap_cur->ifa_addr;
				printf(" link_addr: %s\n", link_ntoa(addr_dl));
				break;

			default:
				printf("unknown (family id: %d)\n", family);
		}

		ifap_cur = ifap_cur->ifa_next;
	}

	rc = 0;
	cleanup:
	if(ifap) freeifaddrs(ifap);
	return rc;
}

int main(int ac, char **av)
{
	method_ioctls();

	printf("--------\n");

	method_getifaddrs();

	return 0;
}
