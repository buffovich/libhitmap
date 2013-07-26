#define _GNU_SOURCE

#include <hitmap.h>

#include <atomic_ops.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#if defined( __GNUC__ ) && !defined( __INTEL_COMPILER )
	#define ffsl __builtin_ffsl
#endif

#define _BIT_NUMBER_MASK ( HITMAP_INITIAL_SIZE - 1 )

size_t dummy_calc_sz( int len_pow ) {
	return ( sizeof( map_t ) +
		( 1ul << ( len_pow - _WORD_POW ) ) * sizeof( AO_t )
	);
}

void dummy_init( map_t *map, int len_pow ) {
	if( len_pow < _WORD_POW )
		len_pow = _WORD_POW;

	map->len_pow = len_pow;

	memset( map->bits, 0, sizeof( AO_t ) * ( 1ul << ( len_pow - _WORD_POW ) ) );
}

void dummy_change_for( map_t *map,
	size_t idx,
	enum hitmap_mark is_put
) {
	assert( map != NULL );
	assert( ( is_put == BIT_SET ) || ( is_put == BIT_UNSET ) );
	assert( idx < ( 1ul << map->len_pow ) );
	
	switch( is_put ) {
		case BIT_SET:
			map->bits[ idx >> _WORD_POW ] |= 1ul << ( idx & _BIT_NUMBER_MASK );
			break;
		case BIT_UNSET:
			map->bits[ idx >> _WORD_POW ] &=
				~ ( 1ul << ( idx & _BIT_NUMBER_MASK ) );
			break;
	}
}

size_t dummy_discover( map_t *map,
	size_t start_idx,
	enum hitmap_mark which
) {
	assert( map != NULL );
	assert( ( which == BIT_SET ) || ( which == BIT_UNSET ) );
	assert( start_idx < ( 1ul << map->len_pow ) );

	size_t wcapacity = 1ul << ( map->len_pow - _WORD_POW );
	size_t bitn = 0;
	switch( which ) {
		case BIT_SET:
			for( size_t cyc = start_idx >> _WORD_POW;
				cyc < wcapacity;
				++cyc
			) {
				bitn = ( size_t ) ffsl( AO_load( map->bits + cyc ) );
				if( bitn != 0 )
					return ( cyc << _WORD_POW ) | ( bitn - 1 );
			}
			break;
		case BIT_UNSET:
			for( size_t cyc = start_idx >> _WORD_POW;
				cyc < wcapacity;
				++cyc
			) {
				bitn = ffsl( ~ AO_load( map->bits + cyc ) );
				if( bitn != 0 )
					return ( cyc << _WORD_POW ) | ( bitn - 1 );
			}
			break;
	}

	return SIZE_MAX;
}

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
	if( mword >>= idx )
		return ( idx + ffsl( mword ) );
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
		AO_load( level_border + ( ( idx >> _WORD_POW ) << 1 ) + which ),
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

static inline AO_t _load_word( map_t *map , size_t idx ) {
	return AO_load( map->bits + ( idx >> _WORD_POW ) );
}

static inline size_t _scan_0_level( map_t *map,
	size_t idx,
	enum hitmap_mark which
) {
	size_t bitn = ( which == BIT_SET ) ?
		_ffsl_after( _load_word( map, idx ), idx & _BIT_NUMBER_MASK ) :
		_ffcl_after( _load_word( map, idx ), idx & _BIT_NUMBER_MASK );
	idx &= ~ _BIT_NUMBER_MASK;
	if( bitn != 0 )
		return ( idx | ( bitn - 1 ) );

	return SIZE_MAX;
}

size_t hitmap_discover( map_t *map,
	size_t start_idx,
	enum hitmap_mark which
) {
	assert( map != NULL );
	assert( start_idx < ( 1ul << map->len_pow ) );

	size_t new_idx = _scan_0_level( map, start_idx, which );
	if( new_idx != SIZE_MAX )
		return new_idx;

	start_idx += HITMAP_INITIAL_SIZE;
	if( start_idx >= ( 1ul << map->len_pow )  )
		return SIZE_MAX;

	int cur_pow = map->len_pow - _WORD_POW;
	if( cur_pow < 2 ) {
		size_t bitn = ( which == BIT_SET ) ?
			ffsl( _load_word( map, start_idx ) ) :
			ffsl( ~ _load_word( map, start_idx ) );

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

				if( ( cur_pow + _WORD_POW ) >= map->len_pow ) {
					start_idx <<= ( cur_pow < _WORD_POW ) ? cur_pow : _WORD_POW;
					break;
				}
			} while(
				! _map_sink(
					&level_border, map->len_pow, &cur_pow, &start_idx, which
				)
			);

			if(
				( new_idx = _scan_0_level( map, start_idx, which ) ) !=
				SIZE_MAX
			)
				return new_idx;

			start_idx += HITMAP_INITIAL_SIZE;
			if( start_idx >= ( 1ul << map->len_pow )  )
				return SIZE_MAX;

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
