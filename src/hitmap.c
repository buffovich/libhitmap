#define _GNU_SOURCE

#include <hitmap.h>

#include <atomic_ops.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

void hitmap_init( AO_t map[], int len_pow ) {
	if( len_pow < _WORD_POW )
		len_pow = _WORD_POW;

	size_t wcapacity = 1ul << ( len_pow - _WORD_POW );
	memset( map, 0, wcapacity );

	if( len_pow > _WORD_POW )
		for( AO_t *cur = map + wcapacity + 1,
				*limit = map + hitmap_calc_sz( len_pow );
			cur < limit;
			cur += 2
		)
			*cur = SIZE_MAX;
}

#define _BIT_NUMBER_MASK ( HITMAP_INITIAL_SIZE - 1 )

void hitmap_change_for( AO_t map[],
	int len_pow,
	AO_t idx,
	enum hitmap_mark is_put
) {
	assert( map != NULL );
	assert( ( is_put == BIT_SET ) || ( is_put == BIT_UNSET ) );

	if( len_pow < _WORD_POW )
		len_pow = _WORD_POW;

	int level_limit = len_pow - _WORD_POW + 1;
	size_t widx = idx >> _WORD_POW;
	int is_full = map[ widx ] & ( 1ul << ( ( idx & _BIT_NUMBER_MASK ) ^ 1ul ) );
	switch( is_put ) {
		case BIT_SET:
			map[ widx ] |= 1ul << ( idx & _BIT_NUMBER_MASK );
			break;
		case BIT_UNSET:
			map[ widx ] &= ~ ( 1ul << ( idx & _BIT_NUMBER_MASK ) );
			is_full = ! is_full;
			break;
	}

	if( level_limit < 2 )
		return;

	idx >>= 1;
	widx >>= 1;

	size_t wcapacity = 1 << ( len_pow - _WORD_POW );
	AO_t *level_border = map + wcapacity;
	wcapacity >>= 1;
	AO_t bit = 0;
	for( int level = 1;
		level < level_limit;
		++level
	) {
		bit = 1ul << ( idx & _BIT_NUMBER_MASK );
		level_border[ widx * 2 + 1 - is_put ] |= bit;

		if( is_full ) {
			level_border[ widx * 2 + is_put ] &= ~bit;
			is_full = ! (
					level_border[ widx * 2 + is_put ] &
					( 1ul << ( ( idx & _BIT_NUMBER_MASK ) ^ 1ul ) )
				);
		}

		idx >>= 1;
		widx >>= 1;
		level_border += wcapacity * 2;
		wcapacity >>= 1;
	}
}

inline static size_t _ffsl_after( AO_t mword, size_t idx ) {
	int ret = ffsl( mword >> idx );
	
	if( ret && ( ret > 0 ) )
		return ( idx + ret );
	else
		return 0;

	return 0;
}

static size_t _ffcl_after( AO_t mword, size_t idx ) {
	AO_t mask = 1 << idx;
	for( size_t cyc = idx + 1;
		cyc <= ( sizeof( unsigned long ) * 8 );
		++cyc, mask <<= 1
	)
		if( mword & mask )
			return cyc;

	return 0;
}

static int _map_bob_up( AO_t map[],
	int len_pow,
	int *levelp,
	size_t *idxp,
	enum hitmap_find which
) {
	int level_limit = len_pow - _WORD_POW + 1;
	size_t capacity = 1 << len_pow;
	int level = *levelp;
	size_t idx = *idxp;

	if( level < 1 ) {
		AO_t mword = AO_load( &( map[ idx >> _WORD_POW ] ) );
		size_t bitn = ( which == FIND_SET ) ?
			_ffsl_after( mword, idx & _BIT_NUMBER_MASK ) :
			_ffcl_after( mword, idx & _BIT_NUMBER_MASK );
			
		if( bitn != 0 ) {
			*idxp = ( idx & ( ~ _BIT_NUMBER_MASK ) ) | ( bitn - 1 );
			return 1;
		}

		if( ( level + 1 ) >= level_limit )
			return 0;

		idx = ( idx & ( ~ _BIT_NUMBER_MASK ) ) + HITMAP_INITIAL_SIZE;

		if( idx >= capacity  )
			return 0;

		++level;
		idx >>= 1;
	}

	AO_t *level_border = map + hitmap_calc_sz( len_pow ) -
		( ( 1 << ( level_limit - level ) ) - 1 ) * 2;

	capacity >>= level;
	size_t wcapacity = capacity >> _WORD_POW;
	for( size_t bitn = 0;
		level < level_limit;
		++level
	) {
		bitn = _ffsl_after(
			AO_load( &( level_border[ ( idx >> _WORD_POW ) + which ] ) ),
			idx & _BIT_NUMBER_MASK
		);

		if( bitn != 0 ) {
			*idxp = ( idx & ( ~ _BIT_NUMBER_MASK ) ) | ( bitn - 1 );
			*levelp = level;
			return 1;
		}

		idx = ( idx & ( ~ _BIT_NUMBER_MASK ) ) + HITMAP_INITIAL_SIZE;

		if( idx >= capacity  )
			return 0;

		idx >>= 1;
		level_border += wcapacity * 2;
		wcapacity >>= 1;
		capacity >>= 1;
	}

	return 0;
}

static int _map_sink( AO_t map[],
	int len_pow,
	int *levelp,
	size_t *idxp,
	enum hitmap_find which
) {
	if( *levelp < 1 )
		return 1;

	int level = *levelp - 1;
	size_t idx = *idxp << 1;
	size_t wcapacity = 1ul << ( len_pow - _WORD_POW - level );

	if( level > 0 ) {
		AO_t *level_border = map + hitmap_calc_sz( len_pow ) -
			( ( 1 << ( len_pow - _WORD_POW - level + 1 ) ) - 1 ) * 2;

		for( size_t bitn = 0;
			level > 0;
			--level
		) {
			bitn = _ffsl_after(
				AO_load( &( level_border[ ( idx >> _WORD_POW ) + which ] ) ),
				idx & _BIT_NUMBER_MASK
			);

			if( bitn == 0 ) {
				*idxp = idx;
				*levelp = level;
				return 0;
			}

			idx = ( ( idx & ( ~ _BIT_NUMBER_MASK ) ) | ( bitn - 1 ) ) << 1;
			wcapacity <<= 1;
			level_border -= wcapacity * 2;
		}
	}

	AO_t mword = AO_load( &( map[ idx >> _WORD_POW ] ) );
	size_t bitn = ( which == FIND_SET ) ?
		_ffsl_after( mword, idx & _BIT_NUMBER_MASK ) :
		_ffcl_after( mword, idx & _BIT_NUMBER_MASK );

	if( bitn != 0 ) {
		*idxp = ( idx & ( ~ _BIT_NUMBER_MASK ) ) | ( bitn - 1 );
		*levelp = level;
		return 1;
	}

	*idxp = idx;
	*levelp = level;

	return 0;
}

size_t hitmap_discover( AO_t map[],
	int len_pow,
	size_t start_idx,
	enum hitmap_find which
) {
	int level = 0;

	do {
		if( ! _map_bob_up( map, len_pow, &level, &start_idx, which ) )
			return SIZE_MAX;
	} while (
		! _map_sink( map, len_pow, &level, &start_idx, which )
	);

	return start_idx;
}

int hitmap_has( AO_t map[], int len_pow, enum hitmap_mark which ) {
	if( len_pow <= _WORD_POW ) {
		AO_t mword = AO_load( map );
		
		switch( which ) {
			case BIT_UNSET:
				return ( mword != SIZE_MAX );
			case BIT_SET:
				return ( mword != 0 );
		}
	}

	return (
		AO_load( &( map[ hitmap_calc_sz( len_pow ) - 2 + which ] ) ) != 0
	);
}
