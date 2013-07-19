#define _GNU_SOURCE

#include <hitmap.h>

#include <atomic_ops.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

static size_t _calc_elements_num( int len_pow ) {
	// everything wich is less than 1 << _WORD_POW
	// will be put to just one word
	if( len_pow <= _WORD_POW )
		return 1;

	len_pow -= _WORD_POW;
	// we don't need layered structure for less than 4 words
	if( len_pow < 2 )
		return 2;

	size_t sz = 1ul << len_pow;
	while( len_pow > _WORD_POW ) {
		len_pow -= _WORD_POW;
		sz += 1ul << ( len_pow + 1 );
	}

	if( len_pow >= 2 )
		sz += 2;

	return sz;
}

size_t hitmap_calc_sz( int len_pow ) {
	return ( sizeof( map_t ) +
		_calc_elements_num( len_pow ) * sizeof( AO_t )
	);
}

void hitmap_init( map_t *map, int len_pow ) {
	if( len_pow < _WORD_POW )
		len_pow = _WORD_POW;

	map->len_pow = len_pow;

	size_t el_num = _calc_elements_num( len_pow );
	memset( map->bits, 0, el_num * sizeof( AO_t ) );

	len_pow -= _WORD_POW;
	if( len_pow >= 2 )
		for( AO_t *cur = map->bits + ( 1ul << len_pow ) + 1,
				*limit = map->bits + el_num;
			cur < limit;
			cur += 2
		)
			*cur = SIZE_MAX;
}

#define _BIT_NUMBER_MASK ( HITMAP_INITIAL_SIZE - 1 )

static inline AO_t _group_mask_for_bit( int len_pow, size_t idx ) {
	if( len_pow >= _WORD_POW )
		return SIZE_MAX;

	return ( ( 1ul << ( 1ul << len_pow ) ) - 1 ) <<
		( idx & ( ~ ( ( 1ul << len_pow ) - 1 ) ) & _BIT_NUMBER_MASK );
}

void hitmap_change_for( map_t *map, size_t idx, enum hitmap_mark is_put ) {
	assert( map != NULL );
	assert( ( is_put == BIT_SET ) || ( is_put == BIT_UNSET ) );
	assert( idx < ( 1ul << map->len_pow ) );

	size_t widx = idx >> _WORD_POW;
	int len_pow = map->len_pow - _WORD_POW;
	switch( is_put ) {
		case BIT_SET:
			map->bits[ widx ] |= 1ul << ( idx & _BIT_NUMBER_MASK );
			break;
		case BIT_UNSET:
			map->bits[ widx ] &= ~ ( 1ul << ( idx & _BIT_NUMBER_MASK ) );
			break;
	}

	if( len_pow < 2 )
		return;

	AO_t mask = _group_mask_for_bit( len_pow, idx );
	int is_full = ( is_put == BIT_SET ) ?
		( ( map->bits[ widx ] & mask ) == mask ) :
		( ( map->bits[ widx ] & mask ) == 0 );
	AO_t bit = 0;
	AO_t *level_border = map->bits + ( 1ul << len_pow );
	while( len_pow > _WORD_POW ) {
		len_pow -= _WORD_POW;
		idx >>= _WORD_POW;
		widx >>= _WORD_POW;

		bit = 1ul << ( idx & _BIT_NUMBER_MASK );
		level_border[ ( widx << 1 ) + is_put ] |= bit;

		if( is_full )
			is_full = (
				(
					( level_border[ ( widx << 1 ) + 1 - is_put ] &= ~bit ) &
					_group_mask_for_bit( len_pow, idx )
				)
			) == 0;

		level_border += ( 1ul << len_pow ) << 1;
	}

	if( len_pow >= 2 ) {
		idx >>= len_pow;

		bit = 1ul << ( idx & _BIT_NUMBER_MASK );
		level_border[ is_put ] |= bit;

		if( is_full )
			level_border[ 1 - is_put ] &= ~bit;
	}
}

inline static size_t _ffsl_after( AO_t mword, size_t idx ) {
	if( mword == 0 )
		return 0;

	#if defined( __GNUC__ ) && !defined( __INTEL_COMPILER )
		int ret = __builtin_ffsl( mword >> idx );
	#else
		int ret = ffsl( mword >> idx );
	#endif
	
	if( ret )
		return ( idx + ret );
	else
		return 0;

	return 0;
}

inline static size_t _ffcl_after( AO_t mword, size_t idx ) {
	return _ffsl_after( ~ mword, idx );
}

inline static size_t _get_bit_number( AO_t *level_border,
	size_t idx,
	enum hitmap_mark which
) {
	return _ffsl_after(
		AO_load( &( level_border[ ( idx >> ( _WORD_POW - 1 ) ) + which ] ) ),
		idx & _BIT_NUMBER_MASK
	);
}

static int _map_bob_up( AO_t **level_borderp,
	int *cur_powp,
	size_t *idxp,
	enum hitmap_mark which
) {
	int cur_pow = *cur_powp;
	AO_t *level_border = *level_borderp;
	size_t idx = *idxp;

	size_t bitn = 0;
	if( cur_pow > _WORD_POW )
		while( 1 ) {
			bitn = _get_bit_number( level_border, idx, which );

			if( bitn != 0 )
				break;

			idx = ( idx & ( ~ _BIT_NUMBER_MASK ) ) + HITMAP_INITIAL_SIZE;

			if( idx >= ( 1ul << cur_pow )  )
				return 0;

			if( cur_pow > 2 * _WORD_POW ) {
				cur_pow -= _WORD_POW;
				level_border += ( 1ul << ( cur_pow + 1 ) );
				idx >>= _WORD_POW;
			} else {
				if( cur_pow >= ( 2 + _WORD_POW ) ) {
					cur_pow -= _WORD_POW;
					level_border += ( 1ul << ( cur_pow + 1 ) );
					idx >>= cur_pow;
				}

				bitn = _get_bit_number( level_border, idx, which );
				break;
			}
		}
	else
		bitn = _get_bit_number( level_border, idx, which );

	if( bitn != 0 ) {
		*idxp =( idx & ( ~ _BIT_NUMBER_MASK ) ) | ( bitn - 1 );
		*cur_powp = cur_pow;
		*level_borderp = level_border;
		return 1;
	}

	return 0;
}

static int _map_sink( AO_t **level_borderp,
	int len_pow,
	int *cur_powp,
	size_t *idxp,
	enum hitmap_mark which
) {
	int cur_pow = *cur_powp;
	
	if( ( cur_pow + _WORD_POW ) >= len_pow ) {
		*idxp <<= len_pow - cur_pow;
		return 1;
	}

	AO_t *level_border = *level_borderp - ( 1ul << ( cur_pow + 1 ) );
	size_t idx = *idxp << ( ( cur_pow > _WORD_POW ) ? _WORD_POW : cur_pow );
	cur_pow += _WORD_POW;

	size_t bitn = 0;
	while( cur_pow < len_pow ) {
		bitn = _get_bit_number( level_border, idx, which );

		if( bitn == 0 ) {
			*idxp = idx;
			*level_borderp = level_border;
			*cur_powp = cur_pow;
			return 0;
		}

		idx = ( ( idx & ( ~ _BIT_NUMBER_MASK ) ) | ( bitn - 1 ) ) << _WORD_POW;
		level_border -= ( 1ul << ( cur_pow + 1 ) );
		cur_pow += _WORD_POW;
	}

	*idxp = idx;

	return 1;
}

enum _scan_result {
	SCAN_NOT_FOUND,
	SCAN_FOUND,
	SCAN_NEXT
};

static inline enum _scan_result _scan_0_level( map_t *map,
	size_t *idxp,
	enum hitmap_mark which
) {
	size_t idx = *idxp;
	AO_t mword = AO_load( &( map->bits[ idx >> _WORD_POW ] ) );
	size_t bitn = ( which == BIT_SET ) ?
		_ffsl_after( mword, idx & _BIT_NUMBER_MASK ) :
		_ffcl_after( mword, idx & _BIT_NUMBER_MASK );

	idx &= ~ _BIT_NUMBER_MASK;

	if( bitn != 0 ) {
		*idxp = idx | ( bitn - 1 );
		return SCAN_FOUND;
	}

	idx += HITMAP_INITIAL_SIZE;

	if( idx >= ( 1ul << map->len_pow )  )
		return SCAN_NOT_FOUND;

	*idxp = idx;

	return SCAN_NEXT;
}

size_t hitmap_discover( map_t *map,
	size_t start_idx,
	enum hitmap_mark which
) {
	assert( map != NULL );
	assert( start_idx < ( 1ul << map->len_pow ) );

	switch( _scan_0_level( map, &start_idx, which ) ) {
		case SCAN_FOUND:
			return start_idx;
		case SCAN_NOT_FOUND:
			return SIZE_MAX;
		default:
			break;
	}

	int cur_pow = map->len_pow - _WORD_POW;
	if( cur_pow < 2 ) {
		AO_t mword = AO_load( &( map->bits[ start_idx >> _WORD_POW ] ) );
		size_t bitn = ( which == BIT_SET ) ?
			_ffsl_after( mword, 0 ) :
			_ffcl_after( mword, 0 );

		if( bitn != 0 )
			return start_idx | ( bitn - 1 );
		else
			return SIZE_MAX;
	} else {
		int initial_shift = ( cur_pow > _WORD_POW ) ? _WORD_POW : cur_pow;
		AO_t *level_border = map->bits + ( 1ul << cur_pow );
		start_idx >>= initial_shift;
		AO_t *initial_level_border = level_border;
		int initial_cur_pow = cur_pow;
		while( 1 ) {
			do{
				if( ! _map_bob_up(
					&level_border, &cur_pow, &start_idx, which
				) )
					return SIZE_MAX;
			} while(
				! _map_sink(
					&level_border, map->len_pow, &cur_pow, &start_idx, which
				)
			);

			switch( _scan_0_level( map, &start_idx, which ) ) {
				case SCAN_FOUND:
					return start_idx;
				case SCAN_NOT_FOUND:
					return SIZE_MAX;
				default:
					break;
			}

			cur_pow = initial_cur_pow;
			level_border = initial_level_border;
			start_idx >>= initial_shift;
		}
	}

	return SIZE_MAX;
}

int hitmap_has( AO_t map[], int len_pow, enum hitmap_mark which ) {
	assert( map != NULL );

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
