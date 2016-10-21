#include "packet_interface.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <netinet/in.h>
#include <time.h>


struct __attribute__((__packed__)) pkt {
    uint8_t window : 5, type : 3;
    uint8_t seqnum;
    uint16_t length;
    uint32_t timestamp;
    char *payload;
    uint32_t crc;
};

pkt_t* pkt_new() {
    pkt_t *new = malloc(sizeof(pkt_t));
    if(new == NULL) {
        return NULL;
    }
    new->payload = NULL;

    return new;
}

void pkt_del (pkt_t *pkt) {
  free(pkt);
}

pkt_status_code pkt_decode (const char *data, const size_t len, pkt_t *pkt) {
    if(len < 8) {
        return E_NOHEADER;
    }
    if(data == NULL) {
        return E_UNCONSISTENT;
    }
	memcpy(pkt, data, 4);
	uint16_t length = ntohs(pkt->length);
    pkt->length = length;
    uint32_t timestamp;
    memcpy((void *) &timestamp, (void *) data+4, 4);
    pkt->timestamp = timestamp;

    char *buffer = (char *) malloc(sizeof(char)*length);
    memcpy(buffer, data+8, length);
    pkt->payload = buffer;

    uint32_t crc;
    memcpy(&crc, data+7+length, 4);
    crc = ntohl(crc);
    pkt->crc = crc;

    return 0;
}

pkt_status_code pkt_encode (const pkt_t* pkt, char *buf, size_t *len)
{
    uint16_t length = htons(pkt->length);
    memcpy(buf, pkt, 2);
    buf[2] = length;
    buf[3] = length >> 8;
    memcpy(&buf[4], &(pkt->timestamp), 4);

    memcpy((void *) &buf[8], (void *) pkt->payload, pkt->length);

    uint32_t crc = crc32(0L, Z_NULL, 0);
    crc = htonl(crc32(0, (void*) buf, 8+(pkt->length)));
    memcpy((void *) &buf[8+pkt->length], (void *) &crc, 4);

    *len = 8+(pkt->length)+4;

    return PKT_OK;
}

ptypes_t pkt_get_type (const pkt_t *pkt) {
    return pkt->type;
}

uint8_t pkt_get_window (const pkt_t *pkt) {
    return pkt->window;
}

uint8_t pkt_get_seqnum (const pkt_t *pkt) {
    return pkt->seqnum;
}

uint16_t pkt_get_length (const pkt_t *pkt) {
    return pkt->length;
}

uint32_t pkt_get_timestamp (const pkt_t *pkt) {
    return pkt->timestamp;
}

uint32_t pkt_get_crc (const pkt_t *pkt) {
    return pkt->crc;
}

const char* pkt_get_payload(const pkt_t *pkt) {
    return pkt->payload;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type) {
    if (pkt != NULL) {
        pkt->type = type;
        return 0;
    }
    return E_TYPE;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window) {
    if (pkt != NULL) {
        pkt->window = window;
        return 0;
    }
    return E_WINDOW;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum) {
    if (pkt != NULL) {
        pkt->seqnum = seqnum;
        return 0;
    }
    return E_SEQNUM;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length) {
  if (pkt != NULL) {
      pkt->length = length;
      return 0;
  }
  return E_LENGTH;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp) {
    if (pkt != NULL) {
        pkt->timestamp = timestamp;
        return 0;
    }
    return E_UNCONSISTENT;
}

pkt_status_code pkt_set_crc(pkt_t *pkt, const uint32_t crc) {
    if (pkt != NULL) {
        pkt->crc = crc;
        return 0;
    }
    return E_CRC;
}

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length) {
    pkt->payload = (char *) malloc(sizeof(char)*length); //Attention +1
    if (pkt->payload == NULL) {
        return E_NOMEM;
    }
    if (data == NULL) {
      pkt->payload = NULL;
    }
    else {
      memcpy(pkt->payload, data, length);
      pkt->length = length;
      return 0;
    }
}


/*
int main(int argc, char *argv[]) {
	pkt_t *pkt = pkt_new();
	char *string = "Salut les gars";
	pkt_set_type(pkt, PTYPE_DATA);
	pkt_set_window(pkt, 1);
	pkt_set_length(pkt ,15);
	pkt_set_seqnum(pkt, 123);
	pkt_set_payload(pkt, string, 15);
  uint32_t timestamp = time(NULL);
  pkt_set_timestamp(pkt, timestamp);
  printf("Payload : %s\n", pkt->payload);
  printf("Type : %d\n", pkt->type);
  printf("Window : %d\n", pkt->window);
  printf("Seqnum : %d\n", pkt->seqnum);
  printf("Length : %d\n", pkt->length);
  printf("Timestamp : %d\n", pkt->timestamp);

  char buffer[8+(pkt->length)+4];
  size_t taille = 27;
  pkt_encode(pkt, (char *) &buffer, &taille);
  pkt_t *pkt2 = pkt_new();
  pkt_decode((char *) &buffer, taille, pkt2);
  printf("Payload : %s\n", pkt2->payload);
  printf("%d\n", pkt2->type);
  printf("%d\n", pkt2->window);
  printf("%d\n", pkt2->seqnum);
  printf("%d\n", pkt2->length);
  printf("%d\n", pkt2->timestamp);
  printf("%d\n", pkt2->crc);
}
*/
