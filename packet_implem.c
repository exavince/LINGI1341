#include "packet_interface.h"


struct __attribute__((__packed__)) pkt {
	uint8_t type : 3,
					window : 5;
	uint8_t seqnum;
	uint16_t length;
	uint32_t timestamp;
	char *payload;
	uint32_t crc;
};

pkt_t* pkt_new() {
	struct pkt *new = calloc(1, sizeof(pkt_t));
	new->payload = NULL;

	return new;
}

void pkt_del (pkt_t *pkt) {
	if(pkt == NULL) {
		break;
	}
	else {
		if(pkt->payload != NULL) {
			free(pkt->payload);
		}
		free(pkt);
	}
}

pkt_status_code pkt_decode (const char *data, const size_t len, pkt_t *pkt) {
	/* Your code will be inserted here */
}

pkt_status_code pkt_encode (const pkt_t* pkt, char *buf, size_t *len)
{
	/* Your code will be inserted here */
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

uint32_t pkt_get_crc (const pkt_t*) {
	return pkt->crc;
}

const char* pkt_get_payload(const pkt_t*) {
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
	pkt->payload = (char *) malloc(sizeof(char)*length);
	if (pkt->payload == NULL) {
		return E_NOMEM;
	}
	strncpy(pkt->payload, data, length);
	return 0;
}
