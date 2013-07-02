#ifndef LIBHITMAP
#define LIBHITMAP

#include <atomic_ops.h>
#include <limits.h>

#define HITMAP_INITIAL_SIZE ( sizeof( AO_t ) * 8 )

#if ( ULONG_MAX != 4294967295UL )
	#define _WORD_POW ( 6 )
#else
	#define _WORD_POW ( 5 )
#endif

enum hitmap_mark {
	BIT_UNSET = 0,
	BIT_SET = 1
};

enum hitmap_find {
	FIND_UNSET = 1,
	FIND_SET = 0
};

inline static size_t hitmap_calc_sz( int len_pow ) {
	if( len_pow < _WORD_POW )
		return 1;

	return 3 * ( 1 << ( len_pow - _WORD_POW ) ) - 2;
}

extern void hitmap_init( AO_t map[], int len_pow );
extern void hitmap_change_for( AO_t map[],
	int len_pow,
	AO_t idx,
	enum hitmap_mark is_put
);
extern size_t hitmap_discover( AO_t map[],
	int len_pow,
	size_t start_idx,
	enum hitmap_find which
);
extern int hitmap_has( AO_t map[], int len_pow, enum hitmap_mark which );

#endif
