
#include <stddef.h> /* size_t */
#include <stdint.h> /* uintx_t */

/* Raccourci pour struct pkt */
typedef struct pkt pkt_t;

/* Types de paquets */
typedef enum {
	PTYPE_DATA = 1,
	PTYPE_ACK = 2,
} ptypes_t;

/* Taille maximale permise pour le payload */
#define MAX_PAYLOAD_SIZE 512
/* Taille maximale de Window */
#define MAX_WINDOW_SIZE 31

/* Valeur de retours des fonctions */
typedef enum {
	PKT_OK = 0,     /* Le paquet a Ã©tÃ© traitÃ© avec succÃ¨s */
	E_TYPE,         /* Erreur liÃ©e au champs Type */
	E_LENGTH,       /* Erreur liÃ©e au champs Length  */
	E_CRC,          /* CRC invalide */
	E_WINDOW,       /* Erreur liÃ©e au champs Window */
	E_SEQNUM,       /* NumÃ©ro de sÃ©quence invalide */
	E_NOMEM,        /* Pas assez de mÃ©moire */
	E_NOHEADER,     /* Le paquet n'a pas de header (trop court) */
	E_UNCONSISTENT, /* Le paquet est incohÃ©rent */
} pkt_status_code;

/* Alloue et initialise une struct pkt
 * @return: NULL en cas d'erreur */
pkt_t* pkt_new();
/* LibÃ¨re le pointeur vers la struct pkt, ainsi que toutes les
 * ressources associÃ©es
 */
void pkt_del(pkt_t *pkt);

/*
 * DÃ©code des donnÃ©es reÃ§ues et crÃ©e une nouvelle structure pkt.
 * Le paquet reÃ§u est en network byte-order.
 * La fonction vÃ©rifie que:
 * - Le CRC32 des donnÃ©es reÃ§ues est le mÃªme que celui dÃ©codÃ© Ã  la fin
 *   du flux de donnÃ©es
 * - Le type du paquet est valide
 * - La longeur du paquet est valide et cohÃ©rente avec le nombre d'octets
 *   reÃ§us.
 *
 * @data: L'ensemble d'octets constituant le paquet reÃ§u
 * @len: Le nombre de bytes reÃ§us
 * @pkt: Une struct pkt valide
 * @post: pkt est la reprÃ©sentation du paquet reÃ§u
 *
 * @return: Un code indiquant si l'opÃ©ration a rÃ©ussi ou reprÃ©sentant
 *         l'erreur rencontrÃ©e.
 */
pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt);

/*
 * Encode une struct pkt dans un buffer, prÃªt Ã  Ãªtre envoyÃ© sur le rÃ©seau
 * (c-Ã -d en network byte-order), incluant le CRC32 du header et payload.
 *
 * @pkt: La structure Ã  encoder
 * @buf: Le buffer dans lequel la structure sera encodÃ©e
 * @len: La taille disponible dans le buffer
 * @len-POST: Le nombre de d'octets Ã©crit dans le buffer
 * @return: Un code indiquant si l'opÃ©ration a rÃ©ussi ou E_NOMEM si
 *         le buffer est trop petit.
 */
pkt_status_code pkt_encode(const pkt_t *pkt, char *buf, size_t *len);

/* Accesseurs pour les champs toujours prÃ©sents du paquet.
 * Les valeurs renvoyÃ©es sont toutes dans l'endianness native
 * de la machine!
 */
ptypes_t pkt_get_type(const pkt_t *pkt);
uint8_t  pkt_get_window(const pkt_t *pkt);
uint8_t  pkt_get_seqnum(const pkt_t *pkt);
uint16_t pkt_get_length(const pkt_t *pkt);
uint32_t pkt_get_timestamp(const pkt_t *pkt);
uint32_t pkt_get_crc(const pkt_t *pkt);
/* Renvoie un pointeur vers le payload du paquet, ou NULL s'il n'y
 * en a pas.
 */
const char* pkt_get_payload(const pkt_t *pkt);

/* Setters pour les champs obligatoires du paquet. Si les valeurs
 * fournies ne sont pas dans les limites acceptables, les fonctions
 * doivent renvoyer un code d'erreur adaptÃ©.
 * Les valeurs fournies sont dans l'endianness native de la machine!
 */
pkt_status_code pkt_set_type     (pkt_t *pkt, const ptypes_t type);
pkt_status_code pkt_set_window   (pkt_t *pkt, const uint8_t window);
pkt_status_code pkt_set_seqnum   (pkt_t *pkt, const uint8_t seqnum);
pkt_status_code pkt_set_length   (pkt_t *pkt, const uint16_t length);
pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp);
pkt_status_code pkt_set_crc      (pkt_t *pkt, const uint32_t crc);
/* DÃ©fini la valeur du champs payload du paquet.
 * @data: Une succession d'octets reprÃ©sentants le payload
 * @length: Le nombre d'octets composant le payload
 * @POST: pkt_get_length(pkt) == length */
pkt_status_code pkt_set_payload(pkt_t *pkt,
                                const char *data,
                                const uint16_t length);
