#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef WORDS_BIGENDIAN
#define le_uint32(a) (swap32((a)))
#define le_uint16(a) (swap16((a)))
#define le_int16(a) ( (int16_t) swap16((uint16_t)((a))) )
#else
#define le_uint32(a) (a)
#define le_uint16(a) (a)
#define le_int16(a) (a)
#endif

typedef struct _riff_t {
	char riff[4] ;	/* "RIFF" (ASCII characters) */
	uint32_t  len ;	/* Length of package (binary, little endian) */
	char wave[4] ;	/* "WAVE" (ASCII characters) */
} riff_t;

/* The FORMAT chunk */

typedef struct _format_t {
	char  fmt[4] ;		/* "fmt_" (ASCII characters) */
	uint32_t   len ;	/* length of FORMAT chunk (always 0x10) */
	uint16_t  type;		/* codec type*/
	uint16_t channel ;	/* Channel numbers (0x01 = mono, 0x02 = stereo) */
	uint32_t   rate ;	/* Sample rate (binary, in Hz) */
	uint32_t   bps ;	/* Average Bytes Per Second */
	uint16_t blockalign ;	/*number of bytes per sample */
	uint16_t bitpspl ;	/* bits per sample */
} format_t;

/* The DATA chunk */

typedef struct _data_t {
	char data[4] ;	/* "data" (ASCII characters) */
	int  len ;	/* length of data */
} data_t;

typedef struct _wave_header_t
{
	riff_t riff_chunk;
	format_t format_chunk;
	data_t data_chunk;
} wave_header_t;

typedef enum _PlayerState{
    Closed,
    Paused,
    Playing,
    Eof,
    Dummy
}PlayerState;

typedef enum _RecorderState{
    RecorderClosed,
    RecorderPaused,
    RecorderRunning
}RecorderState;
