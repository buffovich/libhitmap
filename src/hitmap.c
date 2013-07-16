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
		if( ! ( mword & mask ) )
			return cyc;

	return 0;
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
	if( cur_pow < 2 ) {
		bitn = _get_bit_number( level_border, idx, which );
		if( bitn == 0 )
			idx = ( idx & ( ~ _BIT_NUMBER_MASK ) ) + HITMAP_INITIAL_SIZE;
		if( idx >= ( 1ul << cur_pow )  )
			return 0;
	} else
		while( cur_pow > _WORD_POW ) {
			bitn = _get_bit_number( level_border, idx, which );

			if( bitn != 0 )
				break;

			idx = ( idx & ( ~ _BIT_NUMBER_MASK ) ) + HITMAP_INITIAL_SIZE;

			if( idx >= ( 1ul << cur_pow )  )
				return 0;

			cur_pow -= _WORD_POW;
			if( cur_pow > _WORD_POW ) {
				level_border += ( 1ul << ( cur_pow + 1 ) );
				idx >>= _WORD_POW;
			}
		}

	if( bitn == 0 ) {
		if( cur_pow < 2 ) {
			bitn = _get_bit_number( level_border, idx, which );
			cur_pow += _WORD_POW;
		} else {
			idx >>= cur_pow;
			level_border += ( 1ul << ( cur_pow + 1 ) );
			bitn = _get_bit_number( level_border, idx, which );
		}
	}

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

		cur_pow += _WORD_POW;
		if( cur_pow < len_pow )
			level_border -= ( 1ul << ( cur_pow + 1 ) );
	}

	*idxp = idx;

	return 1;
}

size_t hitmap_discover( map_t *map,
	size_t start_idx,
	enum hitmap_mark which
) {
	assert( map != NULL );
	assert( start_idx < ( 1ul << map->len_pow ) );

	AO_t mword = AO_load( &( map->bits[ start_idx >> _WORD_POW ] ) );
	size_t bitn = ( which == BIT_SET ) ?
		_ffsl_after( mword, start_idx & _BIT_NUMBER_MASK ) :
		_ffcl_after( mword, start_idx & _BIT_NUMBER_MASK );

	start_idx &= ~ _BIT_NUMBER_MASK;

	if( bitn != 0 )
		return start_idx | ( bitn - 1 );

	if( map->len_pow <= _WORD_POW )
		return SIZE_MAX;

	start_idx += HITMAP_INITIAL_SIZE;

	if( start_idx >= ( 1ul << map->len_pow )  )
		return SIZE_MAX;

	int cur_pow = map->len_pow - _WORD_POW;
	if( cur_pow < 2 ) {
		mword = AO_load( &( map->bits[ start_idx >> _WORD_POW ] ) );
		bitn = ( which == BIT_SET ) ?
			_ffsl_after( mword, 0 ) :
			_ffcl_after( mword, 0 );

		if( bitn != 0 )
			return start_idx | ( bitn - 1 );
		else
			return SIZE_MAX;
	} else {
		AO_t *level_border = map->bits + ( 1ul << cur_pow );
		start_idx >>= ( ( cur_pow > _WORD_POW ) ? _WORD_POW : cur_pow );
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

			mword = AO_load( &( map->bits[ start_idx >> _WORD_POW ] ) );
			bitn = ( which == BIT_SET ) ?
				_ffsl_after( mword, start_idx & _BIT_NUMBER_MASK ) :
				_ffcl_after( mword, start_idx & _BIT_NUMBER_MASK );

			start_idx &= ~ _BIT_NUMBER_MASK;

			if( bitn != 0 )
				return start_idx | ( bitn - 1 );

			start_idx += HITMAP_INITIAL_SIZE;

			if( start_idx >= ( 1ul << map->len_pow )  )
				return SIZE_MAX;

			cur_pow = map->len_pow - _WORD_POW;
			level_border = map->bits + ( 1ul << cur_pow );
			start_idx >>= ( ( cur_pow > _WORD_POW ) ? _WORD_POW : cur_pow );
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
